#define LIBJSON_IMPLEMENTATION
#include "libjson.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <ctype.h>
#include <errno.h>

static char config_path[512] = {0};

const char *config_get_path(void) {
    if (config_path[0] != '\0') {
        return config_path;
    }
    
    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (!pw) {
            fprintf(stderr, "Error: Cannot determine home directory\n");
            return config_path;
        }
        home = pw->pw_dir;
    }
    
    if (xdg_config) {
        snprintf(config_path, sizeof(config_path), 
                 "%s/muslimtify/config.json", xdg_config);
    } else {
        snprintf(config_path, sizeof(config_path), 
                 "%s/.config/muslimtify/config.json", home);
    }
    
    return config_path;
}

static int ensure_config_dir(void) {
    char dir_path[512];
    const char *path = config_get_path();

    // Extract directory path
    snprintf(dir_path, sizeof(dir_path), "%s", path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
    }

    // Recursively create directories (mkdir -p)
    for (char *p = dir_path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(dir_path, 0755) != 0 && errno != EEXIST) {
                fprintf(stderr, "Error: Cannot create directory '%s': %s\n",
                        dir_path, strerror(errno));
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(dir_path, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: Cannot create config directory '%s': %s\n",
                dir_path, strerror(errno));
        return -1;
    }

    return 0;
}

Config config_default(void) {
    Config cfg = {0};
    
    // Location defaults
    cfg.auto_detect = true;
    strcpy(cfg.timezone, "UTC");
    cfg.timezone_offset = 0.0;
    
    // Prayer defaults with reminders [30, 15, 5]
    int default_reminders[] = {30, 15, 5};
    
    cfg.fajr.enabled = true;
    memcpy(cfg.fajr.reminders, default_reminders, sizeof(default_reminders));
    cfg.fajr.reminder_count = 3;
    
    cfg.sunrise.enabled = false;  // DISABLED BY DEFAULT
    cfg.sunrise.reminder_count = 0;
    
    cfg.dhuha.enabled = false;    // DISABLED BY DEFAULT
    cfg.dhuha.reminder_count = 0;
    
    cfg.dhuhr.enabled = true;
    memcpy(cfg.dhuhr.reminders, default_reminders, sizeof(default_reminders));
    cfg.dhuhr.reminder_count = 3;
    
    cfg.asr.enabled = true;
    memcpy(cfg.asr.reminders, default_reminders, sizeof(default_reminders));
    cfg.asr.reminder_count = 3;
    
    cfg.maghrib.enabled = true;
    memcpy(cfg.maghrib.reminders, default_reminders, sizeof(default_reminders));
    cfg.maghrib.reminder_count = 3;
    
    cfg.isha.enabled = true;
    memcpy(cfg.isha.reminders, default_reminders, sizeof(default_reminders));
    cfg.isha.reminder_count = 3;
    
    // Notification defaults
    cfg.notification_timeout = 5000;
    strcpy(cfg.notification_urgency, "critical");
    cfg.notification_sound = true;
    strcpy(cfg.notification_icon, "muslimtify");
    
    // Calculation defaults
    strcpy(cfg.calculation_method, "kemenag");
    strcpy(cfg.madhab, "shafi");
    
    return cfg;
}

static void json_escape_string(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; s++) {
        switch (*s) {
        case '"':  fputs("\\\"", f); break;
        case '\\': fputs("\\\\", f); break;
        case '\b': fputs("\\b", f);  break;
        case '\f': fputs("\\f", f);  break;
        case '\n': fputs("\\n", f);  break;
        case '\r': fputs("\\r", f);  break;
        case '\t': fputs("\\t", f);  break;
        default:
            if ((unsigned char)*s < 0x20) {
                fprintf(f, "\\u%04x", (unsigned char)*s);
            } else {
                fputc(*s, f);
            }
            break;
        }
    }
    fputc('"', f);
}

