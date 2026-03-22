# Windows Notification Support Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Windows toast notification support via `src/notification_win.c`, implementing the existing `notification.h` API using WinRT COM.

**Architecture:** Platform-specific `.c` files behind a shared header. CMake selects the right backend. WinRT interface declarations inlined in `notification_win.c`. C standard downgraded from C23 to C99 for MSVC compatibility.

**Tech Stack:** C99, WinRT COM (Windows), libnotify (Linux), CMake

**Spec:** `docs/superpowers/specs/2026-03-22-windows-notification-design.md`

---

## File Structure

| Action | File | Responsibility |
|--------|------|----------------|
| Create | `src/notification_win.c` | Windows toast notification implementation |
| Modify | `CMakeLists.txt` | Platform-conditional compilation, C99, MSVC flags |

---

### Task 1: Downgrade C standard from C23 to C99

**Files:**
- Modify: `CMakeLists.txt:16`

- [ ] **Step 1: Change C standard**

In `CMakeLists.txt`, change line 16:

```cmake
set(CMAKE_C_STANDARD 99)
```

- [ ] **Step 2: Verify Linux build still works**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
```
Expected: builds successfully with no errors.

- [ ] **Step 3: Run tests**

Run:
```bash
cd build && ctest --output-on-failure
```
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt
git commit -m "chore: downgrade C standard from C23 to C99 for MSVC compatibility"
```

---

### Task 2: Add MSVC compiler flag guards in CMake

**Files:**
- Modify: `CMakeLists.txt:39-52` (the `muslimtify_set_target_defaults` function)

- [ ] **Step 1: Guard GCC/Clang-only flags**

Replace the `muslimtify_set_target_defaults` function body. The include directories stay the same, but wrap compiler flags:

```cmake
function(muslimtify_set_target_defaults target)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wformat=2
        )
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()
```

- [ ] **Step 2: Verify Linux build still works**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
```
Expected: builds successfully.

- [ ] **Step 3: Run tests**

Run:
```bash
cd build && ctest --output-on-failure
```
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt
git commit -m "chore: guard GCC/Clang-only compiler flags for MSVC compatibility"
```

---

### Task 3: Add platform-conditional notification source and dependencies in CMake

**Files:**
- Modify: `CMakeLists.txt:28-31` (dependencies section)
- Modify: `CMakeLists.txt:56-69` (object library source list)
- Modify: `CMakeLists.txt:84-89` (link libraries)

- [ ] **Step 1: Guard libnotify dependency**

Replace the dependencies section (lines 28-31):

```cmake
# ── Dependencies ──────────────────────────────────────────────────────────────

if(WIN32)
    # Notification uses WinRT — linked directly, no pkg-config needed
    # libcurl Windows integration is out of scope for this plan
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
    pkg_check_modules(LIBCURL REQUIRED libcurl)
endif()
```

Note: `libcurl` is moved inside the `else()` guard so the Windows build doesn't fail on `pkg_check_modules`. The Windows libcurl integration is a separate future concern (spec: out of scope).

- [ ] **Step 2: Platform-select notification source in object library**

In the `add_library(muslimtify_lib OBJECT ...)` block, replace the `src/notification.c` line:

```cmake
    $<IF:$<BOOL:${WIN32}>,src/notification_win.c,src/notification.c>
```

- [ ] **Step 3: Platform-select link libraries**

Replace the `target_link_libraries(muslimtify ...)` block:

```cmake
target_link_libraries(muslimtify
    muslimtify_lib
    $<$<BOOL:${WIN32}>:ole32>
    $<$<BOOL:${WIN32}>:runtimeobject>
    $<$<NOT:$<BOOL:${WIN32}>>:${LIBNOTIFY_LIBRARIES}>
    ${LIBCURL_LIBRARIES}
    m
)
```

- [ ] **Step 4: Guard libnotify include dirs**

