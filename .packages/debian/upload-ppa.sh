#!/bin/bash
# Build source package in debootstrap chroot and upload to Launchpad PPA
set -euo pipefail

DISTRO="${1:-noble}"  # Ubuntu series to target
ARCH="amd64"
CHROOT_DIR="${HOME}/.cache/muslimtify-deb-chroot"
PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
PKG_VERSION="$(grep -oP '\(\K[^)]+' "$PROJECT_DIR/.packages/debian/debian/changelog" | head -1 | cut -d- -f1)"
PKG_NAME="muslimtify"
GPG_KEY="47EED10975B13711"
PPA="ppa:rizukirr/muslimtify"
OUTPUT_DIR="$PROJECT_DIR/.packages/debian"

echo "==> Building source package ${PKG_NAME}_${PKG_VERSION} for ${DISTRO}"
echo "==> Will upload to ${PPA}"

# --- Ensure chroot exists ---
if [ ! -d "$CHROOT_DIR" ]; then
    echo "==> Creating debootstrap chroot at ${CHROOT_DIR}..."
    sudo debootstrap --arch="$ARCH" "$DISTRO" "$CHROOT_DIR" http://archive.ubuntu.com/ubuntu
fi

# --- Mount filesystems ---
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
sudo mkdir -p "$CHROOT_DIR/build"
sudo mount --bind "$PROJECT_DIR" "$CHROOT_DIR/build"

# --- Build unsigned source package inside chroot ---
sudo chroot "$CHROOT_DIR" /bin/bash -c "
set -euo pipefail

cat > /etc/apt/sources.list <<SOURCES
deb http://archive.ubuntu.com/ubuntu ${DISTRO} main universe
deb http://archive.ubuntu.com/ubuntu ${DISTRO}-updates main universe
deb http://archive.ubuntu.com/ubuntu ${DISTRO}-security main universe
SOURCES

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
    build-essential debhelper cmake pkg-config \
    libnotify-dev libcurl4-openssl-dev \
    devscripts fakeroot dpkg-dev

# Create working copy
cd /tmp
rm -rf ${PKG_NAME}-build
mkdir -p ${PKG_NAME}-${PKG_VERSION}

# Copy source (exclude .git, build dirs, .packages)
cd /build
tar --exclude=.git --exclude=build --exclude=build-release \
    --exclude=.packages --exclude=.cache \
    -cf - . | tar -xf - -C /tmp/${PKG_NAME}-${PKG_VERSION}/

# Create orig tarball
cd /tmp
tar czf ${PKG_NAME}_${PKG_VERSION}.orig.tar.gz ${PKG_NAME}-${PKG_VERSION}

# Add debian/ directory
cp -a /build/.packages/debian/debian ${PKG_NAME}-${PKG_VERSION}/

# Update changelog target to ${DISTRO}
sed -i '1s/unstable/${DISTRO}/' ${PKG_NAME}-${PKG_VERSION}/debian/changelog

# Build unsigned source package
cd ${PKG_NAME}-${PKG_VERSION}
dpkg-buildpackage -S -us -uc -d

# Copy results out
cp /tmp/${PKG_NAME}_${PKG_VERSION}* /build/.packages/debian/ 2>/dev/null || true
cp /tmp/${PKG_NAME}_${PKG_VERSION}.orig.tar.gz /build/.packages/debian/ || true

echo '==> Source package built successfully'
ls -lh /tmp/${PKG_NAME}_${PKG_VERSION}*
"

# --- Sign on host as the real user ---
REAL_USER="${SUDO_USER:-$(whoami)}"
REAL_HOME="$(eval echo "~${REAL_USER}")"
DSC_FILE="${OUTPUT_DIR}/${PKG_NAME}_${PKG_VERSION}-1.dsc"
CHANGES_FILE="${OUTPUT_DIR}/${PKG_NAME}_${PKG_VERSION}-1_source.changes"

echo "==> Signing source package on host as ${REAL_USER}..."

# Fix ownership from chroot build
chown "${REAL_USER}:${REAL_USER}" "${OUTPUT_DIR}/"${PKG_NAME}_${PKG_VERSION}*

# Sign as real user: sign .dsc, update checksums in .changes, sign .changes
sudo -u "${REAL_USER}" bash -c '
set -euo pipefail
export HOME="'"${REAL_HOME}"'"

GPG_KEY="'"${GPG_KEY}"'"
DSC_FILE="'"${DSC_FILE}"'"
CHANGES_FILE="'"${CHANGES_FILE}"'"
GPG_SIGN="gpg --default-key ${GPG_KEY} --batch --yes"

# 1) Sign .dsc
${GPG_SIGN} --clearsign --output "${DSC_FILE}.signed" "${DSC_FILE}"
mv -f "${DSC_FILE}.signed" "${DSC_FILE}"
echo "==> Signed .dsc"

# 2) Update checksums in .changes to match the newly signed .dsc
DSC_BASENAME="$(basename "${DSC_FILE}")"
DSC_SIZE="$(stat -c%s "${DSC_FILE}")"
DSC_MD5="$(md5sum "${DSC_FILE}" | cut -d" " -f1)"
DSC_SHA1="$(sha1sum "${DSC_FILE}" | cut -d" " -f1)"
DSC_SHA256="$(sha256sum "${DSC_FILE}" | cut -d" " -f1)"

# Update the checksums lines in .changes for the .dsc file
# Checksums-Sha1 / Checksums-Sha256: " <hash> <size> <filename>"
# Files (md5):                        " <hash> <size> <section> <priority> <filename>"
sed -i -E "s|^ [0-9a-f]{40} [0-9]+ ${DSC_BASENAME}$| ${DSC_SHA1} ${DSC_SIZE} ${DSC_BASENAME}|" "${CHANGES_FILE}"
sed -i -E "s|^ [0-9a-f]{64} [0-9]+ ${DSC_BASENAME}$| ${DSC_SHA256} ${DSC_SIZE} ${DSC_BASENAME}|" "${CHANGES_FILE}"
sed -i -E "s|^ [0-9a-f]{32} [0-9]+ (.+) ${DSC_BASENAME}$| ${DSC_MD5} ${DSC_SIZE} \1 ${DSC_BASENAME}|" "${CHANGES_FILE}"
echo "==> Updated checksums in .changes"

# 3) Sign .changes
${GPG_SIGN} --clearsign --output "${CHANGES_FILE}.signed" "${CHANGES_FILE}"
mv -f "${CHANGES_FILE}.signed" "${CHANGES_FILE}"
echo "==> Signed .changes"

echo "==> All files signed successfully"
'

echo ""
echo "==> Upload with:"
echo "  dput ppa:rizukirr/muslimtify ${CHANGES_FILE}"
echo ""
read -p "Upload now? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo -u "${REAL_USER}" dput "${PPA}" "${CHANGES_FILE}"
    echo "==> Upload complete! Check build status at:"
    echo "    https://launchpad.net/~rizukirr/+archive/ubuntu/muslimtify/+packages"
fi