static void write_json_file(FILE *f, const Config *cfg) {
    fprintf(f, "{\n");
    fprintf(f, "  \"location\": {\n");
    fprintf(f, "    \"latitude\": %.6f,\n", cfg->latitude);
    fprintf(f, "    \"longitude\": %.6f,\n", cfg->longitude);
    fprintf(f, "    \"timezone\": "); json_escape_string(f, cfg->timezone); fprintf(f, ",\n");
    fprintf(f, "    \"timezone_offset\": %.1f,\n", cfg->timezone_offset);
    fprintf(f, "    \"auto_detect\": %s,\n", cfg->auto_detect ? "true" : "false");
    fprintf(f, "    \"city\": "); json_escape_string(f, cfg->city); fprintf(f, ",\n");
    fprintf(f, "    \"country\": "); json_escape_string(f, cfg->country); fprintf(f, "\n");
    fprintf(f, "  },\n");
    
    fprintf(f, "  \"prayers\": {\n");
    
    const char *prayer_names[] = {"fajr", "sunrise", "dhuha", "dhuhr", "asr", "maghrib", "isha"};
    const PrayerConfig *prayers[] = {&cfg->fajr, &cfg->sunrise, &cfg->dhuha, 
                                      &cfg->dhuhr, &cfg->asr, &cfg->maghrib, &cfg->isha};
    
    for (int i = 0; i < 7; i++) {
        fprintf(f, "    \"%s\": {\n", prayer_names[i]);
        fprintf(f, "      \"enabled\": %s,\n", prayers[i]->enabled ? "true" : "false");
        fprintf(f, "      \"reminders\": [");
        for (int j = 0; j < prayers[i]->reminder_count; j++) {
            fprintf(f, "%d", prayers[i]->reminders[j]);
            if (j < prayers[i]->reminder_count - 1) fprintf(f, ", ");
        }
        fprintf(f, "]\n");
        fprintf(f, "    }%s\n", i < 6 ? "," : "");
    }
    
    fprintf(f, "  },\n");
    
    fprintf(f, "  \"notification\": {\n");
    fprintf(f, "    \"timeout\": %d,\n", cfg->notification_timeout);
    fprintf(f, "    \"urgency\": "); json_escape_string(f, cfg->notification_urgency); fprintf(f, ",\n");
    fprintf(f, "    \"sound\": %s,\n", cfg->notification_sound ? "true" : "false");
    fprintf(f, "    \"icon\": "); json_escape_string(f, cfg->notification_icon); fprintf(f, "\n");
    fprintf(f, "  },\n");

    fprintf(f, "  \"calculation\": {\n");
    fprintf(f, "    \"method\": "); json_escape_string(f, cfg->calculation_method); fprintf(f, ",\n");
    fprintf(f, "    \"madhab\": "); json_escape_string(f, cfg->madhab); fprintf(f, "\n");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");
}

int config_save(const Config *cfg) {
    if (ensure_config_dir() != 0) {
        return -1;
    }
    
    const char *path = config_get_path();
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot write config file: %s\n", strerror(errno));
        return -1;
    }
    
    write_json_file(f, cfg);
    fclose(f);
    
    return 0;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_SET);

    char *content = malloc((size_t)size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    size_t n = fread(content, 1, (size_t)size, f);
    content[n] = '\0';
    fclose(f);
    
    return content;
}

static void parse_prayer_config(JsonContext *ctx, char *prayer_obj, PrayerConfig *pcfg) {
    char *enabled_str = get_value(ctx, "enabled", prayer_obj);
    if (enabled_str) {
        pcfg->enabled = strcmp(enabled_str, "true") == 0;
    }
    
    char *reminders_str = get_value(ctx, "reminders", prayer_obj);
    if (reminders_str && reminders_str[0] == '[') {
        // Simple manual parsing of array [30, 15, 5]
        char *p = reminders_str + 1;  // Skip '['
        pcfg->reminder_count = 0;
        
        while (*p && *p != ']' && pcfg->reminder_count < MAX_REMINDERS) {
            // Skip whitespace and commas
            while (*p && (*p == ' ' || *p == ',')) p++;
            
            if (*p >= '0' && *p <= '9') {
                int value = atoi(p);
                if (value > 0) {
                    pcfg->reminders[pcfg->reminder_count++] = value;
                }
                // Skip to next number
                while (*p && *p >= '0' && *p <= '9') p++;
            } else {
                break;
            }
        }
    }
}

