#define _GNU_SOURCE
#include "../include/notification.h"
#include <libgen.h>
#include <libnotify/notify.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Get the icon path - tries multiple locations
static const char *get_icon_path(void) {
  static char icon_path[PATH_MAX] = {0};

  // Return cached path if already found
  if (icon_path[0] != '\0') {
    return icon_path;
  }

  // Try different icon locations in order of preference
  const char *possible_paths[] = {
      // Installed location (system-wide)
      "/usr/local/share/icons/hicolor/128x128/apps/muslimtify.png",
      "/usr/share/icons/hicolor/128x128/apps/muslimtify.png",

      // XDG data directory
      NULL, // Will be filled with XDG_DATA_HOME

      // Relative to binary (for source builds)
      NULL, // Will be filled with binary path

      // Current directory (fallback)
      "assets/muslimtify.png", "../assets/muslimtify.png",

      NULL};

  // Try XDG_DATA_HOME
  char xdg_path[PATH_MAX];
  const char *xdg_data = getenv("XDG_DATA_HOME");
  if (xdg_data) {
    snprintf(xdg_path, sizeof(xdg_path),
             "%s/icons/hicolor/128x128/apps/muslimtify.png", xdg_data);
    possible_paths[2] = xdg_path;
  }

  // Try relative to binary location
  char assets_path[PATH_MAX];
  {
    char binary_path[PATH_MAX];
    ssize_t len =
        readlink("/proc/self/exe", binary_path, sizeof(binary_path) - 1);
    if (len > 0) {
      binary_path[len] = '\0';
      char *dir = dirname(binary_path);
      snprintf(assets_path, sizeof(assets_path), "%s/../assets/muslimtify.png",
               dir);
      possible_paths[3] = assets_path;
    }
  }

  // Check each path (counted loop — NULL entries are skipped, not sentinels)
  int path_count = (int)(sizeof(possible_paths) / sizeof(possible_paths[0]));
  for (int i = 0; i < path_count; i++) {
    if (possible_paths[i] == NULL) continue;
    if (access(possible_paths[i], R_OK) == 0) {
      // Found readable icon - convert to absolute path
      if (possible_paths[i][0] == '/') {
        // Already absolute
        strncpy(icon_path, possible_paths[i], sizeof(icon_path) - 1);
        icon_path[sizeof(icon_path) - 1] = '\0';
      } else {
        // Convert relative to absolute
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
          int n = snprintf(icon_path, sizeof(icon_path), "%s/%s", cwd,
                           possible_paths[i]);
          if (n < 0 || (size_t)n >= sizeof(icon_path))
            icon_path[0] = '\0'; // truncated — skip this path
        } else {
          strncpy(icon_path, possible_paths[i], sizeof(icon_path) - 1);
          icon_path[sizeof(icon_path) - 1] = '\0';
        }
      }
      return icon_path;
    }
  }

  // Fallback to icon name (let system find it)
  return "mosque";
}

int notify_init_once(const char *app_name) { return notify_init(app_name); }

void notify_send(const char *title, const char *message) {
  NotifyNotification *n =
      notify_notification_new(title, message, get_icon_path());
  notify_notification_set_timeout(n, 3000);
  notify_notification_show(n, NULL);
  g_object_unref(G_OBJECT(n));
}

void notify_prayer(const char *prayer_name, const char *time_str,
                   int minutes_before) {
  char title[128];
  char message[256];

  if (minutes_before == 0) {
    // Exact prayer time notification
    snprintf(title, sizeof(title), "Prayer Time: %s", prayer_name);
    snprintf(message, sizeof(message), "It's time for %s prayer\nTime: %s",
             prayer_name, time_str);
  } else {
    // Reminder notification
    snprintf(title, sizeof(title), "Prayer Reminder: %s", prayer_name);
    snprintf(message, sizeof(message), "%s prayer in %d minutes\nTime: %s",
             prayer_name, minutes_before, time_str);
  }

  const char *icon = get_icon_path();
  NotifyNotification *n = notify_notification_new(title, message, icon);

  notify_notification_set_timeout(n, 5000);

  // Use different urgency for reminders vs exact time
  NotifyUrgency urgency =
      minutes_before == 0 ? NOTIFY_URGENCY_CRITICAL : NOTIFY_URGENCY_NORMAL;
  notify_notification_set_urgency(n, urgency);

  notify_notification_show(n, NULL);
  g_object_unref(G_OBJECT(n));
}

void notify_cleanup(void) { notify_uninit(); }
