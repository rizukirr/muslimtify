#define _GNU_SOURCE
#include "cli.h"
#include "config.h"
#include "display.h"
#include "location.h"
#include "notification.h"
#include "prayer_checker.h"
#include "prayertimes.h"
#include <errno.h>
#include <linux/limits.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// ── dispatch table type ─────────────────────────────────────────────────────

typedef int (*HandlerFn)(int argc, char **argv);

typedef struct {
  const char *name;
  HandlerFn handler;
} CommandEntry;

static const CommandEntry *dispatch_lookup(const CommandEntry *table, int n,
                                           const char *name) {
  for (int i = 0; i < n; i++) {
    if (strcmp(table[i].name, name) == 0)
      return &table[i];
  }
  return NULL;
}

// ── forward declarations ────────────────────────────────────────────────────

static int handle_show(int argc, char **argv);
static int handle_check(int argc, char **argv);
static int handle_next(int argc, char **argv);
static int handle_config(int argc, char **argv);
static int handle_location(int argc, char **argv);
static int handle_enable(int argc, char **argv);
static int handle_disable(int argc, char **argv);
static int handle_list(int argc, char **argv);
static int handle_reminder(int argc, char **argv);
static int handle_daemon(int argc, char **argv);
static int handle_version(int argc, char **argv);
static int handle_help(int argc, char **argv);

// ── top-level dispatch table ────────────────────────────────────────────────

static const CommandEntry top_commands[] = {
    {"show", handle_show},         {"check", handle_check},
    {"next", handle_next},         {"config", handle_config},
    {"location", handle_location}, {"enable", handle_enable},
    {"disable", handle_disable},   {"list", handle_list},
    {"reminder", handle_reminder}, {"daemon", handle_daemon},
    {"version", handle_version},   {"--version", handle_version},
    {"-v", handle_version},        {"help", handle_help},
    {"--help", handle_help},       {"-h", handle_help},
};
static const int top_commands_n =
    (int)(sizeof(top_commands) / sizeof(top_commands[0]));

// ── helpers ─────────────────────────────────────────────────────────────────

static int ensure_location(Config *cfg) {
  if (cfg->auto_detect &&
      (fabs(cfg->latitude) < 1e-6 && fabs(cfg->longitude) < 1e-6)) {
    printf("Detecting location...\n");
    if (location_fetch(cfg) != 0) {
      fprintf(stderr, "Error: Failed to detect location\n");
      return -1;
    }
    printf("✓ Location detected: ");
    if (cfg->city[0] != '\0') {
      printf("%s, %s\n", cfg->city, cfg->country);
    } else {
      printf("%.4f, %.4f\n", cfg->latitude, cfg->longitude);
    }
    config_save(cfg);
  }
  return 0;
}

// ── handlers ────────────────────────────────────────────────────────────────

static int handle_show(int argc, char **argv) {
  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (ensure_location(&cfg) != 0) {
    return 1;
  }

  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: Failed to get current time\n");
    return 1;
  }

  struct PrayerTimes times = calculate_prayer_times(
      tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
      cfg.latitude, cfg.longitude, cfg.timezone_offset);

  bool json_format = false;
  if (argc >= 2 && strcmp(argv[0], "--format") == 0 &&
      strcmp(argv[1], "json") == 0) {
    json_format = true;
  }

  if (json_format) {
    display_prayer_times_json(&times, &cfg, tm_now);
  } else {
    display_prayer_times_table(&times, &cfg, tm_now);
  }

  return 0;
}

