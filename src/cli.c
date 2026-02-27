#define _GNU_SOURCE
#include "../include/cli.h"
#include "../include/config.h"
#include "../include/display.h"
#include "../include/location.h"
#include "../include/notification.h"
#include "../include/prayer_checker.h"
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

// Forward declarations
static int cli_handle_show(const CliArgs *args);
static int cli_handle_daemon(const CliArgs *args);
static int cli_handle_check(const CliArgs *args);
static int cli_handle_next(const CliArgs *args);
static int cli_handle_config(const CliArgs *args);
static int cli_handle_location(const CliArgs *args);
static int cli_handle_enable(const CliArgs *args);
static int cli_handle_disable(const CliArgs *args);
static int cli_handle_list(const CliArgs *args);
static int cli_handle_reminder(const CliArgs *args);
static int cli_handle_version(const CliArgs *args);

CliArgs cli_parse(int argc, char **argv) {
  CliArgs args = {0};

  // Default command is show (just "muslimtify")
  if (argc < 2) {
    args.command = CMD_SHOW;
    return args;
  }

  const char *cmd = argv[1];

  // Parse command
  if (strcmp(cmd, "show") == 0)
    args.command = CMD_SHOW;
  else if (strcmp(cmd, "check") == 0)
    args.command = CMD_CHECK;
  else if (strcmp(cmd, "next") == 0)
    args.command = CMD_NEXT;
  else if (strcmp(cmd, "config") == 0)
    args.command = CMD_CONFIG;
  else if (strcmp(cmd, "location") == 0)
    args.command = CMD_LOCATION;
  else if (strcmp(cmd, "enable") == 0)
    args.command = CMD_ENABLE;
  else if (strcmp(cmd, "disable") == 0)
    args.command = CMD_DISABLE;
  else if (strcmp(cmd, "list") == 0)
    args.command = CMD_LIST;
  else if (strcmp(cmd, "reminder") == 0)
    args.command = CMD_REMINDER;
  else if (strcmp(cmd, "daemon") == 0)
    args.command = CMD_DAEMON;
  else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 ||
           strcmp(cmd, "-v") == 0)
    args.command = CMD_VERSION;
  else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 ||
           strcmp(cmd, "-h") == 0)
    args.command = CMD_HELP;
  else {
    args.command = CMD_UNKNOWN;
    return args;
  }

  // Parse subcommand and remaining args
  if (argc > 2) {
    args.subcommand = argv[2];
    args.argc = argc - 3;
    args.argv = argv + 3;
  }

  return args;
}

int cli_execute(const CliArgs *args) {
  switch (args->command) {
  case CMD_SHOW:
    return cli_handle_show(args);
  case CMD_CHECK:
    return cli_handle_check(args);
  case CMD_NEXT:
    return cli_handle_next(args);
  case CMD_CONFIG:
    return cli_handle_config(args);
  case CMD_LOCATION:
    return cli_handle_location(args);
  case CMD_ENABLE:
    return cli_handle_enable(args);
  case CMD_DISABLE:
    return cli_handle_disable(args);
  case CMD_LIST:
    return cli_handle_list(args);
  case CMD_REMINDER:
    return cli_handle_reminder(args);
  case CMD_VERSION:
    return cli_handle_version(args);
  case CMD_HELP:
    cli_print_help(args->subcommand);
    return 0;
  case CMD_DAEMON:
    return cli_handle_daemon(args);
  default:
    fprintf(stderr, "Unknown command. Use 'muslimtify help' for usage.\n");
    return 1;
  }
}

static int ensure_location(Config *cfg) {
  // Check if we need to auto-detect location
  if (cfg->auto_detect && (fabs(cfg->latitude) < 1e-6 && fabs(cfg->longitude) < 1e-6)) {
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

static int cli_handle_show(const CliArgs *args) {
  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (ensure_location(&cfg) != 0) {
    return 1;
  }

  // Get current date
  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: Failed to get current time\n");
    return 1;
  }

  // Calculate prayer times
  struct PrayerTimes times = calculate_prayer_times(
      tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, cfg.latitude,
      cfg.longitude, cfg.timezone_offset);

  // Check for --format flag
  bool json_format = false;
  if (args->subcommand && strcmp(args->subcommand, "--format") == 0) {
    if (args->argc > 0 && strcmp(args->argv[0], "json") == 0) {
      json_format = true;
    }
  }

  if (json_format) {
    display_prayer_times_json(&times, &cfg, tm_now);
  } else {
    display_prayer_times_table(&times, &cfg, tm_now);
  }

  return 0;
}

static int cli_handle_check(const CliArgs *args) {
  (void)args; // Unused

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (ensure_location(&cfg) != 0) {
    return 1;
  }

  // Get current time
  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: Failed to get current time\n");
    return 1;
  }

  // Calculate prayer times
  struct PrayerTimes times = calculate_prayer_times(
      tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, cfg.latitude,
      cfg.longitude, cfg.timezone_offset);

  // Check if it's prayer time or reminder
  PrayerMatch match = prayer_check_current(&cfg, tm_now, &times);

  if (match.type != PRAYER_NONE) {
    // Initialize notification system
    if (!notify_init_once("Muslimtify")) {
      fprintf(stderr, "Error: Failed to initialize notification system\n");
      return 1;
    }

    // Format prayer time
    char time_str[16];
    format_time_hm(match.prayer_time, time_str, sizeof(time_str));

    // Send notification
    notify_prayer(prayer_get_name(match.type), time_str, match.minutes_before);

    notify_cleanup();

    return 0;
  }

  // No match - exit silently (normal for cron/systemd)
  return 0;
}