int config_load(Config *cfg) {
    const char *path = config_get_path();
    
    // Check if file exists
    if (access(path, F_OK) != 0) {
        // Config doesn't exist, return default
        *cfg = config_default();
        return 0;
    }
    
    // Initialize with defaults so partial JSON still has sane values
    *cfg = config_default();

    char *content = read_file(path);
    if (!content) {
        fprintf(stderr, "Error: Cannot read config file\n");
        return -1;
    }

    JsonContext *ctx = json_begin();
    if (!ctx) {
        free(content);
        return -1;
    }
    
    // Parse location
    char *location = get_value(ctx, "location", content);
    if (location) {
        char *lat_str = get_value(ctx, "latitude", location);
        char *lon_str = get_value(ctx, "longitude", location);
        char *tz_str = get_value(ctx, "timezone", location);
        char *tz_offset_str = get_value(ctx, "timezone_offset", location);
        char *auto_detect_str = get_value(ctx, "auto_detect", location);
        char *city_str = get_value(ctx, "city", location);
        char *country_str = get_value(ctx, "country", location);
        
        if (lat_str) cfg->latitude = atof(lat_str);
        if (lon_str) cfg->longitude = atof(lon_str);
        if (tz_str) {
            strncpy(cfg->timezone, tz_str, sizeof(cfg->timezone) - 1);
            cfg->timezone[sizeof(cfg->timezone) - 1] = '\0';
        }
        if (tz_offset_str) cfg->timezone_offset = atof(tz_offset_str);
        if (auto_detect_str) cfg->auto_detect = strcmp(auto_detect_str, "true") == 0;
        if (city_str) {
            strncpy(cfg->city, city_str, sizeof(cfg->city) - 1);
            cfg->city[sizeof(cfg->city) - 1] = '\0';
        }
        if (country_str) {
            strncpy(cfg->country, country_str, sizeof(cfg->country) - 1);
            cfg->country[sizeof(cfg->country) - 1] = '\0';
        }
    }
    
    // Parse prayers
    char *prayers = get_value(ctx, "prayers", content);
    if (prayers) {
        char *fajr = get_value(ctx, "fajr", prayers);
        if (fajr) parse_prayer_config(ctx, fajr, &cfg->fajr);
        
        char *sunrise = get_value(ctx, "sunrise", prayers);
        if (sunrise) parse_prayer_config(ctx, sunrise, &cfg->sunrise);
        
        char *dhuha = get_value(ctx, "dhuha", prayers);
        if (dhuha) parse_prayer_config(ctx, dhuha, &cfg->dhuha);
        
        char *dhuhr = get_value(ctx, "dhuhr", prayers);
        if (dhuhr) parse_prayer_config(ctx, dhuhr, &cfg->dhuhr);
        
        char *asr = get_value(ctx, "asr", prayers);
        if (asr) parse_prayer_config(ctx, asr, &cfg->asr);
        
        char *maghrib = get_value(ctx, "maghrib", prayers);
        if (maghrib) parse_prayer_config(ctx, maghrib, &cfg->maghrib);
        
        char *isha = get_value(ctx, "isha", prayers);
        if (isha) parse_prayer_config(ctx, isha, &cfg->isha);
    }
    
    // Parse notification
    char *notification = get_value(ctx, "notification", content);
    if (notification) {
        char *timeout_str = get_value(ctx, "timeout", notification);
        char *urgency_str = get_value(ctx, "urgency", notification);
        char *sound_str = get_value(ctx, "sound", notification);
        char *icon_str = get_value(ctx, "icon", notification);
        
        if (timeout_str) cfg->notification_timeout = atoi(timeout_str);
        if (urgency_str) {
            strncpy(cfg->notification_urgency, urgency_str,
                    sizeof(cfg->notification_urgency) - 1);
            cfg->notification_urgency[sizeof(cfg->notification_urgency) - 1] = '\0';
        }
        if (sound_str) cfg->notification_sound = strcmp(sound_str, "true") == 0;
        if (icon_str) {
            strncpy(cfg->notification_icon, icon_str,
                    sizeof(cfg->notification_icon) - 1);
            cfg->notification_icon[sizeof(cfg->notification_icon) - 1] = '\0';
        }
    }
    
    // Parse calculation
    char *calculation = get_value(ctx, "calculation", content);
    if (calculation) {
        char *method_str = get_value(ctx, "method", calculation);
        char *madhab_str = get_value(ctx, "madhab", calculation);
        
        if (method_str) {
            strncpy(cfg->calculation_method, method_str,
                    sizeof(cfg->calculation_method) - 1);
            cfg->calculation_method[sizeof(cfg->calculation_method) - 1] = '\0';
        }
        if (madhab_str) {
            strncpy(cfg->madhab, madhab_str, sizeof(cfg->madhab) - 1);
            cfg->madhab[sizeof(cfg->madhab) - 1] = '\0';
        }
    }
    
    json_end(ctx);
    free(content);
    
    return 0;
}

