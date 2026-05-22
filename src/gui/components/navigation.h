#ifndef MUSLIMTIFY_NAVIGATION_H
#define MUSLIMTIFY_NAVIGATION_H

#include <raylib.h>
#include <stdint.h>

void SideNavigation(void);

#ifdef MUSLIMTIFY_NAVIGATION_IMPLEMENTATION

#include "../themes/app_assets.h"
#include "../themes/colors.h"
#include "ccompose.h"

typedef struct {
  char *chars;
  char *icon_id;
  Texture2D *icon;
  Texture2D *icon_inactive;
} Navigation;

static bool isExpanded = true;

static Navigation NAVIGATION_ITEMS[] = {
    {
        .chars = "Dashboard",
        .icon_id = "NavIconDashboard",
        .icon = &g_iconDashboard,
        .icon_inactive = &g_iconDashboardInactive,
    },
    {
        .chars = "Prayers",
        .icon_id = "NavIconPrayers",
        .icon = &g_iconPrayers,
        .icon_inactive = &g_iconPrayersInactive,
    },
    {
        .chars = "Location",
        .icon_id = "NavIconLocation",
        .icon = &g_iconLocation,
        .icon_inactive = &g_iconLocationInactive,
    },
    {
        .chars = "Notification",
        .icon_id = "NavIconNotification",
        .icon = &g_iconNotification,
        .icon_inactive = &g_iconNotificationInactive,
    },
    {
        .chars = "About",
        .icon_id = "NavIconAbout",
        .icon = &g_iconAbout,
        .icon_inactive = &g_iconAboutInactive,
    },
};

#define NAVIGATION_ITEMS_COUNT (sizeof(NAVIGATION_ITEMS) / sizeof(NAVIGATION_ITEMS[0]))

typedef void (*OnNavigationSelectedItem)(const size_t index);

static size_t selectedItem = 0;

static CC_Color NavigationButtonColor(size_t currentIndex) {
  return currentIndex == selectedItem ? COLOR_PRIMARY : COLOR_SURFACE_VARIANT;
}

static CC_Color NavigationButtonTextColor(size_t currentIndex) {
  return currentIndex == selectedItem ? COLOR_ON_PRIMARY : COLOR_ON_SURFACE;
}

static void onNavigationItemSelected(const size_t index) {
  selectedItem = index;
}

static void NavigationItems(OnNavigationSelectedItem onSelected, bool isMinimized) {
  Column("NavigationItems", .layout = {
                                .sizing = {.width = Grow(), .height = Grow()},
                                .padding = PadSymmetric(0, 16),
                            }) {
    for (size_t i = 0; i < NAVIGATION_ITEMS_COUNT; i++) {
      if (CC_Clicked(NAVIGATION_ITEMS[i].chars)) {
        onSelected(i);
      }

      Button(NAVIGATION_ITEMS[i].chars,
             .layout = {.sizing = {.width = Grow()},
                        .padding = PadAll(16),
                        .childAlignment = {.x = AlignXStart(), .y = AlignYCenter()},
                        .childGap = 8},
             .backgroundColor = NavigationButtonColor(i), .cornerRadius = RadiusAll(8)) {

        bool selected = i == selectedItem;

        Texture2D *icon = selected ? NAVIGATION_ITEMS[i].icon : NAVIGATION_ITEMS[i].icon_inactive;

        Image(NAVIGATION_ITEMS[i].icon_id, ImgFit(icon),
              .layout = {.sizing = {.height = Fixed(20), .width = Fixed(20)}});

        if (!isMinimized)
          Text(NAVIGATION_ITEMS[i].chars, .textColor = NavigationButtonTextColor(i), .fontSize = 16,
               .fontId = selected ? g_fontBold : 0);
      }
    }
  }
}

static void SideNavigationCollapsed(void) {
  Column("Navigation",
         .layout = {.sizing = {.height = Grow(), .width = Fixed(80)}, .padding = PadAll(16)},
         .backgroundColor = COLOR_SURFACE_VARIANT) {

    VSpacer();
    if (CC_Clicked("Minimize")) {
      isExpanded = !isExpanded;
    }

    Button("Minimize", .layout = {.padding = PadAll(8)}, .cornerRadius = RadiusAll(999),
           .backgroundColor = CC_Hovered("Minimize") ? COLOR_ON_PRIMARY : COLOR_SURFACE_VARIANT) {
      Texture2D *icon = isExpanded ? &g_iconExpand : &g_iconCollapse;
      Image("ExpandedIcon", ImgFit(icon),
            .layout = {.sizing = {.height = Fixed(24), .width = Fixed(24)}});
    }
    NavigationItems(onNavigationItemSelected, true);
  }
}

static void SideNavigationExpanded(void) {
  Column("Navigation",
         .layout = {.sizing = {.height = Grow(), .width = Fixed(288)}, .padding = PadAll(16)},
         .backgroundColor = COLOR_SURFACE_VARIANT) {
    Row("LogoAndAction",
        .layout = {
            .sizing = {.width = Grow(), .height = Fit()},
            .childAlignment = {.x = AlignXStart(), .y = AlignYCenter()},
        }, ) {

      Text("Muslimtify", .textColor = COLOR_PRIMARY, .fontSize = 24, .fontId = g_fontManrope);
      HSpacer();
      if (CC_Clicked("Minimize")) {
        isExpanded = !isExpanded;
      }

      Button("Minimize", .layout = {.padding = PadAll(8)}, .cornerRadius = RadiusAll(999),
             .backgroundColor = CC_Hovered("Minimize") ? COLOR_ON_PRIMARY : COLOR_SURFACE_VARIANT) {
        Texture2D *icon = isExpanded ? &g_iconExpand : &g_iconCollapse;
        Image("ExpandedIcon", ImgFit(icon),
              .layout = {.sizing = {.height = Fixed(24), .width = Fixed(24)}});
      }
    }
    Text("Sacred Precision", .textColor = COLOR_ON_SURFACE);
    NavigationItems(onNavigationItemSelected, false);
  }
}

void SideNavigation(void) {
  if (isExpanded)
    SideNavigationExpanded();
  else
    SideNavigationCollapsed();
}

#endif

#endif
