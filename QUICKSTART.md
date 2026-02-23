# Quick Start Guide

Get Muslimtify up and running in 5 minutes!

## 1. Install Dependencies (30 seconds)

```bash
# Ubuntu/Debian
sudo apt install git build-essential cmake pkg-config libnotify-dev libcurl4-openssl-dev

# Fedora/RHEL
sudo dnf install git gcc cmake pkgconfig libnotify-devel libcurl-devel

# Arch Linux
sudo pacman -S git base-devel cmake pkgconfig libnotify curl
```

## 2. Build and Install (1 minute)

```bash
git clone https://github.com/yourusername/muslimtify.git
cd muslimtify
sudo ./install.sh
```

## 3. Try It! (10 seconds)

```bash
# Show today's prayer times
muslimtify
```

**First run output:**
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

**That's it!** Muslimtify automatically:
- ✅ Detected your location via IP
- ✅ Created config file
- ✅ Set default reminders (30, 15, 5 min before)
- ✅ Disabled optional prayers (Sunrise, Dhuha)

## 4. Customize (1 minute)

```bash
# Change reminders
muslimtify reminder all 45,20,10

# Enable Dhuha prayer
muslimtify enable dhuha

# See next prayer
muslimtify next
```

## 5. Verify Systemd Timer

The installer sets up the timer automatically. Check its status:

```bash
systemctl --user status muslimtify.timer
```

**Output:**
```
● muslimtify.timer - Check prayer times every minute
     Loaded: loaded
     Active: active (waiting)
    Trigger: Mon 2026-02-23 20:15:00 WIB
```

**Done!** You'll now get notifications:
- 🔔 30 minutes before prayer
- 🔔 15 minutes before prayer  
- 🔔 5 minutes before prayer
- ⚠️ At exact prayer time

## Common Tasks

### Change Your Location
```bash
muslimtify location set <latitude> <longitude>
```

### Disable All Reminders (Only Notify at Prayer Time)
```bash
muslimtify reminder all clear
```

### Disable Specific Prayer
```bash
muslimtify disable sunrise
```

### Show Configuration
```bash
muslimtify config show
```

### View Logs
```bash
journalctl --user -u muslimtify -f
```

### Uninstall
```bash
sudo ./uninstall.sh           # remove binary, icons, and systemd units
sudo ./uninstall.sh --purge   # also remove config
```

## Troubleshooting

### "muslimtify: command not found"

**Solution 1:** Run from build directory
```bash
./build/bin/muslimtify
```

**Solution 2:** Add to PATH
```bash
# Add to ~/.bashrc or ~/.zshrc
export PATH="$HOME/muslimtify/build/bin:$PATH"
source ~/.bashrc
```

**Solution 3:** Create symlink
```bash
sudo ln -s /path/to/muslimtify/build/bin/muslimtify /usr/local/bin/muslimtify
```

### Location Not Detected

```bash
# Test internet connection
curl ipinfo.io

# Set manually
muslimtify location set -6.2146 106.8451
```

### Notifications Not Showing

```bash
# Test system notifications
notify-send "Test" "Hello"

# If that works, check muslimtify logs
journalctl --user -u muslimtify -n 20
```

## What's Next?

- Read full documentation: [README.md](README.md)
- Explore all features: [FEATURES.md](FEATURES.md)
- Detailed installation: [INSTALL.md](INSTALL.md)
- Get help: `muslimtify help`

## Getting Help

- **Built-in help:** `muslimtify help`
- **GitHub Issues:** https://github.com/yourusername/muslimtify/issues
- **Documentation:** See README.md

---

**Enjoy never missing a prayer again!** 🕌
