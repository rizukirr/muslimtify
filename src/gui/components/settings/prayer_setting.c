#include "prayer_setting.h"
#include "ccompose.h"
#include "components/topbar.h"

void PrayerSetting(void) {
  Column("PrayerSettings", .layout = {.sizing = {.width = Grow(), .height = Grow()}}) {
    TopBar("Prayer Settings");
  }
}