In the `target_include_directories` for `muslimtify_lib` and `muslimtify`, wrap `${LIBNOTIFY_INCLUDE_DIRS}` so it's only added on non-Windows:

```cmake
if(NOT WIN32)
    target_include_directories(muslimtify_lib PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})
endif()
target_include_directories(muslimtify_lib PRIVATE ${LIBCURL_INCLUDE_DIRS})
```

Apply the same pattern for the `muslimtify` executable and all test targets (`test_cli`, `test_prayer_checker`, `test_config`):

```cmake
# For muslimtify executable:
if(NOT WIN32)
    target_include_directories(muslimtify PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})
endif()
target_include_directories(muslimtify PRIVATE ${LIBCURL_INCLUDE_DIRS})

# For each test target (test_cli, test_prayer_checker, test_config):
if(NOT WIN32)
    target_include_directories(test_cli PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})
endif()
target_include_directories(test_cli PRIVATE ${LIBCURL_INCLUDE_DIRS})

# Same for link libraries in test targets:
if(WIN32)
    target_link_libraries(test_cli muslimtify_lib ole32 runtimeobject ${LIBCURL_LIBRARIES} m)
else()
    target_link_libraries(test_cli muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
endif()
```

Repeat for `test_prayer_checker` and `test_config`.

- [ ] **Step 5: Verify Linux build still works**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
```
Expected: builds successfully (Linux path unchanged).

- [ ] **Step 6: Run tests**

Run:
```bash
cd build && ctest --output-on-failure
```
Expected: all tests pass.

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt
git commit -m "feat: add platform-conditional notification source selection in CMake"
```

---

### Task 4: Create `src/notification_win.c` — WinRT declarations

**Files:**
- Create: `src/notification_win.c`

This task creates the file with only the WinRT interface declarations and static state. No function implementations yet.

- [ ] **Step 1: Write the WinRT declarations and state struct**

Create `src/notification_win.c` with the following content. This is extracted from `external/ToastLib/libsrc/ToasterDecl.h` — only the interfaces needed for the send pipeline.

