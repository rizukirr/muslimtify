#include "components/prayer_card.h"

#include "app/assets.h"
#include "ccompose.h"
#include "prayer_checker.h"
#include "prayertimes.h"
#include "themes/colors.h"
#include "themes/fonts.h"
#include "utils/gui_prayer.h"
#include <raylib.h>

void PrayerCard(void) {
  Assets *a = appAssets();
  const PrayerSnapshot *snap = guiGetPrayer();

  const char *nextName = (snap->next == PRAYER_NONE) ? "No prayer" : prayer_get_name(snap->next);
  const char *remaining =
      (snap->next == PRAYER_NONE) ? "—" : TextFormat("in %d minutes", snap->minutes_until);

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
    Text(remaining, .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_LARGE);
    VSpacer();
    Row("PrayerCardButtons", .layout = {.sizing = {.width = Grow()},
                                        .childGap = 16,
                                        .childAlignment = {.y = AlignYCenter()}}) {
      Column("startAt") {
        Text("Starts at", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_MEDIUM);
        Text(startStr, .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_HEADLINE_LARGE,
             .fontId = a->fontBold);
      }
      VDivider(.length = 48, .color = COLOR_ON_PRIMARY);
      Column("TimeRemaining") {
        Text("Time Remaining", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_MEDIUM);
        Text("00:43:00", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_HEADLINE_SMALL,
             .fontId = a->fontBold);
      }
      HSpacer();
      Row("CurrentTime",
          .layout = {.sizing = {.width = Fixed(150), .height = Fit()},
                     .childGap = 16,
                     .padding = PadSymmetric(24, 16),
                     .childAlignment = {.y = CC_ALIGN_Y_CENTER}},
          .backgroundColor = COLOR_SURFACE_VARIANT,

          .cornerRadius = RadiusAll(12)) {
        Image("CurrentTimeIcon", ImgFit(&a->currentTime),
              .layout = {.sizing = {Fixed(30), Fixed(30)}});
        Text("11:49 AM", .textColor = COLOR_PRIMARY, .fontSize = FONT_SIZE_HEADLINE_LARGE,
             .fontId = a->fontBold);
      }
    }
  }
}
