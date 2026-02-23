# Muslimtify Features

## Complete Feature List

### ✅ Core Features (Implemented)

#### 1. Prayer Time Calculation
- **Kemenag Method**: Official Indonesian Ministry of Religious Affairs calculation
- **7 Prayer Times**: Fajr, Sunrise, Dhuha, Dhuhr, Asr, Maghrib, Isha
- **Automatic Date Handling**: Calculates for current date automatically
- **Accurate Calculations**: Uses astronomical formulas with ihtiyat adjustments

#### 2. Location Management
- **Auto-Detection**: Uses ipinfo.io to detect location automatically
- **Manual Override**: Set custom latitude/longitude coordinates
- **Timezone Detection**: Automatically determines timezone and UTC offset
- **City/Country Display**: Shows human-readable location information
- **Persistent Storage**: Location saved in config for offline use

#### 3. Customizable Reminders
- **Multiple Reminders per Prayer**: Set any number of reminders (e.g., 30, 15, 5 min before)
- **Per-Prayer Configuration**: Different reminder times for each prayer
- **Bulk Configuration**: Set reminders for all prayers at once
- **Flexible Format**: Easy CSV format (e.g., "30,15,5")
- **Zero Reminders**: Option to only notify at exact prayer time

#### 4. Prayer Notification Control
- **Enable/Disable per Prayer**: Turn notifications on/off individually
- **Bulk Enable/Disable**: Enable or disable all prayers at once
- **Default Settings**: Fajr, Dhuhr, Asr, Maghrib, Isha enabled by default
- **Sunrise/Dhuha Disabled**: Optional prayers disabled by default
- **Status Display**: Easy-to-read enabled/disabled status

#### 5. Desktop Notifications
- **libnotify Integration**: Native Linux desktop notifications
- **Two Urgency Levels**:
  - CRITICAL for exact prayer time
  - NORMAL for reminders
- **Rich Information**: Shows prayer name, time, and countdown
- **Custom Icons**: Supports mosque icon from system theme
- **Configurable Timeout**: Adjust notification display duration

#### 6. Beautiful CLI Interface
- **Unicode Box Drawing**: Pretty table output with ┌─┐│└─┘ characters
- **ANSI Colors**: Green for enabled, dim for disabled, bold yellow `▶` for next prayer
- **NO_COLOR Support**: Respects `NO_COLOR` env var; plain text when piped/redirected
- **Aligned Columns**: Well-formatted prayer time display
- **Readable Layout**: Clean, organized information display
- **JSON Output**: Optional machine-readable format

#### 7. Configuration Management
- **JSON Format**: Easy to read and edit
- **XDG Compliance**: Stored in ~/.config/muslimtify/
- **Auto-Creation**: Creates default config on first run
- **Validation**: Built-in config validation
- **Reset Function**: Easy restoration to defaults
- **No Manual Editing Required**: Full CLI configuration support

#### 8. CLI Commands (Complete)
- `muslimtify` - Show prayer times (default)
- `muslimtify show` - Display today's prayer times
- `muslimtify next` - Show next prayer and countdown
- `muslimtify check` - Check and notify if prayer time (called by systemd)
- `muslimtify location [auto|show|set|clear]` - Manage location
- `muslimtify enable/disable <prayer>` - Toggle prayers
- `muslimtify list` - Show prayer status
- `muslimtify reminder <prayer> <times>` - Manage reminders
- `muslimtify config [show|reset|validate]` - Config management
- `muslimtify daemon [install|uninstall|status]` - Manage systemd daemon
- `muslimtify version` - Version information
- `muslimtify help` - Help message

#### 9. Systemd Integration
- **Timer-Based**: Runs every minute via systemd timer
- **User Service**: No root required
- **Automatic**: Silent operation, only notifies when needed
- **Logging**: Integrated with systemd journal
- **Resource Efficient**: Process starts and exits, no background daemon
- **Installation Scripts**: Easy setup and removal

#### 10. Data Persistence
- **Config File**: All settings saved in JSON
- **Location Cache**: Remembers detected location
- **Reminder Settings**: Persisted across restarts
- **Prayer Status**: Enabled/disabled state saved

### 🔧 Technical Features

#### Code Quality
- **Modern C (C23)**: Latest C standard
- **Modular Architecture**: Clean separation of concerns
- **Header-Only Libraries**: prayertimes.h and libjson.h
- **No Global State**: Proper encapsulation
- **Error Handling**: Comprehensive error checking
- **Memory Safe**: No leaks, proper cleanup

#### Performance
- **Lightweight**: Minimal memory footprint
- **Fast Execution**: Sub-second runtime
- **No Background Process**: Only runs when needed
- **Efficient I/O**: Single config file read/write
- **Optimized Calculations**: Cached prayer times

#### Dependencies
- **Minimal**: Only libnotify and libcurl required
- **Standard Libraries**: Uses libc, libm
- **No GUI Framework**: Pure CLI and notifications
- **Cross-Distribution**: Works on any Linux with systemd