static int cli_handle_next(const CliArgs *args) {
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
      tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, cfg.latitude,
      cfg.longitude, cfg.timezone_offset);

  if (args->subcommand && strcmp(args->subcommand, "name") == 0) {
    int minutes_until = 0;
    PrayerType next = prayer_get_next(&cfg, tm_now, &times, &minutes_until);
    if (next == PRAYER_NONE) {
      fprintf(stderr, "No upcoming prayers enabled.\n");
      return 1;
    }
    printf("%s\n", prayer_get_name(next));
    return 0;
  }

  if (args->subcommand && strcmp(args->subcommand, "time") == 0) {
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

  if (args->subcommand && strcmp(args->subcommand, "remaining") == 0) {
    int minutes_until = 0;
    PrayerType next = prayer_get_next(&cfg, tm_now, &times, &minutes_until);
    if (next == PRAYER_NONE) {
      fprintf(stderr, "No upcoming prayers enabled.\n");
      return 1;
    }
    int hours = minutes_until / 60;
    int mins  = minutes_until % 60;
    if (hours > 0) {
      printf("%d:%02d\n", hours, mins);
    } else {
      printf("%dm\n", mins);
    }
    return 0;
  }

  display_next_prayer(&times, &cfg, tm_now);

  return 0;
}

static int cli_handle_config(const CliArgs *args) {
  if (!args->subcommand) {
    fprintf(stderr, "Usage: muslimtify config [show|get|set|reset|validate]\n");
    return 1;
  }

  Config cfg;

  if (strcmp(args->subcommand, "show") == 0) {
    if (config_load(&cfg) != 0) {
      fprintf(stderr, "Error: Failed to load config\n");
      return 1;
    }
    display_config(&cfg);
    return 0;
  }

  if (strcmp(args->subcommand, "reset") == 0) {
    cfg = config_default();
    if (config_save(&cfg) != 0) {
      fprintf(stderr, "Error: Failed to save config\n");
      return 1;
    }
    printf("✓ Configuration reset to defaults\n");
    printf("  Config file: %s\n", config_get_path());
    return 0;
  }

  if (strcmp(args->subcommand, "validate") == 0) {
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

  fprintf(stderr, "Error: Unknown config subcommand '%s'\n", args->subcommand);
  return 1;
}

static int cli_handle_location(const CliArgs *args) {
  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (!args->subcommand || strcmp(args->subcommand, "show") == 0) {
    display_location(&cfg);
    return 0;
  }

  if (strcmp(args->subcommand, "auto") == 0) {
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

  if (strcmp(args->subcommand, "set") == 0) {
    if (args->argc < 2) {
      fprintf(stderr,
              "Usage: muslimtify location set <latitude> <longitude>\n");
      return 1;
    }

    char *end_lat, *end_lon;
    errno = 0;
    cfg.latitude = strtod(args->argv[0], &end_lat);
    if (end_lat == args->argv[0] || *end_lat != '\0' || errno == ERANGE) {
      fprintf(stderr, "Error: Invalid latitude '%s'\n", args->argv[0]);
      return 1;
    }
    errno = 0;
    cfg.longitude = strtod(args->argv[1], &end_lon);
    if (end_lon == args->argv[1] || *end_lon != '\0' || errno == ERANGE) {
      fprintf(stderr, "Error: Invalid longitude '%s'\n", args->argv[1]);
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

  if (strcmp(args->subcommand, "clear") == 0) {
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

  fprintf(stderr, "Error: Unknown location subcommand '%s'\n",
          args->subcommand);
  return 1;
}

static int cli_handle_enable(const CliArgs *args) {
  if (!args->subcommand) {
    fprintf(stderr, "Usage: muslimtify enable <prayer>\n");
    fprintf(stderr, "       muslimtify enable all\n");
    return 1;
  }

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (strcmp(args->subcommand, "all") == 0) {
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

  PrayerConfig *prayer = config_get_prayer(&cfg, args->subcommand);
  if (!prayer) {
    fprintf(stderr, "Error: Unknown prayer '%s'\n", args->subcommand);
    return 1;
  }

  prayer->enabled = true;

  if (config_save(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to save config\n");
    return 1;
  }

  printf("✓ %s notifications enabled\n", args->subcommand);
  return 0;
}

static int cli_handle_disable(const CliArgs *args) {
  if (!args->subcommand) {
    fprintf(stderr, "Usage: muslimtify disable <prayer>\n");
    fprintf(stderr, "       muslimtify disable all\n");
    return 1;
  }

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (strcmp(args->subcommand, "all") == 0) {
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

  PrayerConfig *prayer = config_get_prayer(&cfg, args->subcommand);
  if (!prayer) {
    fprintf(stderr, "Error: Unknown prayer '%s'\n", args->subcommand);
    return 1;
  }

  prayer->enabled = false;

  if (config_save(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to save config\n");
    return 1;
  }

  printf("✓ %s notifications disabled\n", args->subcommand);
  return 0;
}

static int cli_handle_list(const CliArgs *args) {
  (void)args; // Unused

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  display_prayer_list(&cfg);
  return 0;
}

static int cli_handle_reminder(const CliArgs *args) {
  if (!args->subcommand || strcmp(args->subcommand, "show") == 0) {
    Config cfg;
    if (config_load(&cfg) != 0) {
      fprintf(stderr, "Error: Failed to load config\n");
      return 1;
    }
    display_reminders(&cfg);
    return 0;
  }

  if (args->argc < 1) {
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

  const char *prayer_name = args->subcommand;
  const char *reminder_str = args->argv[0];

  // Handle "all" keyword
  if (strcmp(prayer_name, "all") == 0) {
    PrayerConfig *prayers[] = {&cfg.fajr, &cfg.sunrise, &cfg.dhuha, &cfg.dhuhr,
                               &cfg.asr,  &cfg.maghrib, &cfg.isha};

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
      printf("✓ Reminders updated for all enabled prayers: %s\n", reminder_str);
    }
    return 0;
  }

  // Set for specific prayer
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

// ── daemon helpers
// ────────────────────────────────────────────────────────────

// Run: systemctl --user <args...>  (NULL-terminated)
// Inherits stdout/stderr so output goes straight to the terminal.
static int systemctl_user(const char *const *args) {
  int n = 0;
  while (args[n])
    n++;

  char **argv = malloc((size_t)(n + 3) * sizeof(char *));
  if (!argv)
    return 1;

  argv[0] = "systemctl";
  argv[1] = "--user";
  for (int i = 0; i < n; i++)
    argv[i + 2] = (char *)args[i];
  argv[n + 2] = NULL;

  pid_t pid = fork();
  if (pid < 0) {
    free(argv);
    return 1;
  }
  if (pid == 0) {
    execvp("systemctl", argv);
    _exit(127);
  }
  free(argv);
  int wstatus;
  waitpid(pid, &wstatus, 0);
  return WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
}

// mkdir -p for a path (silently ignores existing directories)
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

// ── daemon subcommands
// ────────────────────────────────────────────────────────

static int daemon_install(void) {
  // Resolve the path of the running binary
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

  // Write service file
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

  // Write timer file
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

  // Reload, enable, start
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

static int daemon_uninstall(void) {
  // Stop (ignore error — may not be running)
  if (systemctl_user((const char *[]){"is-active", "--quiet",
                                      "muslimtify.timer", NULL}) == 0) {
    systemctl_user((const char *[]){"stop", "muslimtify.timer", NULL});
    printf("✓ Stopped muslimtify.timer\n");
  }

  // Disable (ignore error — may not be enabled)
  if (systemctl_user((const char *[]){"is-enabled", "--quiet",
                                      "muslimtify.timer", NULL}) == 0) {
    systemctl_user((const char *[]){"disable", "muslimtify.timer", NULL});
    printf("✓ Disabled muslimtify.timer\n");
  }

  // Remove unit files
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

static int daemon_status(void) {
  // Show timer status
  printf("=== Timer ===\n");
  systemctl_user(
      (const char *[]){"status", "muslimtify.timer", "--no-pager", NULL});

  // Show next trigger
  printf("\n=== Next trigger ===\n");
  systemctl_user(
      (const char *[]){"list-timers", "muslimtify", "--no-pager", NULL});
  return 0;
}

static int cli_handle_daemon(const CliArgs *args) {
  if (!args->subcommand || strcmp(args->subcommand, "status") == 0)
    return daemon_status();
  if (strcmp(args->subcommand, "install") == 0)
    return daemon_install();
  if (strcmp(args->subcommand, "uninstall") == 0)
    return daemon_uninstall();

  fprintf(stderr, "Usage: muslimtify daemon [install|uninstall|status]\n");
  return 1;
}

// ─────────────────────────────────────────────────────────────────────────────

static int cli_handle_version(const CliArgs *args) {
  (void)args; // Unused

  printf("Muslimtify v0.1.1\n");
  printf("Prayer Time Notification Daemon\n\n");
  printf("Build: %s %s\n", __DATE__, __TIME__);
  printf("Method: Kemenag Indonesia\n");
  printf("Location: Auto-detect (ipinfo.io)\n");

  return 0;
}

void cli_print_help(const char *command) {
  if (!command) {
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
    return;
  }

  // Command-specific help can be added here
  printf("No detailed help available for '%s'\n", command);
}
