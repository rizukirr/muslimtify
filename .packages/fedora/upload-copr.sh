#!/bin/bash
# Build SRPM and upload to Fedora COPR
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
SPEC_FILE="$PROJECT_DIR/.packages/fedora/muslimtify.spec"
PKG_NAME="muslimtify"
PKG_VERSION="$(grep '^Version:' "$SPEC_FILE" | awk '{print $2}')"
COPR_PROJECT="rizukirr/muslimtify"
OUTPUT_DIR="$PROJECT_DIR/.packages/fedora"

echo "==> Building SRPM for ${PKG_NAME}-${PKG_VERSION}"

# --- Setup rpmbuild tree in a temp dir ---
RPMBUILD_DIR="$(mktemp -d)"
mkdir -p "$RPMBUILD_DIR"/{SOURCES,SPECS,SRPMS}
trap 'rm -rf "$RPMBUILD_DIR"' EXIT

# --- Download source tarball ---
SOURCE_URL="https://github.com/rizukirr/muslimtify/archive/v${PKG_VERSION}/${PKG_NAME}-${PKG_VERSION}.tar.gz"
SOURCE_FILE="$RPMBUILD_DIR/SOURCES/${PKG_NAME}-${PKG_VERSION}.tar.gz"

if [ -f "$OUTPUT_DIR/${PKG_NAME}-${PKG_VERSION}.tar.gz" ]; then
    echo "==> Using cached source tarball"
    cp "$OUTPUT_DIR/${PKG_NAME}-${PKG_VERSION}.tar.gz" "$SOURCE_FILE"
else
    echo "==> Downloading source tarball from GitHub..."
    curl -L -o "$SOURCE_FILE" "$SOURCE_URL"
    cp "$SOURCE_FILE" "$OUTPUT_DIR/"
fi

# --- Copy spec ---
cp "$SPEC_FILE" "$RPMBUILD_DIR/SPECS/"

# --- Build SRPM ---
echo "==> Building SRPM..."
rpmbuild -bs \
    --define "_topdir $RPMBUILD_DIR" \
    "$RPMBUILD_DIR/SPECS/muslimtify.spec"

SRPM_FILE="$(find "$RPMBUILD_DIR/SRPMS" -name '*.src.rpm' | head -1)"
cp "$SRPM_FILE" "$OUTPUT_DIR/"
SRPM_BASENAME="$(basename "$SRPM_FILE")"

echo "==> SRPM built: $OUTPUT_DIR/$SRPM_BASENAME"

# --- Upload to COPR ---
if ! command -v copr-cli &>/dev/null; then
    echo ""
    echo "==> copr-cli not found. Install it with:"
    echo "      sudo dnf install copr-cli"
    echo "    or:"
    echo "      pip install copr-cli"
    echo ""
    echo "==> Then configure your API token:"
    echo "      Visit https://copr.fedorainfracloud.org/api/"
    echo "      Save the token to ~/.config/copr"
    echo ""
    echo "==> Manual upload:"
    echo "      copr-cli build ${COPR_PROJECT} $OUTPUT_DIR/$SRPM_BASENAME"
    exit 0
fi

echo ""
read -p "Upload to COPR (${COPR_PROJECT})? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    copr-cli build "$COPR_PROJECT" "$OUTPUT_DIR/$SRPM_BASENAME"
    echo "==> Upload complete! Check build status at:"
    echo "    https://copr.fedorainfracloud.org/coprs/${COPR_PROJECT}/builds/"
fi
