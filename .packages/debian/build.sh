#!/bin/bash
# Build muslimtify .deb package using debootstrap chroot on Arch Linux
set -euo pipefail

DISTRO="${1:-noble}"  # Default: Ubuntu 24.04 (noble)
ARCH="amd64"
CHROOT_DIR="${HOME}/.cache/muslimtify-deb-chroot"
PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
PKG_VERSION="$(grep -oP '\(\K[^)]+' "$PROJECT_DIR/.packages/debian/debian/changelog" | head -1 | cut -d- -f1)"
PKG_NAME="muslimtify"

echo "==> Building ${PKG_NAME}_${PKG_VERSION} for ${DISTRO} (${ARCH})"
echo "==> Project: ${PROJECT_DIR}"

# --- Step 1: Create chroot ---
if [ ! -d "$CHROOT_DIR" ]; then
    echo "==> Creating debootstrap chroot at ${CHROOT_DIR}..."
    sudo debootstrap --arch="$ARCH" "$DISTRO" "$CHROOT_DIR" http://archive.ubuntu.com/ubuntu
else
    echo "==> Reusing existing chroot at ${CHROOT_DIR}"
fi

# --- Step 2: Mount necessary filesystems ---
cleanup() {
    echo "==> Cleaning up mounts..."
    sudo umount "$CHROOT_DIR/proc" 2>/dev/null || true
    sudo umount "$CHROOT_DIR/sys" 2>/dev/null || true
    sudo umount "$CHROOT_DIR/dev/pts" 2>/dev/null || true
    sudo umount "$CHROOT_DIR/dev" 2>/dev/null || true
    sudo umount "$CHROOT_DIR/build" 2>/dev/null || true
}
trap cleanup EXIT

sudo mount --bind /proc "$CHROOT_DIR/proc"
sudo mount --bind /sys "$CHROOT_DIR/sys"
sudo mount --bind /dev "$CHROOT_DIR/dev"
sudo mount --bind /dev/pts "$CHROOT_DIR/dev/pts"

# --- Step 3: Copy source into chroot ---
sudo mkdir -p "$CHROOT_DIR/build"
sudo mount --bind "$PROJECT_DIR" "$CHROOT_DIR/build"

# --- Step 4: Install build deps and build ---
sudo chroot "$CHROOT_DIR" /bin/bash -c "
set -euo pipefail

# Enable main + universe repos
cat > /etc/apt/sources.list <<SOURCES
deb http://archive.ubuntu.com/ubuntu ${DISTRO} main universe
deb http://archive.ubuntu.com/ubuntu ${DISTRO}-updates main universe
deb http://archive.ubuntu.com/ubuntu ${DISTRO}-security main universe
SOURCES

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
    build-essential \
    debhelper \
    cmake \
    pkg-config \
    libnotify-dev \
    libcurl4-openssl-dev \
    devscripts \
    fakeroot \
    lsb-release

# Prepare source tree
cd /tmp
rm -rf ${PKG_NAME}-build
cp -a /build ${PKG_NAME}-build
cd ${PKG_NAME}-build

# Copy debian/ directory into source root
cp -a .packages/debian/debian .

# Build the .deb
dpkg-buildpackage -us -uc -b

# Copy results back
cp /tmp/${PKG_NAME}_*.deb /build/.packages/debian/
cp /tmp/${PKG_NAME}_*.buildinfo /build/.packages/debian/ 2>/dev/null || true
cp /tmp/${PKG_NAME}_*.changes /build/.packages/debian/ 2>/dev/null || true

echo '==> Build complete!'
ls -lh /tmp/${PKG_NAME}_*.deb
"

echo "==> Output .deb files:"
ls -lh "$PROJECT_DIR/.packages/debian/"*.deb 2>/dev/null || echo "Check ${PROJECT_DIR}/.packages/debian/ for output"
