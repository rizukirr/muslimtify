#ifndef MUSLIMTIFY_DASHBOARD_CONTENT_H
#define MUSLIMTIFY_DASHBOARD_CONTENT_H

#include "ccompose.h"
#include <raylib.h>

void DashboardContent(void);

#ifdef MUSLIMTIFY_DASHBOARD_CONTENT_IMPLEMENTATION

#define MUSLIMTIFY_TOPBAR_IMPLEMENTATION
#include "../themes/app_assets.h"
#include "../themes/colors.h"
#include "../themes/fonts.h"
#include "topbar.h"

static void CalculationMethodCard(void) {
  Column("CalculationMethodCard",
         .layout = {.sizing = {.width = Grow()}, .padding = Pad(16, 16, 32, 32), .childGap = 8},
         .backgroundColor = COLOR_SURFACE_ALT, .cornerRadius = RadiusAll(8)) {
    Text("CALCULATION METHOD", .fontSize = FONT_SIZE_TITLE_MEDIUM, .textColor = COLOR_ON_SURFACE);
    Text("Kemenag - Shafi'", .fontId = g_fontBold, .fontSize = FONT_SIZE_HEADLINE_MEDIUM);
    Text("Ministry of Religious Affairs of the Republic of Indonesia");
  }
}

static void ModifySettingsCard(void) {
  Row("ModifySettingsCard", .layout = {.sizing = {.width = Grow()}, .padding = PadAll(16)},
      .backgroundColor = COLOR_SURFACE_ALT, .cornerRadius = RadiusAll(8)) {
    Text("Modify Settings", .fontSize = FONT_SIZE_TITLE_MEDIUM, .fontId = g_fontBold,
         .textColor = COLOR_ON_SURFACE);
    HSpacer();
    Image("ModifySettingsIcons", ImgFit(&g_iconModifySettings),
          .layout = {.sizing = {.height = Fixed(18), .width = Fixed(18)}});
  }
}

static void CalculationProfileCard(void) {
  Column("CalculationProfileCard", .layout = {.padding = PadAll(16), .childGap = 16},
         .cornerRadius = RadiusAll(16), .backgroundColor = COLOR_ON_PRIMARY) {
    Row("CalculationProfileTitle", .layout = {.sizing = {.width = Grow()},
                                              .padding = PadAll(16),
                                              .childGap = 8,
                                              .childAlignment = {.y = AlignMiddle()}}) {
      Box("CalculationProfileIconBox",
          .layout =
              {
                  .padding = PadAll(8),
              },
          .backgroundColor = COLOR_SURFACE_VARIANT, .cornerRadius = RadiusAll(999)) {
        Image(
            "CalculationProfileIcon", ImgFit(&g_iconCalculationProfile),
            .layout = {.sizing = {.height = Fixed(24), .width = Fixed(24)}, .padding = PadAll(8)});
      }
      Text("Calculation Profile", .fontId = g_fontManrope, .fontSize = FONT_SIZE_TITLE_LARGE);
    }
    CalculationMethodCard();
    ModifySettingsCard();
  }
}

static void PrayerCard(void) {
  Column("PrayerCard",
         .layout = {.sizing = {.height = Fit(), .width = Grow()},
                    .padding = PadAll(48),
                    .childGap = 16},
         .cornerRadius = RadiusAll(16), .backgroundColor = COLOR_PRIMARY) {
    Text("UPCOMING PRAYER", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_MEDIUM);
    Text("Duhr", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_DISPLAY_LARGE,
         .fontId = g_fontManrope);
    Text("in 45 minutes", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_LARGE);
    VSpacer();
    Row("PrayerCardButtons", .layout = {.sizing = {.width = Grow()},
                                        .childGap = 16,
                                        .childAlignment = {.y = AlignMiddle()}}) {
      Column("startAt") {
        Text("Starts at", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_MEDIUM);
        Text("12:00 AM", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_HEADLINE_LARGE,
             .fontId = g_fontBold);
      }
      VDivider(.size = 48, .color = COLOR_ON_PRIMARY);
      Column("TimeRemaining") {
        Text("Time Remaining", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_TITLE_MEDIUM);
        Text("00:43:00", .textColor = COLOR_ON_PRIMARY, .fontSize = FONT_SIZE_HEADLINE_SMALL,
             .fontId = g_fontBold);
      }
      HSpacer();
      Row("CurrentTime",
          .layout = {.sizing = {.width = Fixed(150), .height = Fit()},
                     .childGap = 16,
                     .padding = Pad(24, 24, 16, 16),
                     .childAlignment = {.y = CC_ALIGN_Y_CENTER}},
          .backgroundColor = COLOR_SURFACE_VARIANT,

          .cornerRadius = RadiusAll(12)) {
        Image("CurrentTimeIcon", ImgFit(&g_iconCurrentTime),
              .layout = {.sizing = {Fixed(30), Fixed(30)}});
        Text("11:49 AM", .textColor = COLOR_PRIMARY, .fontSize = FONT_SIZE_HEADLINE_LARGE,
             .fontId = g_fontBold);
      }
    }
  }
}

static void DailyScheduleCard(void) {}

static void DailyScheduleSection(void) {
  Column("DailyScheduleSection",
         .layout = {.sizing = {.width = Grow(), .height = Fit()}, .childGap = 16}) {
    Text("Daily Schedule", .fontId = g_fontManrope, .fontSize = FONT_SIZE_HEADLINE_MEDIUM);
    Text("Tuesday, 24 October 2026");
    DailyScheduleCard();
  }
}

void DashboardContent(void) {
  Column("Body", .layout = {.sizing = {.height = Grow(), .width = Grow()}}) {

    TopBar();
    Column("ContentContainer",
           .layout = {.sizing = {.height = Grow(), .width = Grow()}, .padding = PadAll(48)}) {
      Row("Containter1", .layout = {.sizing = {.width = Grow()}, .childGap = 16}) {
        PrayerCard();
        CalculationProfileCard();
      }
      Spacer(.height = Fixed(48));
      DailyScheduleSection();
    }
  }
}
#endif

#endif
