#ifndef MUSLIMTIFY_APP_ASSETS_H
#define MUSLIMTIFY_APP_ASSETS_H

#include "ccompose.h"

extern int16_t g_fontManrope;
extern int16_t g_fontBold;

extern Texture2D g_iconCurrentTime;
extern Texture2D g_iconCurrentLocation;
extern Texture2D g_iconCalculationProfile;
extern Texture2D g_iconModifySettings;

extern Texture2D g_iconFajr;
extern Texture2D g_iconFajrActive;
extern Texture2D g_iconSunrise;
extern Texture2D g_iconSunriseActive;
extern Texture2D g_iconDhuhr;
extern Texture2D g_iconDhuhrActive;
extern Texture2D g_iconAsr;
extern Texture2D g_iconAsrActive;
extern Texture2D g_iconMaghrib;
extern Texture2D g_iconMaghribActive;
extern Texture2D g_iconIsha;
extern Texture2D g_iconIshaActive;

extern Texture2D g_iconDashboard;
extern Texture2D g_iconDashboardInactive;
extern Texture2D g_iconPrayers;
extern Texture2D g_iconPrayersInactive;
extern Texture2D g_iconLocation;
extern Texture2D g_iconLocationInactive;
extern Texture2D g_iconNotification;
extern Texture2D g_iconNotificationInactive;
extern Texture2D g_iconAbout;
extern Texture2D g_iconAboutInactive;
extern Texture2D g_iconExpand;
extern Texture2D g_iconCollapse;

extern Texture2D g_iconNextNotification;

void AppAssetsLoad(void);
void AppAssetsUnload(void);

#ifdef MUSLIMTIFY_APP_ASSETS_IMPLEMENTATION

#include "assets.h"
#include "fonts.h"

int16_t g_fontManrope;
int16_t g_fontBold;
int16_t g_fontNormal;

Texture2D g_iconCurrentTime;
Texture2D g_iconCurrentLocation;
Texture2D g_iconCalculationProfile;
Texture2D g_iconModifySettings;

Texture2D g_iconFajr;
Texture2D g_iconFajrActive;
Texture2D g_iconSunrise;
Texture2D g_iconSunriseActive;
Texture2D g_iconDhuhr;
Texture2D g_iconDhuhrActive;
Texture2D g_iconAsr;
Texture2D g_iconAsrActive;
Texture2D g_iconMaghrib;
Texture2D g_iconMaghribActive;
Texture2D g_iconIsha;
Texture2D g_iconIshaActive;

Texture2D g_iconDashboard;
Texture2D g_iconDashboardInactive;
Texture2D g_iconPrayers;
Texture2D g_iconPrayersInactive;
Texture2D g_iconLocation;
Texture2D g_iconLocationInactive;
Texture2D g_iconNotification;
Texture2D g_iconNotificationInactive;
Texture2D g_iconAbout;
Texture2D g_iconAboutInactive;
Texture2D g_iconExpand;
Texture2D g_iconCollapse;
Texture2D g_iconNextNotification;

void AppAssetsLoad(void) {
  g_fontManrope = (int16_t)CC_LoadFont(FONT_MANROPE_BOLD, 48);
  g_fontBold = (int16_t)CC_LoadFont(FONT_INTER_BOLD, 48);
  g_fontNormal = (int16_t)CC_LoadFont(FONT_INTER_REGULAR, 48);

  g_iconCurrentTime = CC_LoadImage(IC_PRAYERS_TIME);
  g_iconCurrentLocation = CC_LoadImage(IC_CURRENT_LOCATION);
  g_iconCalculationProfile = CC_LoadImage(IC_CALCULATION_PROFILE);
  g_iconModifySettings = CC_LoadImage(IC_NEXT);

  g_iconFajr = CC_LoadImage(IC_FAJR);
  g_iconFajrActive = CC_LoadImage(IC_FAJR_ACTIVE);
  g_iconSunrise = CC_LoadImage(IC_SUNRISE);
  g_iconSunriseActive = CC_LoadImage(IC_SUNRISE_ACTIVE);
  g_iconDhuhr = CC_LoadImage(IC_DHUHR);
  g_iconDhuhrActive = CC_LoadImage(IC_DHUHR_ACTIVE);
  g_iconAsr = CC_LoadImage(IC_ASR);
  g_iconAsrActive = CC_LoadImage(IC_ASR_ACTIVE);
  g_iconMaghrib = CC_LoadImage(IC_MAGHRIB);
  g_iconMaghribActive = CC_LoadImage(IC_MAGHRIB_ACTIVE);
  g_iconIsha = CC_LoadImage(IC_ISHA);
  g_iconIshaActive = CC_LoadImage(IC_ISHA_ACTIVE);

  g_iconDashboard = CC_LoadImage(IC_DASHBOARD);
  g_iconDashboardInactive = CC_LoadImage(IC_DASHBOARD_INACTIVE);
  g_iconPrayers = CC_LoadImage(IC_PRAYERS);
  g_iconPrayersInactive = CC_LoadImage(IC_PRAYERS_INACTIVE);
  g_iconLocation = CC_LoadImage(IC_LOCATION);
  g_iconLocationInactive = CC_LoadImage(IC_LOCATION_INACTIVE);
  g_iconNotification = CC_LoadImage(IC_NOTIFICATION);
  g_iconNotificationInactive = CC_LoadImage(IC_NOTIFICATION_INACTIVE);
  g_iconAbout = CC_LoadImage(IC_ABOUT);
  g_iconAboutInactive = CC_LoadImage(IC_ABOUT_INACTIVE);
  g_iconExpand = CC_LoadImage(IC_EXPAND);
  g_iconCollapse = CC_LoadImage(IC_COLLAPSE);
  
  g_iconNextNotification = CC_LoadImage(IC_NEXT_NOTIFICATION);
}

void AppAssetsUnload(void) {
  CC_UnloadImage(g_iconCurrentTime);
  CC_UnloadImage(g_iconCurrentLocation);
  CC_UnloadImage(g_iconCalculationProfile);
  CC_UnloadImage(g_iconModifySettings);

  CC_UnloadImage(g_iconFajr);
  CC_UnloadImage(g_iconFajrActive);
  CC_UnloadImage(g_iconSunrise);
  CC_UnloadImage(g_iconSunriseActive);
  CC_UnloadImage(g_iconDhuhr);
  CC_UnloadImage(g_iconDhuhrActive);
  CC_UnloadImage(g_iconAsr);
  CC_UnloadImage(g_iconAsrActive);
  CC_UnloadImage(g_iconMaghrib);
  CC_UnloadImage(g_iconMaghribActive);
  CC_UnloadImage(g_iconIsha);
  CC_UnloadImage(g_iconIshaActive);

  CC_UnloadImage(g_iconDashboard);
  CC_UnloadImage(g_iconDashboardInactive);
  CC_UnloadImage(g_iconPrayers);
  CC_UnloadImage(g_iconPrayersInactive);
  CC_UnloadImage(g_iconLocation);
  CC_UnloadImage(g_iconLocationInactive);
  CC_UnloadImage(g_iconNotification);
  CC_UnloadImage(g_iconNotificationInactive);
  CC_UnloadImage(g_iconAbout);
  CC_UnloadImage(g_iconAboutInactive);
  CC_UnloadImage(g_iconExpand);
  CC_UnloadImage(g_iconCollapse);
}

#endif

#endif
