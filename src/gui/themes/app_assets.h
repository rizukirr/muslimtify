#ifndef MUSLIMTIFY_APP_ASSETS_H
#define MUSLIMTIFY_APP_ASSETS_H

#include "ccompose.h"

extern int16_t g_fontManrope;
extern int16_t g_fontBold;

extern Texture2D g_iconCurrentTime;
extern Texture2D g_iconCurrentLocation;
extern Texture2D g_iconCalculationProfile;
extern Texture2D g_iconModifySettings;

void AppAssetsLoad(void);
void AppAssetsUnload(void);

#ifdef MUSLIMTIFY_APP_ASSETS_IMPLEMENTATION

#include "assets.h"
#include "fonts.h"

int16_t g_fontManrope;
int16_t g_fontBold;

Texture2D g_iconCurrentTime;
Texture2D g_iconCurrentLocation;
Texture2D g_iconCalculationProfile;
Texture2D g_iconModifySettings;

void AppAssetsLoad(void) {
  g_fontManrope = (int16_t)CC_LoadFont(FONT_MANROPE_BOLD, 48);
  g_fontBold = (int16_t)CC_LoadFont(FONT_INTER_BOLD, 48);

  g_iconCurrentTime = CC_LoadImage(IC_PRAYERS_TIME);
  g_iconCurrentLocation = CC_LoadImage(IC_CURRENT_LOCATION);
  g_iconCalculationProfile = CC_LoadImage(IC_CALCULATION_PROFILE);
  g_iconModifySettings = CC_LoadImage(IC_NEXT);
}

void AppAssetsUnload(void) {
  CC_UnloadImage(g_iconCurrentTime);
  CC_UnloadImage(g_iconCurrentLocation);
  CC_UnloadImage(g_iconCalculationProfile);
  CC_UnloadImage(g_iconModifySettings);
}

#endif

#endif