static int handle_check(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (ensure_location(&cfg) != 0) {
    return 1;
  }

  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: Failed to get current time\n");
    return 1;
  }

  struct PrayerTimes times = calculate_prayer_times(
      tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
      cfg.latitude, cfg.longitude, cfg.timezone_offset);

  PrayerMatch match = prayer_check_current(&cfg, tm_now, &times);

  if (match.type != PRAYER_NONE) {
    if (!notify_init_once("Muslimtify")) {
      fprintf(stderr, "Error: Failed to initialize notification system\n");
      return 1;
    }

    char time_str[16];
    format_time_hm(match.prayer_time, time_str, sizeof(time_str));
    notify_prayer(prayer_get_name(match.type), time_str, match.minutes_before);
    notify_cleanup();
  }

  return 0;
}

// ── next sub-table ──────────────────────────────────────────────────────────

static int next_name(int argc, char **argv);
static int next_time(int argc, char **argv);
static int next_remaining(int argc, char **argv);

static const CommandEntry next_commands[] = {
    {"name", next_name},
    {"time", next_time},
    {"remaining", next_remaining},
};
static const int next_commands_n =
    (int)(sizeof(next_commands) / sizeof(next_commands[0]));

static int next_name(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }
  if (ensure_location(&cfg) != 0)
    return 1;

  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: Failed to get current time\n");
    return 1;
  }

  struct PrayerTimes times = calculate_prayer_times(
      tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
      cfg.latitude, cfg.longitude, cfg.timezone_offset);

  int minutes_until = 0;
  PrayerType next = prayer_get_next(&cfg, tm_now, &times, &minutes_until);
  if (next == PRAYER_NONE) {
    fprintf(stderr, "No upcoming prayers enabled.\n");
    return 1;
  }
  printf("%s\n", prayer_get_name(next));
  return 0;
}

static int next_time(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }
  if (ensure_location(&cfg) != 0)
    return 1;

  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: Failed to get current time\n");
    return 1;
  }

  struct PrayerTimes times = calculate_prayer_times(
      tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
      cfg.latitude, cfg.longitude, cfg.timezone_offset);

  int minutes_until = 0;
  PrayerType next = prayer_get_next(&cfg, tm_now, &times, &minutes_until);
  if (next == PRAYER_NONE) {
    fprintf(stderr, "No upcoming prayers enabled.\n");
    return 1;
  }
  char time_str[16];
  format_time_hm(prayer_get_time(&times, next), time_str, sizeof(time_str));
  printf("%s\n", time_str);
  return 0;
}

static int next_remaining(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }
  if (ensure_location(&cfg) != 0)
    return 1;

  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: Failed to get current time\n");
    return 1;
  }

  struct PrayerTimes times = calculate_prayer_times(
      tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
      cfg.latitude, cfg.longitude, cfg.timezone_offset);

  int minutes_until = 0;
  PrayerType next = prayer_get_next(&cfg, tm_now, &times, &minutes_until);
  if (next == PRAYER_NONE) {
    fprintf(stderr, "No upcoming prayers enabled.\n");
    return 1;
  }
  int hours = minutes_until / 60;
  int mins = minutes_until % 60;
  if (hours > 0) {
    printf("%d:%02d\n", hours, mins);
  } else {
    printf("%dm\n", mins);
  }
  return 0;
}

static int handle_next(int argc, char **argv) {
  if (argc > 0) {
    const CommandEntry *sub =
        dispatch_lookup(next_commands, next_commands_n, argv[0]);
    if (sub)
      return sub->handler(argc - 1, argv + 1);
  }

  // Default: display next prayer
  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }
  if (ensure_location(&cfg) != 0)
    return 1;

  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: Failed to get current time\n");
    return 1;
  }

  struct PrayerTimes times = calculate_prayer_times(
      tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
      cfg.latitude, cfg.longitude, cfg.timezone_offset);

  display_next_prayer(&times, &cfg, tm_now);
  return 0;
}

// ── config sub-table ────────────────────────────────────────────────────────

static int config_show_handler(int argc, char **argv);
static int config_reset_handler(int argc, char **argv);
static int config_validate_handler(int argc, char **argv);

