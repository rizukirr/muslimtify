#!/bin/bash
# Create the muslimtify COPR project
set -euo pipefail

COPR_PROJECT="rizukirr/muslimtify"

if ! command -v copr-cli &>/dev/null; then
    echo "==> copr-cli not found. Install it with:"
    echo "      sudo dnf install copr-cli"
    echo "    or:"
    echo "      pip install copr-cli"
    echo ""
    echo "==> Then configure your API token:"
    echo "      Visit https://copr.fedorainfracloud.org/api/"
    echo "      Save the token to ~/.config/copr"
    exit 1
fi

echo "==> Creating COPR project: ${COPR_PROJECT}"

copr-cli create muslimtify \
    --chroot fedora-rawhide-x86_64 \
    --chroot fedora-43-x86_64 \
    --chroot fedora-42-x86_64 \
    --description "Prayer time notifier for the desktop" \
    --instructions 'sudo dnf copr enable rizukirr/muslimtify && sudo dnf install muslimtify'

echo "==> COPR project created!"
echo "    https://copr.fedorainfracloud.org/coprs/${COPR_PROJECT}/"
