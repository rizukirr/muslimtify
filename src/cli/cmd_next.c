#include "cli_internal.h"
#include "display.h"
#include "prayer_checker.h"
#include "prayertimes.h"
#include <stdio.h>

static int next_name(int argc, char **argv) {
  (void)argc;
  (void)argv;

  PrayerSnapshot snap;
  if (cli_load_snapshot(&snap))
    return 1;

  if (snap.next == PRAYER_NONE) {
    fprintf(stderr, "No upcoming prayers enabled.\n");
    return 1;
  }
  printf("%s\n", prayer_get_name(snap.next));
  return 0;
}

static int next_time(int argc, char **argv) {
  (void)argc;
  (void)argv;

  PrayerSnapshot snap;
  if (cli_load_snapshot(&snap))
    return 1;

  if (snap.next == PRAYER_NONE) {
    fprintf(stderr, "No upcoming prayers enabled.\n");
    return 1;
  }
  char time_str[16];
  format_time_hm(prayer_get_time(&snap.times, snap.next), time_str, sizeof(time_str));
  printf("%s\n", time_str);
  return 0;
}

static int next_remaining(int argc, char **argv) {
  (void)argc;
  (void)argv;

  PrayerSnapshot snap;
  if (cli_load_snapshot(&snap))
    return 1;

  if (snap.next == PRAYER_NONE) {
    fprintf(stderr, "No upcoming prayers enabled.\n");
    return 1;
  }
  int hours = snap.minutes_until / 60;
  int mins = snap.minutes_until % 60;
  if (hours > 0) {
    printf("%d:%02d\n", hours, mins);
  } else {
    printf("%dm\n", mins);
  }
  return 0;
}

static const CommandEntry next_commands[] = {
    {"name", next_name},
    {"time", next_time},
    {"remaining", next_remaining},
};

int handle_next(int argc, char **argv) {
  if (argc > 0) {
    const CommandEntry *sub = dispatch_lookup(next_commands, DISPATCH_N(next_commands), argv[0]);
    if (sub)
      return sub->handler(argc - 1, argv + 1);

    fprintf(stderr, "Error: Unknown next subcommand '%s'\n", argv[0]);
    fprintf(stderr, "Usage: muslimtify next [name|time|remaining]\n");
    return 1;
  }

  PrayerSnapshot snap;
  if (cli_load_snapshot(&snap))
    return 1;

  display_next_prayer(&snap);
  return 0;
}