```c
#define UNICODE
#define _UNICODE
#define COBJMACROS

#include "../include/notification.h"
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── WinRT type declarations ──────────────────────────────────────────────── */

/* HSTRING types (from winstring.h) */
typedef struct HSTRING__ { int unused; } HSTRING__;
typedef HSTRING__ *HSTRING;
typedef struct HSTRING_HEADER {
  union {
    void *Reserved1;
#if defined(_WIN64)
    char Reserved2[24];
#else
    char Reserved2[20];
#endif
  } Reserved;
} HSTRING_HEADER;

STDAPI WindowsCreateStringReference(PCWSTR sourceString, UINT32 length,
                                    HSTRING_HEADER *hstringHeader,
                                    HSTRING *string);
STDAPI WindowsDeleteString(HSTRING string);

/* IInspectable (from inspectable.h) */
typedef interface IInspectable IInspectable;
typedef enum { BaseTrust = 0, PartialTrust = 1, FullTrust = 2 } TrustLevel;
typedef struct IInspectableVtbl {
  BEGIN_INTERFACE
  HRESULT(STDMETHODCALLTYPE *QueryInterface)
  (IInspectable *This, REFIID riid, void **ppvObject);
  ULONG(STDMETHODCALLTYPE *AddRef)(IInspectable *This);
  ULONG(STDMETHODCALLTYPE *Release)(IInspectable *This);
  HRESULT(STDMETHODCALLTYPE *GetIids)
  (IInspectable *This, ULONG *iidCount, IID **iids);
  HRESULT(STDMETHODCALLTYPE *GetRuntimeClassName)
  (IInspectable *This, HSTRING *className);
  HRESULT(STDMETHODCALLTYPE *GetTrustLevel)
  (IInspectable *This, TrustLevel *trustLevel);
  END_INTERFACE
} IInspectableVtbl;
interface IInspectable {
  CONST_VTBL struct IInspectableVtbl *lpVtbl;
};

/* Forward declarations for vtable references */
typedef interface IXmlDocument IXmlDocument;
typedef interface IXmlDocumentIO IXmlDocumentIO;
typedef interface IToastNotification IToastNotification;
typedef interface IToastNotifier IToastNotifier;
typedef interface IToastNotificationFactory IToastNotificationFactory;
typedef interface IToastNotificationManagerStatics
    IToastNotificationManagerStatics;

/* IXmlDocument */
typedef struct IXmlDocumentVtbl {
  BEGIN_INTERFACE
  HRESULT(STDMETHODCALLTYPE *QueryInterface)
  (IXmlDocument *This, REFIID riid, void **ppvObject);
  ULONG(STDMETHODCALLTYPE *AddRef)(IXmlDocument *This);
  ULONG(STDMETHODCALLTYPE *Release)(IXmlDocument *This);
  HRESULT(STDMETHODCALLTYPE *GetIids)
  (IXmlDocument *This, ULONG *iidCount, IID **iids);
  HRESULT(STDMETHODCALLTYPE *GetRuntimeClassName)
  (IXmlDocument *This, HSTRING *className);
  HRESULT(STDMETHODCALLTYPE *GetTrustLevel)
  (IXmlDocument *This, TrustLevel *trustLevel);
  /* IXmlDocument methods — we only need it as an opaque handle passed to
     CreateToastNotification, so the remaining vtable slots are void* padding */
  void *get_Doctype;
  void *get_Implementation;
  void *get_DocumentElement;
  void *CreateElement;
  void *CreateDocumentFragment;
  void *CreateTextNode;
  void *CreateComment;
  void *CreateProcessingInstruction;
  void *CreateAttribute;
  void *CreateEntityReference;
  void *GetElementsByTagName;
  void *CreateCDataSection;
  void *get_DocumentUri;
  void *CreateAttributeNS;
  void *CreateElementNS;
  void *GetElementById;
  void *ImportNode;
  END_INTERFACE
} IXmlDocumentVtbl;
interface IXmlDocument {
  CONST_VTBL struct IXmlDocumentVtbl *lpVtbl;
};

/* IXmlDocumentIO */
typedef struct IXmlDocumentIOVtbl {
  BEGIN_INTERFACE
  HRESULT(STDMETHODCALLTYPE *QueryInterface)
  (IXmlDocumentIO *This, REFIID riid, void **ppvObject);
  ULONG(STDMETHODCALLTYPE *AddRef)(IXmlDocumentIO *This);
  ULONG(STDMETHODCALLTYPE *Release)(IXmlDocumentIO *This);
  HRESULT(STDMETHODCALLTYPE *GetIids)
  (IXmlDocumentIO *This, ULONG *iidCount, IID **iids);
  HRESULT(STDMETHODCALLTYPE *GetRuntimeClassName)
  (IXmlDocumentIO *This, HSTRING *className);
  HRESULT(STDMETHODCALLTYPE *GetTrustLevel)
  (IXmlDocumentIO *This, TrustLevel *trustLevel);
  HRESULT(STDMETHODCALLTYPE *LoadXml)(IXmlDocumentIO *This, HSTRING xml);
  void *LoadXmlWithSettings;
  void *SaveToFileAsync;
  END_INTERFACE
} IXmlDocumentIOVtbl;
interface IXmlDocumentIO {
  CONST_VTBL struct IXmlDocumentIOVtbl *lpVtbl;
};

/* IToastNotification */
typedef struct IToastNotificationVtbl {
  BEGIN_INTERFACE
  HRESULT(STDMETHODCALLTYPE *QueryInterface)
  (IToastNotification *This, REFIID riid, void **ppvObject);
  ULONG(STDMETHODCALLTYPE *AddRef)(IToastNotification *This);
  ULONG(STDMETHODCALLTYPE *Release)(IToastNotification *This);
  HRESULT(STDMETHODCALLTYPE *GetIids)
  (IToastNotification *This, ULONG *iidCount, IID **iids);
  HRESULT(STDMETHODCALLTYPE *GetRuntimeClassName)
  (IToastNotification *This, HSTRING *className);
  HRESULT(STDMETHODCALLTYPE *GetTrustLevel)
  (IToastNotification *This, TrustLevel *trustLevel);
  void *get_Content;
  void *put_ExpirationTime;
  void *get_ExpirationTime;
  void *add_Dismissed;
  void *remove_Dismissed;
  void *add_Activated;
  void *remove_Activated;
  void *add_Failed;
  void *remove_Failed;
  END_INTERFACE
} IToastNotificationVtbl;
interface IToastNotification {
  CONST_VTBL struct IToastNotificationVtbl *lpVtbl;
};

/* IToastNotifier */
typedef struct IToastNotifierVtbl {
  BEGIN_INTERFACE
  HRESULT(STDMETHODCALLTYPE *QueryInterface)
  (IToastNotifier *This, REFIID riid, void **ppvObject);
  ULONG(STDMETHODCALLTYPE *AddRef)(IToastNotifier *This);
  ULONG(STDMETHODCALLTYPE *Release)(IToastNotifier *This);
  HRESULT(STDMETHODCALLTYPE *GetIids)
  (IToastNotifier *This, ULONG *iidCount, IID **iids);
  HRESULT(STDMETHODCALLTYPE *GetRuntimeClassName)
  (IToastNotifier *This, HSTRING *className);
  HRESULT(STDMETHODCALLTYPE *GetTrustLevel)
  (IToastNotifier *This, TrustLevel *trustLevel);
  HRESULT(STDMETHODCALLTYPE *Show)
  (IToastNotifier *This, IToastNotification *notification);
  void *Hide;
  void *get_Setting;
  void *AddToSchedule;
  void *RemoveFromSchedule;
  void *GetScheduledToastNotifications;
  END_INTERFACE
} IToastNotifierVtbl;
interface IToastNotifier {
  CONST_VTBL struct IToastNotifierVtbl *lpVtbl;
};

/* IToastNotificationFactory */
typedef struct IToastNotificationFactoryVtbl {
  BEGIN_INTERFACE
  HRESULT(STDMETHODCALLTYPE *QueryInterface)
  (IToastNotificationFactory *This, REFIID riid, void **ppvObject);
  ULONG(STDMETHODCALLTYPE *AddRef)(IToastNotificationFactory *This);
  ULONG(STDMETHODCALLTYPE *Release)(IToastNotificationFactory *This);
  HRESULT(STDMETHODCALLTYPE *GetIids)
  (IToastNotificationFactory *This, ULONG *iidCount, IID **iids);
  HRESULT(STDMETHODCALLTYPE *GetRuntimeClassName)
  (IToastNotificationFactory *This, HSTRING *className);
  HRESULT(STDMETHODCALLTYPE *GetTrustLevel)
  (IToastNotificationFactory *This, TrustLevel *trustLevel);
  HRESULT(STDMETHODCALLTYPE *CreateToastNotification)
  (IToastNotificationFactory *This, IXmlDocument *content,
   IToastNotification **notification);
  END_INTERFACE
} IToastNotificationFactoryVtbl;
interface IToastNotificationFactory {
  CONST_VTBL struct IToastNotificationFactoryVtbl *lpVtbl;
};

/* IToastNotificationManagerStatics */
typedef struct IToastNotificationManagerStaticsVtbl {
  BEGIN_INTERFACE
  HRESULT(STDMETHODCALLTYPE *QueryInterface)
  (IToastNotificationManagerStatics *This, REFIID riid, void **ppvObject);
  ULONG(STDMETHODCALLTYPE *AddRef)(IToastNotificationManagerStatics *This);
  ULONG(STDMETHODCALLTYPE *Release)(IToastNotificationManagerStatics *This);
  HRESULT(STDMETHODCALLTYPE *GetIids)
  (IToastNotificationManagerStatics *This, ULONG *iidCount, IID **iids);
  HRESULT(STDMETHODCALLTYPE *GetRuntimeClassName)
  (IToastNotificationManagerStatics *This, HSTRING *className);
  HRESULT(STDMETHODCALLTYPE *GetTrustLevel)
  (IToastNotificationManagerStatics *This, TrustLevel *trustLevel);
  HRESULT(STDMETHODCALLTYPE *CreateToastNotifier)
  (IToastNotificationManagerStatics *This, IToastNotifier **notifier);
  HRESULT(STDMETHODCALLTYPE *CreateToastNotifierWithId)
  (IToastNotificationManagerStatics *This, HSTRING applicationId,
   IToastNotifier **notifier);
  void *GetTemplateContent;
  END_INTERFACE
} IToastNotificationManagerStaticsVtbl;
interface IToastNotificationManagerStatics {
  CONST_VTBL struct IToastNotificationManagerStaticsVtbl *lpVtbl;
};

/* ── GUIDs ────────────────────────────────────────────────────────────────── */

static const IID IID_IToastNotificationManagerStatics = {
    0x50ac103f, 0xd235, 0x4598,
    {0xbb, 0xef, 0x98, 0xfe, 0x4d, 0x1a, 0x3a, 0xd4}};
static const IID IID_IToastNotificationFactory = {
    0x04124b20, 0x82c6, 0x4229,
    {0xb1, 0x09, 0xfd, 0x9e, 0xd4, 0x66, 0x2b, 0x53}};
static const IID IID_IXmlDocument = {
    0xf7f3a506, 0x1e87, 0x42d6,
    {0xbc, 0xfb, 0xb8, 0xc8, 0x09, 0xfa, 0x54, 0x94}};
static const IID IID_IXmlDocumentIO = {
    0x6cd0e74e, 0xee65, 0x4489,
    {0x9e, 0xbf, 0xca, 0x43, 0xe8, 0x7b, 0xa6, 0x37}};

/* ── Runtime class names ──────────────────────────────────────────────────── */

static const WCHAR RuntimeClass_ToastNotificationManager[] =
    L"Windows.UI.Notifications.ToastNotificationManager";
static const WCHAR RuntimeClass_ToastNotification[] =
    L"Windows.UI.Notifications.ToastNotification";
static const WCHAR RuntimeClass_XmlDocument[] =
    L"Windows.Data.Xml.Dom.XmlDocument";

/* ── RoAPI declarations ───────────────────────────────────────────────────── */

#ifndef ROAPI
#ifdef _ROAPI_
#define ROAPI
#else
#define ROAPI DECLSPEC_IMPORT
#endif
typedef enum { RO_INIT_MULTITHREADED = 1 } RO_INIT_TYPE;
ROAPI HRESULT WINAPI RoInitialize(RO_INIT_TYPE initType);
ROAPI HRESULT WINAPI RoGetActivationFactory(HSTRING activatableClassId,
                                            REFIID iid, void **factory);
ROAPI HRESULT WINAPI RoActivateInstance(HSTRING activatableClassId,
                                        IInspectable **instance);
ROAPI void WINAPI RoUninitialize(void);
#endif

/* ── AUMID for unpackaged app ─────────────────────────────────────────────── */

static const WCHAR MUSLIMTIFY_AUMID[] =
    L"{1AC14E77-02E7-4E5D-B744-2EB1AE5198B7}"
    L"\\WindowsPowerShell\\v1.0\\powershell.exe";

/* ── File-static state ────────────────────────────────────────────────────── */

typedef struct {
  IToastNotificationFactory *factory;
  IToastNotifier *notifier;
  BOOL initialized;
} NotifyState;

static NotifyState g_state = {0};

/* Function implementations will be added in Task 5 */
```

