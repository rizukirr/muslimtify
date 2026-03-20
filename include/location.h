#ifndef LOCATION_H
#define LOCATION_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Fetch location information from ipinfo.io and update config
 * Returns: 0 on success, -1 on failure
 */
int location_fetch(Config *cfg);

/**
 * Alias for location_fetch (for clarity)
 */
int location_auto_detect(Config *cfg);

/**
 * Free any resources allocated by location module
 */
void location_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // LOCATION_H
