#include "prayer_setting.h"
#include "app/assets.h"
#include "ccompose.h"
#include "components/topbar.h"
#include "themes/colors.h"
#include "themes/fonts.h"
#include "utils/gui_prayer.h"
#include <raylib.h>

static CC_Scroll scroll;

void PrayerSettingItem(int index, const DailyScheduleItem item) {
  Assets *a = appAssets();
  CC_Color textColor = item.isEnabled ? COLOR_PRIMARY : COLOR_PRIMARY_ALPHA10;

  Column(TextFormat("PrayerSettingItem%d", index),
         .layout = {.sizing = {.width = Grow()}, .padding = PadAll(16)},
         .border = {.width = BorderAll(1), .color = COLOR_DISABLED},
         .cornerRadius = RadiusAll(16)) {
    Row(TextFormat("PrayerSettingItemHeader%d", index),
        .layout = {.sizing = {.width = Grow()},
                   .childAlignment = {.y = AlignYCenter()},
                   .padding = PadAll(16)}) {
      Box(TextFormat("PrayerSettingItemHeaderIconBox%d", index), .layout = {.padding = PadAll(8)},
          .cornerRadius = RadiusAll(8), .backgroundColor = COLOR_SURFACE_VARIANT) {
        Image(TextFormat("PrayerSettingItemHeaderIcon%d", index),
              ImgFit(PrayerIconTexture(item.icon, item.isEnabled)),
              .layout = {.sizing = {.width = Fixed(18), .height = Fixed(18)}});
      }
      Spacer(.width = 16);
      Text(item.title, .fontId = a->fontBold, .textColor = textColor,
           .fontSize = FONT_SIZE_TITLE_LARGE);
      Spacer(.width = 16);
      Text(item.time, .textColor = COLOR_ON_SURFACE);
    }
  }
}

void PrayerSetting(void) {
  TodayPrayer today[GUI_PRAYER_COUNT];
  guiTodayPrayer(guiGetPrayer(), today);

  DailyScheduleItem dailyScheduleItems[GUI_PRAYER_COUNT];
  int dailyScheduleItemsCount = guiDailySchedule(today, dailyScheduleItems);

  CC_ScrollUpdate(&scroll, "PrayerSettingsChild", .vertical = true, .drag = true, .wheel = true);

  Column("PrayerSettingsParent", .layout = {.sizing = {.width = Grow(), .height = Grow()}}) {
    TopBar("Prayer Settings");
    Column("PrayerSettingsChild",
           .layout = {.sizing = {.width = Grow(), .height = Grow()},
                      .padding = PadAll(16),
                      .childGap = 8},
           .clip = {.vertical = true, .childOffset = scroll.offset}) {
      for (int i = 0; i < dailyScheduleItemsCount; i++) {
        PrayerSettingItem(i, (const DailyScheduleItem)dailyScheduleItems[i]);
      }
    }
  }
}
