# Windows Notification Support

## Summary

Add Windows toast notification support to muslimtify by creating `src/notification_win.c` — a platform-specific implementation of the existing `include/notification.h` API using WinRT COM interfaces. The Linux implementation (`src/notification.c`) remains unchanged. CMake selects the correct backend at build time.

## Motivation

Muslimtify currently only supports Linux desktop notifications via libnotify. To reach Windows users, we need a native Windows notification backend. The ToastLib library (`external/ToastLib/`) provides a working reference implementation of WinRT toast notifications in plain C, but includes more than we need (callbacks, registry registration, GUI demo). We extract only the minimal notification pipeline.

## Design

### API (unchanged)

`include/notification.h` — no changes. Both platforms implement:

```c
int  notify_init_once(const char *app_name);
void notify_send(const char *title, const char *message);
void notify_prayer(const char *prayer_name, const char *time_str,
                   int minutes_before, const char *urgency);
void notify_cleanup(void);
```

### New file: `src/notification_win.c`

A single self-contained file (~300-350 lines) that implements the 4 notification functions using WinRT COM.

**Structure:**

1. **WinRT interface declarations (~150 lines)** — inlined subset of ToastLib's `ToasterDecl.h`, containing only:
   - `HSTRING` / `HSTRING_HEADER` types
   - `IInspectable` interface
   - `IXmlDocument` interface
   - `IXmlDocumentIO` interface
   - `IToastNotification` interface
   - `IToastNotificationFactory` interface
   - `IToastNotificationManagerStatics` interface
   - `IToastNotifier` interface
   - Runtime class name strings
   - WinRT GUIDs (5 IIDs)
   - `RoInitialize`, `RoGetActivationFactory`, `RoActivateInstance`, `RoUninitialize` declarations

2. **File-static state** — a struct holding the toast notifier and notification factory (created once in `notify_init_once`, released in `notify_cleanup`).

3. **Helper: UTF-8 to UTF-16 conversion** — a small static function wrapping `MultiByteToWideChar(CP_UTF8, ...)` to convert `const char *` arguments to `LPCWSTR` for WinRT APIs.

4. **Helper: XML escaping** — a static function that escapes `<`, `>`, `&`, `"`, `'` in title/message strings before embedding them in the toast XML template. Without this, prayer names or messages containing these characters would produce malformed XML.

5. **Function implementations (~80 lines):**
   - `notify_init_once()` — `RoInitialize(RO_INIT_MULTITHREADED)` (which internally calls `CoInitializeEx`), get activation factories via `WindowsCreateStringReference` + `RoGetActivationFactory`, create notifier with AUMID.
   - `notify_send()` — convert title/message to UTF-16, XML-escape them, build toast XML wide string, create HSTRING via `WindowsCreateStringReference`, create `XmlDocument` via `RoActivateInstance`, query `IXmlDocumentIO`, `LoadXml`, `CreateToastNotification`, `Show`. Failures are logged to stderr (fire-and-forget, but not silent).
   - `notify_prayer()` — format title/message (same logic as Linux version), map urgency to toast `scenario` attribute (`"reminder"` for critical, omit for normal/low), call `notify_send`.
   - `notify_cleanup()` — release COM objects, `RoUninitialize()`.

### AUMID (Application User Model ID)

Windows requires an AUMID to associate toast notifications with an application identity. For unpackaged desktop apps (like muslimtify), we use the PowerShell AUMID as a well-known workaround:

```c
L"{1AC14E77-02E7-4E5D-B744-2EB1AE5198B7}\\WindowsPowerShell\\v1.0\\powershell.exe"
```

This allows toasts to display without requiring Start Menu shortcut registration or MSIX packaging. The toast will appear with generic Windows branding. Custom branding (app name, icon) can be added later by registering a proper AUMID via a Start Menu shortcut with `System.AppUserModel.ID` property.

### File-static state

```c
typedef struct {
    IToastNotificationFactory *factory;
    IToastNotifier            *notifier;
    BOOL                       initialized;
} NotifyState;

static NotifyState g_state = {0};
```

Simpler than ToastLib's `TOAST_NOTIFY` — no GUID or app ID duplication needed since we don't support callbacks.

**What we skip from ToastLib:**

