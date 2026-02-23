# Installation Guide

## Prerequisites

### System Requirements
- Linux operating system (any distribution with systemd)
- Desktop environment with notification support
- Internet connection (for location detection)

### Required Dependencies
- **libnotify** - For desktop notifications
- **libcurl** - For location detection via ipinfo.io
- **systemd** - For timer-based execution (optional but recommended)

## Installation Methods

### Method 1: Build from Source (Recommended)

This is currently the only installation method available.

#### Step 1: Install Build Tools

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install git build-essential cmake pkg-config
```

**Fedora/RHEL:**
```bash
sudo dnf install git gcc gcc-c++ cmake pkgconfig make
```

**Arch Linux:**
```bash
sudo pacman -Sy git base-devel cmake pkgconfig
```

#### Step 2: Install Library Dependencies

**Ubuntu/Debian:**
```bash
sudo apt install libnotify-dev libcurl4-openssl-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install libnotify-devel libcurl-devel
```

**Arch Linux:**
```bash
sudo pacman -S libnotify curl
```

#### Step 3: Clone Repository

```bash
git clone https://github.com/yourusername/muslimtify.git
cd muslimtify
```

#### Step 4: Build and Install

Run the installer — it builds in release mode, installs the binary and icons to `/usr/local`, and sets up the systemd user timer:

```bash
sudo ./install.sh
```

That's it! Skip to [Step 5: First Run](#step-5-first-run).

#### Development Build (Optional)

To build without installing (for testing):

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
./bin/muslimtify
```

**Manual systemd setup (if not using install.sh):**
```bash
# Create systemd user directory
mkdir -p ~/.config/systemd/user

# Create service file
cat > ~/.config/systemd/user/muslimtify.service <<'EOF'
[Unit]
Description=Prayer Time Notification Check
After=network-online.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/muslimtify check
StandardOutput=journal
StandardError=journal
EOF

# Create timer file
cat > ~/.config/systemd/user/muslimtify.timer <<'EOF'
[Unit]
Description=Check prayer times every minute
After=network-online.target

[Timer]
OnCalendar=*:*:00
Persistent=true
AccuracySec=1s

[Install]
WantedBy=timers.target
EOF

# Reload systemd
systemctl --user daemon-reload

# Enable and start timer
systemctl --user enable muslimtify.timer
systemctl --user start muslimtify.timer

# Check status
systemctl --user status muslimtify.timer
```

#### Step 5: First Run

```bash
# Run muslimtify for the first time
muslimtify

# This will:
# 1. Auto-detect your location via ipinfo.io
# 2. Create config at ~/.config/muslimtify/config.json
# 3. Display today's prayer times
```

## Verification

### Test the Application

```bash
# Show prayer times
muslimtify

# Check next prayer
muslimtify next

# Verify location
muslimtify location show

# List prayer status
muslimtify list
```

### Test Systemd Timer

```bash
# Check if timer is active
systemctl --user is-active muslimtify.timer

# View timer schedule
systemctl --user list-timers muslimtify

# View logs
journalctl --user -u muslimtify -n 20
```

### Test Notifications

```bash
# Test system notifications first
notify-send "Test" "If you see this, notifications work"

# Test muslimtify notification (will only show if it's prayer time)
muslimtify check
```

## Post-Installation

### Configure Reminders

```bash
# Set reminders for all prayers (30, 15, 5 minutes before)
muslimtify reminder all 30,15,5

# Set custom reminders for specific prayer
muslimtify reminder fajr 45,30,15
```

### Enable Optional Prayers

```bash
# Enable Dhuha prayer
muslimtify enable dhuha

# Enable Sunrise
muslimtify enable sunrise
```

### Manual Location

If auto-detection doesn't work or you want more accurate times:

```bash
# Set manual coordinates
muslimtify location set -6.2146 106.8451

# Or find your coordinates at: https://www.latlong.net
```

## Uninstallation

### Remove Everything

```bash
# Remove binary, icons, and systemd units (preserves config)
sudo ./uninstall.sh

# Also remove ~/.config/muslimtify
sudo ./uninstall.sh --purge
```

### Remove Configuration (Optional)

```bash
# This will delete all your settings
rm -rf ~/.config/muslimtify
```

### Remove Source Code

```bash
cd ..
rm -rf muslimtify
```

## Troubleshooting

### Build Fails

**"cmake not found":**
```bash
# Install CMake
sudo apt install cmake  # Ubuntu/Debian
sudo dnf install cmake  # Fedora
sudo pacman -S cmake    # Arch
```

**"libnotify not found":**
```bash
# Install libnotify development package
sudo apt install libnotify-dev      # Ubuntu/Debian
sudo dnf install libnotify-devel    # Fedora
sudo pacman -S libnotify            # Arch
```

**"libcurl not found":**
```bash
# Install libcurl development package
sudo apt install libcurl4-openssl-dev  # Ubuntu/Debian
sudo dnf install libcurl-devel         # Fedora
sudo pacman -S curl                    # Arch
```

### Binary Not Found After Installation

```bash
# Check if binary exists
ls -l /usr/local/bin/muslimtify

# Check PATH
echo $PATH

# Add to PATH if needed (add to ~/.bashrc)
export PATH="/usr/local/bin:$PATH"
```

### Permission Denied

```bash
# Make binary executable
chmod +x /path/to/muslimtify

# OR rebuild with correct permissions
cd build
make clean
make -j$(nproc)
```

### Systemd Timer Not Working

```bash
# Check if timer is loaded
systemctl --user list-unit-files | grep muslimtify

# Check for errors
systemctl --user status muslimtify.service

# View detailed logs
journalctl --user -u muslimtify -n 50 --no-pager
```

### Notifications Not Appearing

1. **Verify notification daemon is running:**
   ```bash
   ps aux | grep notification
   ```

2. **Test basic notifications:**
   ```bash
   notify-send "Test" "Hello"
   ```

3. **Check notification settings in your desktop environment**

4. **Verify muslimtify can access notification system:**
   ```bash
   # Run manually and check for errors
   muslimtify check
   ```

## Advanced Configuration

### Custom Build Options

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Custom install prefix
cmake -DCMAKE_INSTALL_PREFIX=/opt/muslimtify ..
```

### Cross-Compilation

```bash
# Example: Build for ARM
cmake -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc ..
```

## Support

For issues, questions, or contributions:
- GitHub Issues: https://github.com/yourusername/muslimtify/issues
- Documentation: See README.md and FEATURES.md

## Next Steps

After successful installation:
1. Read the [README.md](README.md) for usage examples
2. Customize your reminders: `muslimtify reminder all 30,15,5`
3. Check prayer times: `muslimtify`
4. Enable optional prayers: `muslimtify enable dhuha`
5. Verify systemd timer: `systemctl --user status muslimtify.timer`
