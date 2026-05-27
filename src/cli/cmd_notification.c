#include "cli_internal.h"
#include "notification.h"
#include "prayer_checker.h"
#include "prayertimes.h"
#include <stdio.h>

static int notification_test(int argc, char **argv) {
  (void)argc;
  (void)argv;

  PrayerSnapshot snap;
  if (cli_load_snapshot(&snap))
    return 1;

  if (snap.next == PRAYER_NONE) {
    fprintf(stderr, "No upcoming prayers enabled.\n");
    return 1;
  }

  if (!notify_init_once("Muslimtify")) {
    fprintf(stderr, "Error: Failed to initialize notification system\n");
    return 1;
  }

  char time_str[16];
  format_time_hm(prayer_get_time(&snap.times, snap.next), time_str, sizeof(time_str));
  const char *sound_preset =
      snap.config.notification_sound ? snap.config.notification_sound_alarm : NULL;
  notify_prayer(prayer_get_name(snap.next), time_str, 0, snap.config.notification_urgency,
                sound_preset);
  notify_cleanup();

  printf("Sent test notification for %s at %s\n", prayer_get_name(snap.next), time_str);
  return 0;
}

static const CommandEntry notification_commands[] = {
    {"test", notification_test},
};

int handle_notification(int argc, char **argv) {
  if (argc > 0) {
    const CommandEntry *sub =
        dispatch_lookup(notification_commands, DISPATCH_N(notification_commands), argv[0]);
    if (sub)
      return sub->handler(argc - 1, argv + 1);

    fprintf(stderr, "Error: Unknown notification subcommand '%s'\n", argv[0]);
  }

  fprintf(stderr, "Usage: muslimtify notification test\n");
  return 1;
}