| Component | Included | Reason |
|---|---|---|
| Core send pipeline (`C_wrapper.c`) | Yes (adapted) | The notification mechanism |
| Interface declarations (`ToasterDecl.h`) | Partial (5 interfaces) | Skip ~20 unused forward declarations |
| Callback system (`callback.c`) | No | No interactive buttons needed |
| App registration (`fToastRegisterApp`) | No | Only needed for callbacks |
| GUI/console demos | No | Demo code |
| Prebuilt `WRlibs/` | No | Link system `runtimeobject.lib` |

### CMake changes

**Dependencies section:**

```cmake
if(WIN32)
    # Notification uses WinRT — linked directly, no pkg-config needed
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
endif()
# libcurl remains cross-platform (separate concern)
```

**Object library — platform-select notification source:**

```cmake
add_library(muslimtify_lib OBJECT
    src/cli.c
    src/cmd_show.c
    src/cmd_next.c
    src/cmd_config.c
    src/cmd_location.c
    src/cmd_prayer.c
    src/cmd_daemon.c
    src/config.c
    src/location.c
    src/prayer_checker.c
    $<IF:$<BOOL:${WIN32}>,src/notification_win.c,src/notification.c>
    src/display.c
)
```

**Link libraries:**

```cmake
if(WIN32)
    target_link_libraries(muslimtify muslimtify_lib ole32 runtimeobject ${LIBCURL_LIBRARIES} m)
else()
    target_link_libraries(muslimtify muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
endif()
```

**Compiler flags — guard GCC/Clang-only flags:**

```cmake
function(muslimtify_set_target_defaults target)
    target_include_directories(${target} PRIVATE ...)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wformat=2)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()
```

### Urgency mapping

| `notification.h` urgency | Linux (libnotify) | Windows (toast) |
|---|---|---|
| `"critical"` / default | `NOTIFY_URGENCY_CRITICAL` | `scenario="reminder"` (persistent toast) |
| `"normal"` | `NOTIFY_URGENCY_NORMAL` | default toast (no scenario) |
| `"low"` | `NOTIFY_URGENCY_LOW` | default toast (no scenario) |

### Toast XML template

```xml
<toast scenario="{scenario}" duration="short">
  <visual>
    <binding template="ToastGeneric">
      <text>{title}</text>
      <text>{message}</text>
    </binding>
  </visual>
</toast>
```

### C standard: C23 → C99

Change `CMAKE_C_STANDARD` from `23` to `99`. The codebase uses no C23 or C11 features without fallbacks:
- `bool`/`true`/`false` — already via `#include <stdbool.h>` (C99)
- `alignof` in `libjson.h` — already guarded with `#if __STDC_VERSION__ >= 201112L` with C99 fallback
- `restrict` in `libjson.h` — already has conditional macro
- All other features used (designated initializers, `inline`, `//` comments) are C99

This gives MSVC compatibility and broader compiler support.

## Known risks
- **Linux-specific code in other files:** Files like `notification.c` use `<linux/limits.h>`, `readlink("/proc/self/exe")`, `_GNU_SOURCE`. While `notification.c` won't compile on Windows, other files (`config.c`, `display.c`, `cmd_daemon.c`) may also have Linux-specific code. The full Windows build will require platform guards beyond this spec's scope.
- **MinGW compatibility:** The WinRT COM vtable layout assumes MSVC ABI. MinGW may not have the required COM headers or matching vtable layout. This spec targets MSVC only.

## Out of scope

- Windows Task Scheduler integration (systemd equivalent) — separate future work
- `libcurl` Windows build — `location.c` is a separate concern
- Icon support in Windows toasts — can be added later
- Interactive buttons/callbacks — fire-and-forget only
- Custom AUMID registration (Start Menu shortcut) — can be added later for branded toasts
- Platform guards for non-notification source files — separate future work
- The `external/ToastLib/` directory — can be removed after extraction is complete

## Testing

- Cannot unit-test WinRT calls on Linux — the implementation will be verified by building and running on a Windows machine.
- The `notification.h` API contract is the same, so existing test structure applies once a Windows CI is set up.
- Manual verification: build on Windows, run `muslimtify check`, confirm toast appears.
