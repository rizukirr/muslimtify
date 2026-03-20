#define _GNU_SOURCE
#include "cli.h"
#include "cli_internal.h"
#include "config.h"
#include "location.h"
#include "version.h"
#include <math.h>
#include <stdio.h>

// ── shared helper ───────────────────────────────────────────────────────────

int ensure_location(Config *cfg) {
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

// ── version / help ──────────────────────────────────────────────────────────

int handle_version(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("Muslimtify v%s\n", MUSLIMTIFY_VERSION);
  printf("Prayer Time Notification Daemon\n\n");
  printf("Build: %s %s\n", __DATE__, __TIME__);
  printf("Method: Kemenag Indonesia\n");
  printf("Location: Auto-detect (ipinfo.io)\n");

  return 0;
}

int handle_help(int argc, char **argv) {
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
  printf("  show              Display today's prayer times\n");
  printf("    --no-header       Plain key=value output (enabled prayers only)\n");
  printf("    --format json     JSON output\n");
  printf("  check             Check and send notification if prayer time\n");
  printf("  next              Show time until next prayer\n");
  printf("  next name         Print next prayer name only (e.g. Ashr)\n");
  printf("  next time         Print next prayer time only (e.g. 12:05)\n");
  printf("  next remaining    Print time remaining only (e.g. 1:23 or 23m)\n");
  printf("  config            Manage configuration\n");
  printf("  location          Manage location settings "
         "[show|auto|set|clear|refresh]\n");
  printf("  enable <prayer>   Enable prayer notification\n");
  printf("  disable <prayer>  Disable prayer notification\n");
  printf("  list              List prayer notification status\n");
  printf("  reminder          Manage prayer reminders\n");
  printf("  daemon            Manage systemd daemon "
         "[install|uninstall|status]\n");
  printf("  version           Show version information\n");
  printf("  help              Show this help message\n\n");
  printf("Examples:\n");
  printf("  muslimtify                    # Show version and help\n");
  printf("  muslimtify next               # Show next prayer\n");
  printf("  muslimtify location auto      # Auto-detect location\n");
  printf("  muslimtify enable fajr        # Enable Fajr notifications\n");
  printf("  muslimtify reminder fajr 30,15,5  # Set Fajr reminders\n\n");
  printf("Config file: %s\n", config_get_path());
}

int cli_run(int argc, char **argv) {
  if (argc < 2) {
    handle_version(0, NULL);
    printf("\n");
    cli_print_help();
    return 0;
  }

  const char *cmd = argv[1];
  const CommandEntry *entry =
      dispatch_lookup(top_commands, DISPATCH_N(top_commands), cmd);
  if (entry)
    return entry->handler(argc - 2, argv + 2);

  fprintf(stderr, "Unknown command '%s'. Use 'muslimtify help' for usage.\n",
          cmd);
  return 1;
}
