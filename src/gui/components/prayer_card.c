#include "components/prayer_card.h"

#include "app/assets.h"
#include "ccompose.h"
#include "platform.h"
#include "prayer_checker.h"
#include "prayertimes.h"
#include "themes/colors.h"
#include "themes/fonts.h"
#include "utils/gui_prayer.h"
#include <raylib.h>
#include <time.h>

void PrayerCard(void) {
  Assets *a = appAssets();
  const PrayerSnapshot *snap = guiGetPrayer();
  time_t now = time(NULL);
  struct tm tm_buf;
  platform_localtime(&now, &tm_buf);

  const char *clock = TextFormat("%02d:%02d:%02d", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);

  const char *nextName = (snap->next == PRAYER_NONE) ? "No prayer" : prayer_get_name(snap->next);

  int hours = snap->minutes_until / 60;
  int mins = snap->minutes_until % 60;
  const char *remaining =
      (hours > 0) ? TextFormat("%02d:%02d", hours, mins) : TextFormat("%02d minutes", mins);

  char startStr[16] = "--:--";
  if (snap->next != PRAYER_NONE) {
    format_time_hm(prayer_get_time(&snap->times, snap->next), startStr, sizeof(startStr));
  }

  Column("PrayerCard",
         .layout = {.sizing = {.height = Fit(), .width = Grow()},
                    .padding = PadAll(48),
                    .childGap = 16},
         .cornerRadius = RadiusAll(16), .backgroundColor = COLOR_PRIMARY) {
    Text("UPCOMING PRAYER", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_MEDIUM);
    Text(nextName, .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_DISPLAY_LARGE,
         .fontId = a->fontManrope);
    VSpacer();
    Row("PrayerCardButtons", .layout = {.sizing = {.width = Grow()},
                                        .childGap = 16,
                                        .childAlignment = {.y = CC_ALIGN_Y_BOTTOM}}) {
      Column("startAt", .layout = {.sizing = {.height = Fit()}}) {
        Text("Starts at", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_MEDIUM);
        Text(startStr, .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_HEADLINE_LARGE,
             .fontId = a->fontBold);
      }
      VDivider(.thickness = 1, .color = COLOR_ON_PRIMARY);
      Column("TimeRemaining", .layout = {.sizing = {.height = Fit()}}) {
        Text("Time Remaining", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_MEDIUM);
        Text(remaining, .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_HEADLINE_LARGE,
             .fontId = a->fontBold);
      }
      Row("CurrentTime",
          .layout = {.sizing = {.width = Fixed(160), .height = Fit()},
                     .childGap = 8,
                     .padding = PadSymmetric(24, 16),
                     .childAlignment = {.y = AlignYCenter(), .x = AlignXCenter()}},
          .backgroundColor = COLOR_SURFACE_VARIANT,

          .cornerRadius = RadiusAll(12)) {
        Image("CurrentTimeIcon", ImgFit(&a->currentTime),
              .layout = {.sizing = {Fixed(24), Fixed(24)}});
        Text(clock, .textColor = COLOR_PRIMARY, .fontSize = FONT_SIZE_HEADLINE_SMALL,
             .fontId = a->fontBold);
      }
    }
  }
}