static const CommandEntry config_commands[] = {
    {"show", config_show_handler},
    {"reset", config_reset_handler},
    {"validate", config_validate_handler},
};
static const int config_commands_n =
    (int)(sizeof(config_commands) / sizeof(config_commands[0]));

static int config_show_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }
  display_config(&cfg);
  return 0;
}

static int config_reset_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg = config_default();
  if (config_save(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to save config\n");
    return 1;
  }
  printf("✓ Configuration reset to defaults\n");
  printf("  Config file: %s\n", config_get_path());
  return 0;
}

static int config_validate_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }
  if (config_validate(&cfg)) {
    printf("✓ Configuration is valid\n");
    return 0;
  } else {
    fprintf(stderr, "✗ Configuration has errors\n");
    return 1;
  }
}

static int handle_config(int argc, char **argv) {
  if (argc > 0) {
    const CommandEntry *sub =
        dispatch_lookup(config_commands, config_commands_n, argv[0]);
    if (sub)
      return sub->handler(argc - 1, argv + 1);

    fprintf(stderr, "Error: Unknown config subcommand '%s'\n", argv[0]);
    return 1;
  }

  fprintf(stderr, "Usage: muslimtify config [show|reset|validate]\n");
  return 1;
}

// ── location sub-table ──────────────────────────────────────────────────────

static int location_show_handler(int argc, char **argv);
static int location_auto_handler(int argc, char **argv);
static int location_set_handler(int argc, char **argv);
static int location_clear_handler(int argc, char **argv);

static const CommandEntry location_commands[] = {
    {"show", location_show_handler},
    {"auto", location_auto_handler},
    {"set", location_set_handler},
    {"clear", location_clear_handler},
};
static const int location_commands_n =
    (int)(sizeof(location_commands) / sizeof(location_commands[0]));

static int location_show_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }
  display_location(&cfg);
  return 0;
}

