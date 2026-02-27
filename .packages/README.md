# Packaging Guide

This directory contains packaging configurations for distributing muslimtify.

```
.packages/
├── aur/            # Arch Linux (AUR)
│   ├── PKGBUILD
│   ├── .SRCINFO
│   └── muslimtify.install
├── debian/         # Debian/Ubuntu (Launchpad PPA)
│   ├── debian/     # Debian packaging files
│   ├── build.sh    # Build .deb locally
│   └── upload-ppa.sh  # Build + sign + upload to PPA
└── README.md       # This file
```

## Prerequisites

- GPG key registered on Launchpad: `F6F1FE418251F4905D994AC747EED10975B13711`
- Launchpad PPA: `ppa:rizukirr/muslimtify`
- AUR repo: `ssh://aur@aur.archlinux.org/muslimtify.git`
- `debootstrap` and `ubuntu-keyring` installed (for PPA builds on Arch)
- `dput` and `python-charset-normalizer` installed (for PPA uploads)

---

## Releasing a New Version

When you bump the version, update these files:

1. **Source code** — `src/cli.c` (version string)
2. **AUR** — `.packages/aur/PKGBUILD` (`pkgver=`)
3. **AUR** — `.packages/aur/.SRCINFO` (`pkgver =`)
4. **Debian** — `.packages/debian/debian/changelog` (new entry at top)

---

## AUR (Arch Linux)

### First-time setup

```bash
# Clone the AUR repo (separate from the main repo)
git clone ssh://aur@aur.archlinux.org/muslimtify.git ~/aur-muslimtify
```

### Updating the AUR package

1. **Update version in PKGBUILD:**

   ```bash
   # Edit .packages/aur/PKGBUILD
   # Change: pkgver=X.Y.Z
   # Reset:  pkgrel=1
   ```

2. **Update checksums** (after pushing a new GitHub release tag):

   ```bash
   cd .packages/aur
   updpkgsums  # from pacman-contrib, auto-downloads and updates sha256sums
   ```

3. **Regenerate .SRCINFO:**

   ```bash
   makepkg --printsrcinfo > .SRCINFO
   ```

4. **Test the build locally:**

   ```bash
   makepkg -si  # builds and installs
   ```

5. **Push to AUR:**

   ```bash
   # Copy updated files to AUR repo
   cp .packages/aur/PKGBUILD ~/aur-muslimtify/
   cp .packages/aur/.SRCINFO ~/aur-muslimtify/
   cp .packages/aur/muslimtify.install ~/aur-muslimtify/

   cd ~/aur-muslimtify
   git add PKGBUILD .SRCINFO muslimtify.install
   git commit -m "Update to vX.Y.Z"
   git push
   ```

### Full AUR update checklist

```
[ ] Update pkgver in PKGBUILD
[ ] Reset pkgrel to 1
[ ] Create and push git tag: git tag vX.Y.Z && git push --tags
[ ] Run updpkgsums
[ ] Regenerate .SRCINFO
[ ] Test with makepkg -si
[ ] Push to AUR
```

---

## Debian/Ubuntu (Launchpad PPA)

### How it works

The PPA build process uses a debootstrap chroot (since we're on Arch):

1. `build.sh` — builds a `.deb` binary package locally (for testing)
2. `upload-ppa.sh` — builds a source package, signs it, and uploads to Launchpad
3. Launchpad receives the source and builds `.deb` packages for all architectures

### Updating the PPA

1. **Add a new changelog entry:**

   ```bash
   # Edit .packages/debian/debian/changelog
   # Add a new entry at the TOP of the file:
   ```

   ```
   muslimtify (X.Y.Z-1) unstable; urgency=medium

     * Description of changes.

    -- Rizki Rakasiwi <rizkirr.xyz@gmail.com>  Thu, 27 Feb 2026 00:00:00 +0700
   ```

   The date format must be RFC 2822. Generate it with:
   ```bash
   date -R
   ```

2. **Clean old build artifacts:**

   ```bash
   rm -f .packages/debian/muslimtify_*
   ```

3. **Build, sign, and upload:**

   ```bash
   sudo .packages/debian/upload-ppa.sh
   ```

   This will:
   - Build the source package inside a debootstrap chroot
   - Sign the `.dsc` and `.changes` with your GPG key
   - Prompt you to upload via `dput`

4. **Check build status:**

   Visit https://launchpad.net/~rizukirr/+archive/ubuntu/muslimtify/+packages

   You'll also receive an email at rizkirr.xyz@gmail.com when the build completes or fails.

### Targeting multiple Ubuntu series

By default, `upload-ppa.sh` targets Ubuntu 24.04 (noble). To upload for other series:

```bash
# Clean between series
rm -f .packages/debian/muslimtify_*

# Upload for each target
sudo .packages/debian/upload-ppa.sh noble    # Ubuntu 24.04
sudo .packages/debian/upload-ppa.sh jammy    # Ubuntu 22.04
```

Each series needs a separate upload with a distinct version. Edit the changelog to use a
version suffix like `0.1.1-1~jammy1` for jammy and `0.1.1-1~noble1` for noble.

### Building a .deb locally (for testing)

```bash
sudo .packages/debian/build.sh
# Output: .packages/debian/muslimtify_X.Y.Z-1_amd64.deb

# Test install
sudo dpkg -i .packages/debian/muslimtify_*_amd64.deb
sudo apt install -f  # fix dependencies if needed
```

### Re-uploading after rejection

If Launchpad rejects or you need to re-upload:

```bash
rm -f .packages/debian/muslimtify_*
dput --force ppa:rizukirr/muslimtify .packages/debian/muslimtify_*_source.changes
```

Note: Launchpad won't accept the same version twice. If you need to re-upload, bump the
version (e.g., `0.1.1-1` to `0.1.1-2`) in `debian/changelog`.

### Deleting the chroot

The debootstrap chroot is cached at `~/.cache/muslimtify-deb-chroot`. To free disk space:

```bash
sudo rm -rf ~/.cache/muslimtify-deb-chroot
```

It will be recreated on the next build.

---

## Full Release Checklist

```
[ ] Update version in src/cli.c
[ ] Update .packages/aur/PKGBUILD (pkgver, pkgrel=1)
[ ] Update .packages/aur/.SRCINFO
[ ] Update .packages/debian/debian/changelog
[ ] Commit and tag: git tag vX.Y.Z && git push --tags
[ ] Create GitHub release with the tag
[ ] AUR: updpkgsums → makepkg -si → push to AUR
[ ] PPA: rm old artifacts → sudo .packages/debian/upload-ppa.sh
[ ] Verify AUR: yay -S muslimtify
[ ] Verify PPA: https://launchpad.net/~rizukirr/+archive/ubuntu/muslimtify/+packages
```