#### Build System
- **CMake**: Modern build configuration
- **pkg-config**: Automatic dependency detection
- **Parallel Builds**: make -j support
- **Install Target**: Standard installation path

### 📋 Output Examples

#### Prayer Times Display
```
Prayer Times for Monday, February 23, 2026
Location: Jakarta, ID (-6.2146, 106.8451)

┌────────────┬──────────┬──────────┬───────────────────────┐
│ Prayer     │ Time     │ Status   │ Reminders             │
├────────────┼──────────┼──────────┼───────────────────────┤
│ Fajr       │ 04:42    │ Enabled  │ 30, 15, 5 min before  │
│ Sunrise    │ 05:57    │ Disabled │ -                     │
│ Dhuha      │ 06:25    │ Disabled │ -                     │
│ Dhuhr      │ 12:09    │ Enabled  │ 30, 15, 5 min before  │
│▶Asr        │ 15:17    │ Enabled  │ 30, 15, 5 min before  │
│ Maghrib    │ 18:16    │ Enabled  │ 30, 15, 5 min before  │
│ Isha       │ 19:27    │ Enabled  │ 30, 15, 5 min before  │
└────────────┴──────────┴──────────┴───────────────────────┘
```
*(Colors in terminal: bold yellow `▶` for next prayer, green for enabled, dim for disabled)*

#### Next Prayer
```
Next Prayer: Asr
Time: 15:17
Remaining: 2 hours 45 minutes
```

#### Reminder Configuration
```
Prayer Reminders:
  Fajr    : 3 reminders: 30, 15, 5 min before
  Sunrise : (disabled)
  Dhuha   : (disabled)
  Dhuhr   : 3 reminders: 30, 15, 5 min before
  Asr     : 3 reminders: 30, 15, 5 min before
  Maghrib : 3 reminders: 30, 15, 5 min before
  Isha    : 3 reminders: 30, 15, 5 min before
```

### 🚫 Not Implemented (Future Features)

#### Advanced Features (Potential Future)
- Multiple calculation methods (MWL, ISNA, etc.)
- Qibla direction finder
- Monthly prayer calendar
- Hijri date display
- Custom notification sounds
- Notification history
- Prayer time adjustment offsets
- Iqamah time tracking
- Multiple location profiles

### 📊 Configuration Format

```json
{
  "location": {
    "latitude": -6.2146,
    "longitude": 106.8451,
    "timezone": "Asia/Jakarta",
    "timezone_offset": 7.0,
    "auto_detect": true,
    "city": "Jakarta",
    "country": "ID"
  },
  "prayers": {
    "fajr": {
      "enabled": true,
      "reminders": [30, 15, 5]
    },
    "sunrise": {
      "enabled": false,
      "reminders": []
    },
    "dhuha": {
      "enabled": false,
      "reminders": []
    },
    "dhuhr": {
      "enabled": true,
      "reminders": [30, 15, 5]
    },
    "asr": {
      "enabled": true,
      "reminders": [30, 15, 5]
    },
    "maghrib": {
      "enabled": true,
      "reminders": [30, 15, 5]
    },
    "isha": {
      "enabled": true,
      "reminders": [30, 15, 5]
    }
  },
  "notification": {
    "timeout": 5000,
    "urgency": "normal",
    "sound": true,
    "icon": "mosque"
  },
  "calculation": {
    "method": "kemenag",
    "madhab": "shafi"
  }
}
```

### 🎯 Design Principles

1. **User-Friendly**: Simple commands, intuitive interface
2. **Reliable**: Tested, robust error handling
3. **Efficient**: Minimal resource usage
4. **Extensible**: Modular design for future features
5. **Standard-Compliant**: Follows Linux/XDG standards
6. **Self-Documenting**: Clear help messages and examples

### 📈 Testing Coverage

✅ Config loading and saving  
✅ Location auto-detection  
✅ Prayer time calculation  
✅ Reminder parsing  
✅ CLI argument parsing  
✅ Enable/disable functionality  
✅ Reminder management  
✅ Display formatting  
✅ Config validation  
✅ Default config generation  

### 🔄 Workflow

1. **First Run**: Auto-detects location, creates config with defaults
2. **Configuration**: User adjusts reminders, enables/disables prayers
3. **Installation**: Systemd timer installed
4. **Operation**: Timer runs every minute, checks for matches
5. **Notification**: Desktop notification sent when prayer time matches
6. **Updates**: User can modify config anytime via CLI

### 💡 Use Cases

- **Daily Prayer Reminders**: Never miss a prayer time
- **Multiple Advance Warnings**: Prepare for prayer with custom reminders
- **Travel**: Auto-detects new location when traveling
- **Customization**: Different reminders for different prayers
- **Silent Mode**: Disable notifications temporarily
- **Dhuha Tracking**: Optional Dhuha prayer notifications
- **Manual Override**: Set exact location for accuracy