static int location_auto_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  printf("Detecting location...\n");
  if (location_fetch(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to detect location\n");
    return 1;
  }
  printf("✓ Location detected: ");
  if (cfg.city[0] != '\0') {
    printf("%s, %s\n", cfg.city, cfg.country);
  } else {
    printf("%.4f, %.4f\n", cfg.latitude, cfg.longitude);
  }
  printf("  Timezone: %s (UTC%+.1f)\n", cfg.timezone, cfg.timezone_offset);

  cfg.auto_detect = true;
  if (config_save(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to save config\n");
    return 1;
  }
  printf("✓ Saved to config\n");
  return 0;
}

static int location_set_handler(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: muslimtify location set <latitude> <longitude>\n");
    return 1;
  }

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  char *end_lat, *end_lon;
  errno = 0;
  cfg.latitude = strtod(argv[0], &end_lat);
  if (end_lat == argv[0] || *end_lat != '\0' || errno == ERANGE) {
    fprintf(stderr, "Error: Invalid latitude '%s'\n", argv[0]);
    return 1;
  }
  errno = 0;
  cfg.longitude = strtod(argv[1], &end_lon);
  if (end_lon == argv[1] || *end_lon != '\0' || errno == ERANGE) {
    fprintf(stderr, "Error: Invalid longitude '%s'\n", argv[1]);
    return 1;
  }
  cfg.auto_detect = false;

  if (config_save(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to save config\n");
    return 1;
  }

  printf("✓ Location set to: %.4f, %.4f\n", cfg.latitude, cfg.longitude);
  return 0;
}

static int location_clear_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  cfg.latitude = 0.0;
  cfg.longitude = 0.0;
  cfg.auto_detect = true;
  cfg.city[0] = '\0';
  cfg.country[0] = '\0';

  if (config_save(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to save config\n");
    return 1;
  }

  printf("✓ Location cleared. Will auto-detect on next run.\n");
  return 0;
}

static int handle_location(int argc, char **argv) {
  if (argc > 0) {
    const CommandEntry *sub =
        dispatch_lookup(location_commands, location_commands_n, argv[0]);
    if (sub)
      return sub->handler(argc - 1, argv + 1);

    fprintf(stderr, "Error: Unknown location subcommand '%s'\n", argv[0]);
    return 1;
  }

  // Default: show
  return location_show_handler(0, NULL);
}

// ── enable / disable / list ─────────────────────────────────────────────────

static int handle_enable(int argc, char **argv) {
  if (argc < 1) {
    fprintf(stderr, "Usage: muslimtify enable <prayer>\n");
    fprintf(stderr, "       muslimtify enable all\n");
    return 1;
  }

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (strcmp(argv[0], "all") == 0) {
    cfg.fajr.enabled = true;
    cfg.sunrise.enabled = true;
    cfg.dhuha.enabled = true;
    cfg.dhuhr.enabled = true;
    cfg.asr.enabled = true;
    cfg.maghrib.enabled = true;
    cfg.isha.enabled = true;

    if (config_save(&cfg) != 0) {
      fprintf(stderr, "Error: Failed to save config\n");
      return 1;
    }

    printf("✓ All prayers enabled\n");
    return 0;
  }

  PrayerConfig *prayer = config_get_prayer(&cfg, argv[0]);
  if (!prayer) {
    fprintf(stderr, "Error: Unknown prayer '%s'\n", argv[0]);
    return 1;
  }

  prayer->enabled = true;

  if (config_save(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to save config\n");
    return 1;
  }

  printf("✓ %s notifications enabled\n", argv[0]);
  return 0;
}

static int handle_disable(int argc, char **argv) {
  if (argc < 1) {
    fprintf(stderr, "Usage: muslimtify disable <prayer>\n");
    fprintf(stderr, "       muslimtify disable all\n");
    return 1;
  }

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (strcmp(argv[0], "all") == 0) {
    cfg.fajr.enabled = false;
    cfg.sunrise.enabled = false;
    cfg.dhuha.enabled = false;
    cfg.dhuhr.enabled = false;
    cfg.asr.enabled = false;
    cfg.maghrib.enabled = false;
    cfg.isha.enabled = false;

    if (config_save(&cfg) != 0) {
      fprintf(stderr, "Error: Failed to save config\n");
      return 1;
    }

    printf("✓ All prayers disabled\n");
    return 0;
  }

  PrayerConfig *prayer = config_get_prayer(&cfg, argv[0]);
  if (!prayer) {
    fprintf(stderr, "Error: Unknown prayer '%s'\n", argv[0]);
    return 1;
  }

  prayer->enabled = false;

  if (config_save(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to save config\n");
    return 1;
  }

  printf("✓ %s notifications disabled\n", argv[0]);
  return 0;
}

static int handle_list(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  display_prayer_list(&cfg);
  return 0;
}

// ── reminder ────────────────────────────────────────────────────────────────

static int handle_reminder(int argc, char **argv) {
  // "muslimtify reminder" or "muslimtify reminder show"
  if (argc == 0 || strcmp(argv[0], "show") == 0) {
    Config cfg;
    if (config_load(&cfg) != 0) {
      fprintf(stderr, "Error: Failed to load config\n");
      return 1;
    }
    display_reminders(&cfg);
    return 0;
  }

  // Need at least: <prayer> <times>
  if (argc < 2) {
    fprintf(stderr, "Usage: muslimtify reminder <prayer> <time1,time2,...>\n");
    fprintf(stderr, "       muslimtify reminder <prayer> clear\n");
    fprintf(stderr, "       muslimtify reminder all <time1,time2,...>\n");
    fprintf(stderr, "       muslimtify reminder show\n");
    return 1;
  }

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  const char *prayer_name = argv[0];
  const char *reminder_str = argv[1];

  if (strcmp(prayer_name, "all") == 0) {
    PrayerConfig *prayers[] = {&cfg.fajr, &cfg.sunrise, &cfg.dhuha, &cfg.dhuhr,
                               &cfg.asr, &cfg.maghrib, &cfg.isha};

    int reminders[MAX_REMINDERS];
    int count = config_parse_reminders(reminder_str, reminders, MAX_REMINDERS);

    if (count < 0) {
      fprintf(stderr, "Error: Invalid reminder format\n");
      return 1;
    }

    for (int i = 0; i < 7; i++) {
      if (!prayers[i]->enabled)
        continue;

      prayers[i]->reminder_count = count;
      for (int j = 0; j < count; j++) {
        prayers[i]->reminders[j] = reminders[j];
      }
    }

    if (config_save(&cfg) != 0) {
      fprintf(stderr, "Error: Failed to save config\n");
      return 1;
    }

    if (count == 0) {
      printf("✓ Reminders cleared for all enabled prayers\n");
    } else {
      printf("✓ Reminders updated for all enabled prayers: %s\n",
             reminder_str);
    }
    return 0;
  }

  PrayerConfig *prayer = config_get_prayer(&cfg, prayer_name);
  if (!prayer) {
    fprintf(stderr, "Error: Unknown prayer '%s'\n", prayer_name);
    return 1;
  }

  int reminders[MAX_REMINDERS];
  int count = config_parse_reminders(reminder_str, reminders, MAX_REMINDERS);

  if (count < 0) {
    fprintf(stderr, "Error: Invalid reminder format\n");
    return 1;
  }

  prayer->reminder_count = count;
  for (int i = 0; i < count; i++) {
    prayer->reminders[i] = reminders[i];
  }

  if (config_save(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to save config\n");
    return 1;
  }

  if (count == 0) {
    printf("✓ Reminders cleared for %s\n", prayer_name);
  } else {
    printf("✓ Set %d reminder(s) for %s: %s\n", count, prayer_name,
           reminder_str);
  }

  return 0;
}

// ── daemon helpers ──────────────────────────────────────────────────────────

static int systemctl_user(const char *const *args) {
  int n = 0;
  while (args[n])
    n++;

  char **child_argv = malloc((size_t)(n + 3) * sizeof(char *));
  if (!child_argv)
    return 1;

  child_argv[0] = "systemctl";
  child_argv[1] = "--user";
  for (int i = 0; i < n; i++)
    child_argv[i + 2] = (char *)args[i];
  child_argv[n + 2] = NULL;

  pid_t pid = fork();
  if (pid < 0) {
    free(child_argv);
    return 1;
  }
  if (pid == 0) {
    execvp("systemctl", child_argv);
    _exit(127);
  }
  free(child_argv);
  int wstatus;
  waitpid(pid, &wstatus, 0);
  return WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
}

static void mkdir_p(const char *path) {
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s", path);
  tmp[sizeof(tmp) - 1] = '\0';
  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      mkdir(tmp, 0755);
      *p = '/';
    }
  }
  mkdir(tmp, 0755);
}

static const char *get_home(void) {
  const char *home = getenv("HOME");
  if (!home) {
    struct passwd *pw = getpwuid(getuid());
    if (pw)
      home = pw->pw_dir;
  }
  return home;
}

// ── daemon sub-table ────────────────────────────────────────────────────────

static int daemon_install_handler(int argc, char **argv);
static int daemon_uninstall_handler(int argc, char **argv);
static int daemon_status_handler(int argc, char **argv);

static const CommandEntry daemon_commands[] = {
    {"install", daemon_install_handler},
    {"uninstall", daemon_uninstall_handler},
    {"status", daemon_status_handler},
};
static const int daemon_commands_n =
    (int)(sizeof(daemon_commands) / sizeof(daemon_commands[0]));

static int daemon_install_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  char binary_path[PATH_MAX];
  ssize_t len =
      readlink("/proc/self/exe", binary_path, sizeof(binary_path) - 1);
  if (len <= 0) {
    fprintf(stderr, "Error: Cannot determine binary path\n");
    return 1;
  }
  binary_path[len] = '\0';

  const char *home = get_home();
  if (!home) {
    fprintf(stderr, "Error: Cannot determine home directory\n");
    return 1;
  }

  char systemd_dir[PATH_MAX];
  snprintf(systemd_dir, sizeof(systemd_dir), "%s/.config/systemd/user", home);
  mkdir_p(systemd_dir);

  char service_path[PATH_MAX + 32];
  snprintf(service_path, sizeof(service_path), "%s/muslimtify.service",
           systemd_dir);
  FILE *f = fopen(service_path, "w");
  if (!f) {
    fprintf(stderr, "Error: Cannot write %s: %s\n", service_path,
            strerror(errno));
    return 1;
  }
  fprintf(f,
          "[Unit]\n"
          "Description=Prayer Time Notification Check\n"
          "After=network-online.target\n"
          "\n"
          "[Service]\n"
          "Type=oneshot\n"
          "ExecStart=%s check\n"
          "StandardOutput=journal\n"
          "StandardError=journal\n",
          binary_path);
  fclose(f);

  char timer_path[PATH_MAX + 32];
  snprintf(timer_path, sizeof(timer_path), "%s/muslimtify.timer", systemd_dir);
  f = fopen(timer_path, "w");
  if (!f) {
    fprintf(stderr, "Error: Cannot write %s: %s\n", timer_path,
            strerror(errno));
    return 1;
  }
  fprintf(f, "[Unit]\n"
             "Description=Check prayer times every minute\n"
             "After=network-online.target\n"
             "\n"
             "[Timer]\n"
             "OnCalendar=*:*:00\n"
             "Persistent=true\n"
             "AccuracySec=1s\n"
             "\n"
             "[Install]\n"
             "WantedBy=timers.target\n");
  fclose(f);

  printf("✓ Created %s\n", service_path);
  printf("✓ Created %s\n", timer_path);

  if (systemctl_user((const char *[]){"daemon-reload", NULL}) != 0) {
    fprintf(stderr, "Error: systemctl daemon-reload failed\n");
    return 1;
  }
  printf("✓ Reloaded systemd\n");

  if (systemctl_user((const char *[]){"enable", "muslimtify.timer", NULL}) !=
      0) {
    fprintf(stderr, "Error: Failed to enable muslimtify.timer\n");
    return 1;
  }
  printf("✓ Enabled muslimtify.timer\n");

  if (systemctl_user((const char *[]){"start", "muslimtify.timer", NULL}) !=
      0) {
    fprintf(stderr, "Error: Failed to start muslimtify.timer\n");
    return 1;
  }
  printf("✓ Started muslimtify.timer\n");

  printf("\nDaemon installed. Binary: %s\n", binary_path);
  printf("Run 'muslimtify daemon status' to verify.\n");
  return 0;
}

static int daemon_uninstall_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (systemctl_user((const char *[]){"is-active", "--quiet",
                                      "muslimtify.timer", NULL}) == 0) {
    systemctl_user((const char *[]){"stop", "muslimtify.timer", NULL});
    printf("✓ Stopped muslimtify.timer\n");
  }

  if (systemctl_user((const char *[]){"is-enabled", "--quiet",
                                      "muslimtify.timer", NULL}) == 0) {
    systemctl_user((const char *[]){"disable", "muslimtify.timer", NULL});
    printf("✓ Disabled muslimtify.timer\n");
  }

  const char *home = get_home();
  if (!home) {
    fprintf(stderr, "Error: Cannot determine home directory\n");
    return 1;
  }

  char path[PATH_MAX];
  int removed = 0;
  for (int i = 0; i < 2; i++) {
    snprintf(path, sizeof(path), "%s/.config/systemd/user/%s", home,
             i == 0 ? "muslimtify.service" : "muslimtify.timer");
    if (remove(path) == 0) {
      printf("✓ Removed %s\n", path);
      removed++;
    }
  }

  systemctl_user((const char *[]){"daemon-reload", NULL});
  printf("✓ Reloaded systemd\n");

  if (removed == 0)
    printf("Note: No unit files were found (already uninstalled?)\n");

  printf("\nDaemon uninstalled. Binary and config files are untouched.\n");
  printf("To fully remove: sudo ./uninstall.sh\n");
  return 0;
}

