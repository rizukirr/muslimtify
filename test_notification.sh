#!/bin/bash
# Test notification with custom icon from assets/muslimtify.png

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Muslimtify - Notification Test Script                    ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Get absolute path to icon
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ICON_PATH="$SCRIPT_DIR/assets/muslimtify.png"

echo -e "${BLUE}→${NC} Icon path: ${GREEN}$ICON_PATH${NC}"

# Check if icon exists
if [ ! -f "$ICON_PATH" ]; then
    echo -e "${RED}✗ Error: Icon not found at $ICON_PATH${NC}"
    exit 1
fi

echo -e "${GREEN}✓${NC} Icon file found"
echo -e "${BLUE}→${NC} Icon size: $(du -h "$ICON_PATH" | cut -f1)"
echo -e "${BLUE}→${NC} Icon type: $(file -b "$ICON_PATH" | cut -d',' -f1)"
echo ""

# Check if notify-send is available
if ! command -v notify-send &> /dev/null; then
    echo -e "${RED}✗ Error: notify-send not found${NC}"
    echo "Please install libnotify-bin:"
    echo "  Ubuntu/Debian: sudo apt install libnotify-bin"
    echo "  Fedora: sudo dnf install libnotify"
    echo "  Arch: sudo pacman -S libnotify"
    exit 1
fi

echo -e "${GREEN}✓${NC} notify-send is available"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Function to send notification
send_test_notification() {
    local title="$1"
    local message="$2"
    local urgency="${3:-normal}"
    
    echo -e "${YELLOW}→ Sending: ${title}${NC}"
    echo -e "  Message: ${message}"
    echo -e "  Urgency: ${urgency}"
    
    notify-send -i "$ICON_PATH" -t 5000 -u "$urgency" "$title" "$message"
    
    echo -e "${GREEN}✓ Notification sent!${NC}"
    echo ""
}

echo "Testing different notification types..."
echo ""

# Test 1: Prayer Reminder (Normal urgency)
echo -e "${BLUE}Test 1/3:${NC} Prayer Reminder (30 minutes before)"
send_test_notification \
    "Prayer Reminder: Fajr" \
    "Fajr prayer in 30 minutes\nTime: 04:42" \
    "normal"

sleep 2

# Test 2: Prayer Reminder (15 minutes before)
echo -e "${BLUE}Test 2/3:${NC} Prayer Reminder (15 minutes before)"
send_test_notification \
    "Prayer Reminder: Dhuhr" \
    "Dhuhr prayer in 15 minutes\nTime: 12:09" \
    "normal"

sleep 2

# Test 3: Exact Prayer Time (Critical urgency)
echo -e "${BLUE}Test 3/3:${NC} Exact Prayer Time"
send_test_notification \
    "Prayer Time: Maghrib" \
    "It's time for Maghrib prayer\nTime: 18:16" \
    "critical"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo -e "${GREEN}✓ All test notifications sent successfully!${NC}"
echo ""
echo "Did you see the notifications with the custom mosque icon?"
echo ""
echo "If notifications didn't appear, check:"
echo "  • Notification daemon is running"
echo "  • Desktop environment supports notifications"
echo "  • Do Not Disturb mode is OFF"
echo ""
echo "To test muslimtify directly:"
echo -e "  ${BLUE}./build/bin/muslimtify check${NC}"
echo ""
