#ifndef MUSLIMTIFY_TOPBAR_H
#define MUSLIMTIFY_TOPBAR_H

#include "../themes/app_assets.h"
#include "../themes/colors.h"
#include "../themes/fonts.h"
#include "ccompose.h"

void TopBar(void);

#ifdef MUSLIMTIFY_TOPBAR_IMPLEMENTATION

#include "../themes/colors.h"

void TopBar(void) {
  Row("TopBar",
      .layout = {.sizing = {.height = Fixed(64), .width = Grow()},
                 .padding = Pad(42, 42, 16, 16),
                 .childGap = 8},
      .backgroundColor = COLOR_SURFACE_VARIANT) {
    Image("CurrentLocationLogo", ImgFit(&g_iconCurrentLocation),
          .layout = {.sizing = {.height = Fixed(18), .width = Fixed(18)}});
    Column("CurrentLocation", .layout = {.sizing = {.height = Grow(), .width = Grow()},
                                         .childAlignment = {.y = AlignMiddle()}}) {
      Text("Jakarta, Indonesia", .fontId = g_fontBold, .textColor = COLOR_PRIMARY,
           .fontSize = FONT_SIZE_TITLE_LARGE);
      Text("6.2088 S, 106.8456 E", .textColor = COLOR_ON_BACKGROUND,
           .fontSize = FONT_SIZE_TITLE_MEDIUM, .wrapMode = CC_TEXT_WRAP_WORDS);
    }
  }
}

#endif

#endif