static int daemon_status_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("=== Timer ===\n");
  systemctl_user(
      (const char *[]){"status", "muslimtify.timer", "--no-pager", NULL});

  printf("\n=== Next trigger ===\n");
  systemctl_user(
      (const char *[]){"list-timers", "muslimtify", "--no-pager", NULL});
  return 0;
}

static int handle_daemon(int argc, char **argv) {
  if (argc > 0) {
    const CommandEntry *sub =
        dispatch_lookup(daemon_commands, daemon_commands_n, argv[0]);
    if (sub)
      return sub->handler(argc - 1, argv + 1);

    fprintf(stderr, "Usage: muslimtify daemon [install|uninstall|status]\n");
    return 1;
  }

  // Default: status
  return daemon_status_handler(0, NULL);
}

// ── version / help ──────────────────────────────────────────────────────────

static int handle_version(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("Muslimtify v0.1.2\n");
  printf("Prayer Time Notification Daemon\n\n");
  printf("Build: %s %s\n", __DATE__, __TIME__);
  printf("Method: Kemenag Indonesia\n");
  printf("Location: Auto-detect (ipinfo.io)\n");

  return 0;
}

static int handle_help(int argc, char **argv) {
  (void)argc;
  (void)argv;

  cli_print_help();
  return 0;
}