bool config_validate(const Config *cfg) {
    if (!cfg) return false;
    
    // Validate location
    if (cfg->latitude < -90.0 || cfg->latitude > 90.0) return false;
    if (cfg->longitude < -180.0 || cfg->longitude > 180.0) return false;
    if (cfg->timezone_offset < -12.0 || cfg->timezone_offset > 14.0) return false;
    
    // Validate reminders
    const PrayerConfig *prayers[] = {
        &cfg->fajr, &cfg->sunrise, &cfg->dhuha,
        &cfg->dhuhr, &cfg->asr, &cfg->maghrib, &cfg->isha
    };
    
    for (int i = 0; i < 7; i++) {
        if (prayers[i]->reminder_count < 0 || 
            prayers[i]->reminder_count > MAX_REMINDERS) {
            return false;
        }
        for (int j = 0; j < prayers[i]->reminder_count; j++) {
            if (prayers[i]->reminders[j] < 0 || prayers[i]->reminders[j] > 1440) {
                return false;
            }
        }
    }
    
    return true;
}

PrayerConfig *config_get_prayer(Config *cfg, const char *prayer_name) {
    if (!cfg || !prayer_name) return NULL;
    
    // Convert to lowercase for comparison
    char name_lower[32];
    strncpy(name_lower, prayer_name, sizeof(name_lower) - 1);
    name_lower[sizeof(name_lower) - 1] = '\0';
    for (int i = 0; name_lower[i]; i++) {
        name_lower[i] = tolower(name_lower[i]);
    }
    
    if (strcmp(name_lower, "fajr") == 0) return &cfg->fajr;
    if (strcmp(name_lower, "sunrise") == 0) return &cfg->sunrise;
    if (strcmp(name_lower, "dhuha") == 0) return &cfg->dhuha;
    if (strcmp(name_lower, "dhuhr") == 0 || strcmp(name_lower, "dhur") == 0) return &cfg->dhuhr;
    if (strcmp(name_lower, "asr") == 0) return &cfg->asr;
    if (strcmp(name_lower, "maghrib") == 0) return &cfg->maghrib;
    if (strcmp(name_lower, "isha") == 0) return &cfg->isha;
    
    return NULL;
}

int config_parse_reminders(const char *reminder_str, 
                          int *reminders, 
                          int max_reminders) {
    if (!reminder_str || !reminders) return -1;
    
    if (strcmp(reminder_str, "none") == 0 || strcmp(reminder_str, "clear") == 0) {
        return 0;
    }
    
    char buffer[256];
    strncpy(buffer, reminder_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    int count = 0;
    char *token = strtok(buffer, ",");
    
    while (token && count < max_reminders) {
        // Trim whitespace
        while (*token == ' ') token++;
        
        int value = atoi(token);
        if (value > 0 && value <= 1440) {
            reminders[count++] = value;
        }
        
        token = strtok(NULL, ",");
    }
    
    return count;
}

void config_format_reminders(const PrayerConfig *prayer, 
                             char *buffer, 
                             size_t bufsize) {
    if (!prayer || !buffer || bufsize == 0) return;
    
    buffer[0] = '\0';
    
    if (prayer->reminder_count == 0) {
        strncpy(buffer, "none", bufsize - 1);
        return;
    }
    
    char temp[16];
    for (int i = 0; i < prayer->reminder_count; i++) {
        snprintf(temp, sizeof(temp), "%d", prayer->reminders[i]);
        strncat(buffer, temp, bufsize - strlen(buffer) - 1);
        
        if (i < prayer->reminder_count - 1) {
            strncat(buffer, ",", bufsize - strlen(buffer) - 1);
        }
    }
}