- [ ] **Step 2: Commit**

```bash
git add src/notification_win.c
git commit -m "feat: add WinRT declarations for Windows notification backend"
```

---

### Task 5: Implement the 4 notification functions in `notification_win.c`

**Files:**
- Modify: `src/notification_win.c` (append after the declarations from Task 4)

- [ ] **Step 1: Add helper functions and all 4 API implementations**

Append the following after the `static NotifyState g_state = {0};` line:

```c
/* ── Helpers ──────────────────────────────────────────────────────────────── */

/* Convert UTF-8 string to UTF-16. Caller must free() the result. */
static wchar_t *utf8_to_utf16(const char *utf8) {
  if (!utf8) return NULL;
  int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
  if (len <= 0) return NULL;
  wchar_t *wide = (wchar_t *)malloc(len * sizeof(wchar_t));
  if (!wide) return NULL;
  MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, len);
  return wide;
}

/* XML-escape a UTF-16 string. Caller must free() the result. */
static wchar_t *xml_escape(const wchar_t *src) {
  if (!src) return NULL;
  /* Worst case: every char becomes "&quot;" or "&apos;" (6x expansion) */
  size_t src_len = wcslen(src);
  wchar_t *escaped = (wchar_t *)malloc((src_len * 6 + 1) * sizeof(wchar_t));
  if (!escaped) return NULL;
  wchar_t *dst = escaped;
  for (size_t i = 0; i < src_len; i++) {
    switch (src[i]) {
    case L'<':
      wcscpy(dst, L"&lt;");
      dst += 4;
      break;
    case L'>':
      wcscpy(dst, L"&gt;");
      dst += 4;
      break;
    case L'&':
      wcscpy(dst, L"&amp;");
      dst += 5;
      break;
    case L'"':
      wcscpy(dst, L"&quot;");
      dst += 6;
      break;
    case L'\'':
      wcscpy(dst, L"&apos;");
      dst += 6;
      break;
    default:
      *dst++ = src[i];
      break;
    }
  }
  *dst = L'\0';
  return escaped;
}

/* Create HSTRING from static wide string (no allocation — reference only) */
static HRESULT make_hstring_ref(const WCHAR *str, HSTRING_HEADER *header,
                                HSTRING *hstr) {
  return WindowsCreateStringReference(str, (UINT32)wcslen(str), header, hstr);
}

/* Send a pre-built toast XML wide string through the WinRT pipeline */
static void send_toast_xml(const wchar_t *xml) {
  /* Create XmlDocument */
  HSTRING_HEADER hsh_xml_cls;
  HSTRING hs_xml_cls = NULL;
  IInspectable *inspectable = NULL;
  if (!SUCCEEDED(
          make_hstring_ref(RuntimeClass_XmlDocument, &hsh_xml_cls, &hs_xml_cls)))
    return;
  if (!SUCCEEDED(RoActivateInstance(hs_xml_cls, &inspectable))) {
    fprintf(stderr, "muslimtify: failed to create XmlDocument\n");
    return;
  }

  /* Query IXmlDocument */
  IXmlDocument *xml_doc = NULL;
  if (!SUCCEEDED(inspectable->lpVtbl->QueryInterface(
          inspectable, &IID_IXmlDocument, (void **)&xml_doc))) {
    inspectable->lpVtbl->Release(inspectable);
    return;
  }
  inspectable->lpVtbl->Release(inspectable);

  /* Query IXmlDocumentIO and load XML */
  IXmlDocumentIO *xml_io = NULL;
  if (!SUCCEEDED(xml_doc->lpVtbl->QueryInterface(xml_doc, &IID_IXmlDocumentIO,
                                                  (void **)&xml_io))) {
    xml_doc->lpVtbl->Release(xml_doc);
    return;
  }

  HSTRING_HEADER hsh_xml;
  HSTRING hs_xml = NULL;
  if (!SUCCEEDED(WindowsCreateStringReference(xml, (UINT32)wcslen(xml),
                                              &hsh_xml, &hs_xml))) {
    xml_io->lpVtbl->Release(xml_io);
    xml_doc->lpVtbl->Release(xml_doc);
    return;
  }
  HRESULT hr = xml_io->lpVtbl->LoadXml(xml_io, hs_xml);
  xml_io->lpVtbl->Release(xml_io);
  if (!SUCCEEDED(hr)) {
    fprintf(stderr, "muslimtify: LoadXml failed\n");
    xml_doc->lpVtbl->Release(xml_doc);
    return;
  }

  /* Create and show toast */
  IToastNotification *toast = NULL;
  if (!SUCCEEDED(g_state.factory->lpVtbl->CreateToastNotification(
          g_state.factory, xml_doc, &toast))) {
    fprintf(stderr, "muslimtify: CreateToastNotification failed\n");
    xml_doc->lpVtbl->Release(xml_doc);
    return;
  }
  xml_doc->lpVtbl->Release(xml_doc);

  hr = g_state.notifier->lpVtbl->Show(g_state.notifier, toast);
  if (!SUCCEEDED(hr)) {
    fprintf(stderr, "muslimtify: toast Show failed\n");
  }
  toast->lpVtbl->Release(toast);
}

/* Build toast XML from title/message with optional scenario attribute */
static void send_notification(const char *title, const char *message,
                              const char *scenario) {
  if (!g_state.initialized) return;
  if (!title || !message) return;

  /* Convert and escape strings */
  wchar_t *wtitle_raw = utf8_to_utf16(title);
  wchar_t *wmsg_raw = utf8_to_utf16(message);
  wchar_t *wtitle = xml_escape(wtitle_raw);
  wchar_t *wmsg = xml_escape(wmsg_raw);
  free(wtitle_raw);
  free(wmsg_raw);
  if (!wtitle || !wmsg) {
    free(wtitle);
    free(wmsg);
    return;
  }

  /* Build toast XML */
  wchar_t xml[1024];
  if (scenario) {
    wchar_t *wscenario = utf8_to_utf16(scenario);
    _snwprintf_s(xml, sizeof(xml) / sizeof(xml[0]), _TRUNCATE,
                 L"<toast scenario=\"%ls\" duration=\"short\">"
                 L"<visual><binding template=\"ToastGeneric\">"
                 L"<text>%ls</text>"
                 L"<text>%ls</text>"
                 L"</binding></visual>"
                 L"</toast>",
                 wscenario, wtitle, wmsg);
    free(wscenario);
  } else {
    _snwprintf_s(xml, sizeof(xml) / sizeof(xml[0]), _TRUNCATE,
                 L"<toast duration=\"short\">"
                 L"<visual><binding template=\"ToastGeneric\">"
                 L"<text>%ls</text>"
                 L"<text>%ls</text>"
                 L"</binding></visual>"
                 L"</toast>",
                 wtitle, wmsg);
  }
  free(wtitle);
  free(wmsg);

  send_toast_xml(xml);
}

/* ── API implementation ───────────────────────────────────────────────────── */

int notify_init_once(const char *app_name) {
  (void)app_name; /* AUMID is used instead on Windows */

  if (g_state.initialized) return 1;

  if (!SUCCEEDED(RoInitialize(RO_INIT_MULTITHREADED))) {
    fprintf(stderr, "muslimtify: RoInitialize failed\n");
    return 0;
  }

  /* Get ToastNotificationManager factory */
  HSTRING_HEADER hsh_mgr;
  HSTRING hs_mgr = NULL;
  IToastNotificationManagerStatics *mgr = NULL;

  if (!SUCCEEDED(make_hstring_ref(RuntimeClass_ToastNotificationManager,
                                  &hsh_mgr, &hs_mgr))) {
    fprintf(stderr, "muslimtify: failed to create manager HSTRING\n");
    goto fail;
  }
  if (!SUCCEEDED(RoGetActivationFactory(
          hs_mgr, &IID_IToastNotificationManagerStatics, (void **)&mgr))) {
    fprintf(stderr, "muslimtify: failed to get ToastNotificationManager\n");
    goto fail;
  }

  /* Create notifier with AUMID */
  HSTRING_HEADER hsh_aumid;
  HSTRING hs_aumid = NULL;
  if (!SUCCEEDED(make_hstring_ref(MUSLIMTIFY_AUMID, &hsh_aumid, &hs_aumid))) {
    mgr->lpVtbl->Release(mgr);
    goto fail;
  }
  if (!SUCCEEDED(mgr->lpVtbl->CreateToastNotifierWithId(
          mgr, hs_aumid, &g_state.notifier))) {
    fprintf(stderr, "muslimtify: failed to create ToastNotifier\n");
    mgr->lpVtbl->Release(mgr);
    goto fail;
  }
  mgr->lpVtbl->Release(mgr);

  /* Get ToastNotification factory */
  HSTRING_HEADER hsh_notif;
  HSTRING hs_notif = NULL;
  if (!SUCCEEDED(make_hstring_ref(RuntimeClass_ToastNotification, &hsh_notif,
                                  &hs_notif))) {
    goto fail;
  }
  if (!SUCCEEDED(RoGetActivationFactory(
          hs_notif, &IID_IToastNotificationFactory, (void **)&g_state.factory))) {
    fprintf(stderr, "muslimtify: failed to get ToastNotificationFactory\n");
    goto fail;
  }

  g_state.initialized = TRUE;
  return 1;

fail:
  if (g_state.notifier) {
    g_state.notifier->lpVtbl->Release(g_state.notifier);
    g_state.notifier = NULL;
  }
  RoUninitialize();
  return 0;
}

void notify_send(const char *title, const char *message) {
  send_notification(title, message, NULL);
}

void notify_prayer(const char *prayer_name, const char *time_str,
                   int minutes_before, const char *urgency_str) {
  char title[128];
  char message[256];

  if (minutes_before == 0) {
    _snprintf_s(title, sizeof(title), _TRUNCATE, "Prayer Time: %s",
                prayer_name);
    _snprintf_s(message, sizeof(message), _TRUNCATE,
                "It's time for %s prayer\nTime: %s", prayer_name, time_str);
  } else {
    _snprintf_s(title, sizeof(title), _TRUNCATE, "Prayer Reminder: %s",
                prayer_name);
    _snprintf_s(message, sizeof(message), _TRUNCATE,
                "%s prayer in %d minutes\nTime: %s", prayer_name,
                minutes_before, time_str);
  }

  /* Map urgency: critical -> scenario="reminder", normal/low -> default toast */
  const char *scenario = NULL;
  if (!urgency_str || strcmp(urgency_str, "critical") == 0) {
    scenario = "reminder";
  }

  send_notification(title, message, scenario);
}

void notify_cleanup(void) {
  if (!g_state.initialized) return;

  if (g_state.factory) {
    g_state.factory->lpVtbl->Release(g_state.factory);
    g_state.factory = NULL;
  }
  if (g_state.notifier) {
    g_state.notifier->lpVtbl->Release(g_state.notifier);
    g_state.notifier = NULL;
  }

  RoUninitialize();
  g_state.initialized = FALSE;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/notification_win.c
git commit -m "feat: implement Windows toast notification functions"
```

---

### Task 6: Verify Linux build is unaffected

**Files:** None modified — verification only.

- [ ] **Step 1: Clean rebuild**

Run:
```bash
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
```
Expected: builds successfully. `notification_win.c` should NOT be compiled (Linux build).

- [ ] **Step 2: Run all tests**

Run:
```bash
cd build && ctest --output-on-failure
```
Expected: all tests pass.

- [ ] **Step 3: Verify notification_win.c is excluded**

Run:
```bash
grep -r "notification_win" build/compile_commands.json
```
Expected: no output (file not in compile commands on Linux).