// ── public API ──────────────────────────────────────────────────────────────

void cli_print_help(void) {
  printf("Muslimtify - Prayer Time Notification Daemon\n\n");
  printf("Usage: muslimtify [COMMAND] [OPTIONS]\n\n");
  printf("Commands:\n");
  printf("  show              Display today's prayer times (default)\n");
  printf("  check             Check and send notification if prayer time\n");
  printf("  next              Show time until next prayer\n");
  printf("  next name         Print next prayer name only (e.g. Ashr)\n");
  printf("  next time         Print next prayer time only (e.g. 12:05)\n");
  printf("  next remaining    Print time remaining only (e.g. 1:23 or 23m)\n");
  printf("  config            Manage configuration\n");
  printf("  location          Manage location settings\n");
  printf("  enable <prayer>   Enable prayer notification\n");
  printf("  disable <prayer>  Disable prayer notification\n");
  printf("  list              List prayer notification status\n");
  printf("  reminder          Manage prayer reminders\n");
  printf("  daemon            Manage systemd daemon "
         "[install|uninstall|status]\n");
  printf("  version           Show version information\n");
  printf("  help              Show this help message\n\n");
  printf("Examples:\n");
  printf("  muslimtify                    # Show today's prayer times\n");
  printf("  muslimtify next               # Show next prayer\n");
  printf("  muslimtify location auto      # Auto-detect location\n");
  printf("  muslimtify enable fajr        # Enable Fajr notifications\n");
  printf("  muslimtify reminder fajr 30,15,5  # Set Fajr reminders\n\n");
  printf("Config file: %s\n", config_get_path());
}

int cli_run(int argc, char **argv) {
  // No command → default to "show"
  if (argc < 2)
    return handle_show(0, NULL);

  const char *cmd = argv[1];
  const CommandEntry *entry =
      dispatch_lookup(top_commands, top_commands_n, cmd);
  if (entry)
    return entry->handler(argc - 2, argv + 2);

  fprintf(stderr, "Unknown command '%s'. Use 'muslimtify help' for usage.\n",
          cmd);
  return 1;
}
