# Packaging Guide

This directory contains packaging configurations for distributing muslimtify
across three Linux distribution families. Each subdirectory targets a different
package manager and build service:

| Directory | Target distros      | Package format | Build service       | User installs with |
|-----------|---------------------|----------------|---------------------|--------------------|
| `aur/`    | Arch Linux, Manjaro | PKGBUILD       | AUR (user-built)    | `yay -S muslimtify` |
| `debian/` | Ubuntu, Debian      | `.deb`         | Launchpad PPA       | `apt install muslimtify` |
| `fedora/` | Fedora, RHEL, CentOS| `.rpm`         | Fedora COPR         | `dnf install muslimtify` |

```
.packages/
├── aur/                    # Arch Linux (AUR)
│   ├── PKGBUILD            # Package recipe — defines version, deps, build steps
│   ├── .SRCINFO            # Metadata parsed by the AUR web interface (auto-generated)
│   └── muslimtify.install  # Post-install/upgrade/remove hook messages
│
├── debian/                 # Debian/Ubuntu (Launchpad PPA)
│   ├── debian/             # Standard Debian packaging directory
│   │   ├── control         # Package metadata — name, deps, description
│   │   ├── rules           # Build recipe (dh + cmake override)
│   │   ├── changelog       # Version history — drives the package version
│   │   ├── copyright       # License info in Debian machine-readable format
│   │   ├── postinst        # Post-install script (prints daemon setup message)
│   │   └── source/format   # Source package format (3.0 quilt)
│   ├── build.sh            # Build .deb binary locally in a debootstrap chroot
│   └── upload-ppa.sh       # Build source package, GPG-sign, upload to Launchpad
│
├── fedora/                 # Fedora (COPR)
│   ├── muslimtify.spec     # RPM spec file — defines metadata, deps, build, file list
│   └── upload-copr.sh      # Build SRPM and upload to Fedora COPR
│
└── README.md               # This file
```

---

## Prerequisites

### Accounts and keys

| Service      | What you need | Link |
|--------------|---------------|------|
| AUR          | SSH key registered on AUR | https://aur.archlinux.org/ |
| Launchpad    | GPG key `F6F1FE418251F4905D994AC747EED10975B13711` registered | https://launchpad.net/~rizukirr |
| Fedora COPR  | Fedora Account (FAS) + COPR API token | https://accounts.fedoraproject.org/ |

### Repository URLs

- **GitHub:** https://github.com/rizukirr/muslimtify
- **AUR:** `ssh://aur@aur.archlinux.org/muslimtify.git`
- **Launchpad PPA:** `ppa:rizukirr/muslimtify`
- **COPR:** `rizukirr/muslimtify`

### Packages needed on the build machine (Arch)

```bash
# AUR tools
pacman -S pacman-contrib base-devel     # provides updpkgsums, makepkg

# PPA tools (building .deb on Arch via debootstrap chroot)
pacman -S debootstrap ubuntu-keyring
yay -S dput python-charset-normalizer

# COPR tools (building .rpm)
yay -S copr-cli rpm-tools
```

---

## Releasing a New Version

### Files to update

When you bump the version, these files **must** be updated:

| # | File | What to change |
|---|------|----------------|
| 1 | `src/cli.c` | Version string in the source code |
| 2 | `CMakeLists.txt` | `project(muslimtify VERSION X.Y.Z ...)` |
| 3 | `.packages/aur/PKGBUILD` | `pkgver=X.Y.Z`, reset `pkgrel=1` |
| 4 | `.packages/aur/.SRCINFO` | Regenerate with `makepkg --printsrcinfo > .SRCINFO` |
| 5 | `.packages/debian/debian/changelog` | New entry at the **top** of the file |
| 6 | `.packages/fedora/muslimtify.spec` | `Version: X.Y.Z`, reset `Release: 1%{?dist}`, new `%changelog` entry |

### Version flow

```
1. Update source code (src/cli.c, CMakeLists.txt)
2. Update all packaging files (steps 3-6 above)
3. Commit everything
4. Tag and push:   git tag vX.Y.Z && git push --tags
5. Create GitHub release from the tag
6. Upload to AUR, PPA, and COPR (see sections below)
```

> **Important:** The AUR and COPR source tarballs are downloaded from GitHub releases,
> so the GitHub release **must** exist before you can update checksums or build SRPMs.

---

## AUR (Arch Linux)

### How it works

The AUR (Arch User Repository) hosts `PKGBUILD` files that users build locally with
a helper like `yay` or `paru`. There is no binary — the user's machine downloads the
source tarball from GitHub and compiles it.

**Key files:**

- `PKGBUILD` — The build recipe. Defines `pkgver`, `depends`, `makedepends`, `source`
  URL, `sha256sums`, and the `build()` / `package()` functions.
- `.SRCINFO` — Machine-readable metadata extracted from `PKGBUILD`. The AUR web
  interface parses this, not the `PKGBUILD` itself. Must be regenerated every time
  `PKGBUILD` changes.
- `muslimtify.install` — Optional install hooks. Contains `post_install()`,
  `post_upgrade()`, and `post_remove()` functions that print daemon setup instructions.

### First-time setup

```bash
# Clone the AUR repo (this is a SEPARATE git repo from the main project)
git clone ssh://aur@aur.archlinux.org/muslimtify.git ~/aur-muslimtify
```

The `.packages/aur/` directory in this repo is a convenience copy. When pushing to
AUR, you copy the files into your `~/aur-muslimtify` clone and push from there.

### Updating the AUR package

1. **Update version in PKGBUILD:**

   ```bash
   # Edit .packages/aur/PKGBUILD
   # Change: pkgver=X.Y.Z
   # Reset:  pkgrel=1  (always reset to 1 for a new upstream version)
   ```

2. **Update checksums** (requires the GitHub release tag to exist):

   ```bash
   cd .packages/aur
   updpkgsums  # downloads the tarball and updates sha256sums= in PKGBUILD
   ```

   `updpkgsums` is from the `pacman-contrib` package. It automatically downloads each
   URL in `source=()` and replaces the checksums.

3. **Regenerate .SRCINFO:**

   ```bash
   makepkg --printsrcinfo > .SRCINFO
   ```

4. **Test the build locally:**

   ```bash
   makepkg -si    # -s = install missing deps, -i = install the built package
   makepkg -si -f # -f = force rebuild if already built
   ```

5. **Push to AUR:**

   ```bash
   cd ~/aur-muslimtify
   cp /path/to/project/.packages/aur/{PKGBUILD,.SRCINFO,muslimtify.install} .
   git add PKGBUILD .SRCINFO muslimtify.install
   git commit -m "Update to vX.Y.Z"
   git push
   ```

### AUR update checklist

```
[ ] Update pkgver in PKGBUILD
[ ] Reset pkgrel to 1
[ ] Ensure GitHub release tag exists (git tag vX.Y.Z && git push --tags)
[ ] Run updpkgsums (updates sha256sums)
[ ] Regenerate .SRCINFO (makepkg --printsrcinfo > .SRCINFO)
[ ] Test locally (makepkg -si)
[ ] Copy files to ~/aur-muslimtify, commit, and push
```

### Troubleshooting

- **`updpkgsums` fails with 404:** The GitHub release tag doesn't exist yet. Create it first.
- **`makepkg` fails on checksum mismatch:** Run `updpkgsums` again — someone may have
  re-tagged the release.
- **AUR push rejected:** Make sure your SSH key is registered at https://aur.archlinux.org/
  under your account settings.

---

## Debian/Ubuntu (Launchpad PPA)

### How it works

Launchpad PPAs host source packages that Launchpad builds into `.deb` binaries for
each target architecture. The workflow is:

1. `upload-ppa.sh` creates a debootstrap chroot (since we develop on Arch, not Ubuntu)
2. Inside the chroot, it builds an **unsigned source package** (`.dsc` + `.orig.tar.gz`)
3. Back on the host, it **GPG-signs** the `.dsc` and `.changes` files
4. You upload the signed source package to Launchpad with `dput`
5. Launchpad receives the source, builds `.deb` binaries, and publishes them to the PPA

Users then add the PPA and install with `apt`.

**Key files:**

- `debian/control` — Package metadata: name, build dependencies, runtime dependencies,
  description. This is where `libnotify-dev` and `libcurl4-openssl-dev` are declared as
  build deps.
- `debian/rules` — The build recipe. Minimal: uses `dh` (debhelper) with a single
  override to pass `-DCMAKE_BUILD_TYPE=Release` to CMake.
- `debian/changelog` — Version history in a strict Debian format. The **first entry**
  determines the package version. Must use RFC 2822 dates.
- `debian/copyright` — License info in Debian machine-readable format (MIT).
- `debian/postinst` — Post-install script that prints a message telling the user to
  run `muslimtify daemon install`.
- `debian/source/format` — Declares source format `3.0 (quilt)`.
- `build.sh` — Builds a `.deb` binary locally inside a debootstrap chroot (for testing).
- `upload-ppa.sh` — Builds a source package in a chroot, GPG-signs it on the host,
  and prompts you to upload via `dput`.

### First-time setup

1. **Register your GPG key on Launchpad:**

   Your key `F6F1FE418251F4905D994AC747EED10975B13711` must be registered at
   https://launchpad.net/~rizukirr/+editpgpkeys

2. **Install tools on Arch:**

   ```bash
   sudo pacman -S debootstrap ubuntu-keyring
   yay -S dput python-charset-normalizer
   ```

3. **The PPA already exists at:** https://launchpad.net/~rizukirr/+archive/ubuntu/muslimtify

### Updating the PPA

1. **Add a new changelog entry** at the **top** of `.packages/debian/debian/changelog`:

   ```
   muslimtify (X.Y.Z-1) unstable; urgency=medium

     * Description of changes.

    -- Rizki Rakasiwi <rizkirr.xyz@gmail.com>  Thu, 27 Feb 2026 00:00:00 +0700
   ```

   > The date format **must** be RFC 2822. Generate it with `date -R`.
   > The two spaces before `--` are required.

2. **Clean old build artifacts:**

   ```bash
   rm -f .packages/debian/muslimtify_*
   ```

3. **Build, sign, and upload:**

   ```bash
   sudo .packages/debian/upload-ppa.sh
   ```

   This script will:
   - Create/reuse a debootstrap chroot at `~/.cache/muslimtify-deb-chroot`
   - Mount the project directory into the chroot
   - Install build dependencies inside the chroot
   - Build an unsigned source package (`.dsc`, `.orig.tar.gz`, `.debian.tar.xz`)
   - Sign the `.dsc` and `.changes` files with GPG key `47EED10975B13711` on the host
   - Update checksums in `.changes` to match the signed `.dsc`
   - Prompt you to upload via `dput`

4. **Check build status:**

   https://launchpad.net/~rizukirr/+archive/ubuntu/muslimtify/+packages

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

Each series needs a separate upload with a **distinct version**. Edit the changelog to use a
version suffix like `0.1.3-1~jammy1` for jammy and `0.1.3-1~noble1` for noble.

### Building a .deb locally (for testing)

```bash
sudo .packages/debian/build.sh
# Output: .packages/debian/muslimtify_X.Y.Z-1_amd64.deb

# Test install
sudo dpkg -i .packages/debian/muslimtify_*_amd64.deb
sudo apt install -f  # fix dependencies if needed
```

`build.sh` works similarly to `upload-ppa.sh` but produces a binary `.deb` instead of
a source package. It uses `dpkg-buildpackage -us -uc -b` (unsigned, binary-only).

### Re-uploading after rejection

If Launchpad rejects your upload or you need to re-upload:

```bash
rm -f .packages/debian/muslimtify_*
dput --force ppa:rizukirr/muslimtify .packages/debian/muslimtify_*_source.changes
```

> **Note:** Launchpad will **not** accept the same version twice. If you need to
> re-upload, bump the version (e.g., `0.1.3-1` → `0.1.3-2`) in `debian/changelog`.

### Deleting the chroot

The debootstrap chroot is cached at `~/.cache/muslimtify-deb-chroot` (~500MB).
To free disk space:

```bash
sudo rm -rf ~/.cache/muslimtify-deb-chroot
```

It will be recreated automatically on the next build.

### Troubleshooting

- **GPG signing fails:** Make sure the key `47EED10975B13711` is in your GPG keyring
  and not expired. Test with `gpg --list-keys 47EED10975B13711`.
- **Launchpad rejects "already uploaded":** Bump the `-N` suffix in the changelog
  (e.g., `-1` → `-2`).
- **Build fails inside chroot:** Delete the chroot (`sudo rm -rf ~/.cache/muslimtify-deb-chroot`)
  and rebuild — it may be corrupted or stale.
- **`dput` fails with "already uploaded":** Remove `~/.dput.cf` upload records or use
  `dput --force`.

---

## Fedora (COPR)

### How it works

Fedora COPR (Cool Other Package Repositories) is the Fedora equivalent of Ubuntu PPAs
or the AUR. The workflow is:

1. `upload-copr.sh` downloads the source tarball from GitHub
2. It builds an **SRPM** (source RPM) locally using `rpmbuild`
3. You upload the SRPM to COPR via `copr-cli`
4. COPR builds binary RPMs for each enabled chroot (Fedora version + architecture)
5. Users enable the COPR repo and install with `dnf`

**Key files:**

- `muslimtify.spec` — The RPM spec file. Contains all metadata, build instructions,
  and the file manifest in one file. Sections:
  - Header: `Name`, `Version`, `Release`, `Summary`, `License`, `URL`, `Source0`
  - `BuildRequires` / `Requires`: build-time and runtime dependencies
  - `%description`: long description
  - `%prep`: source extraction (`%autosetup` unpacks and patches)
  - `%build`: compile steps (`%cmake` + `%cmake_build` macros)
  - `%install`: install into the buildroot (`%cmake_install`)
  - `%post`: post-install script (prints daemon setup message)
  - `%files`: explicit list of every file the package owns
  - `%changelog`: version history (newest entry at the **top**)
- `upload-copr.sh` — Downloads the source tarball, builds an SRPM in a temp directory
  with `rpmbuild -bs`, copies the result to `.packages/fedora/`, and optionally uploads
  to COPR.

### First-time setup

1. **Create a Fedora Account (FAS):**

   Register at https://accounts.fedoraproject.org/

2. **Get your COPR API token:**

   Visit https://copr.fedorainfracloud.org/api/ — log in with your FAS credentials
   and it will show you a config block. Save it to `~/.config/copr`:

   ```ini
   [copr-cli]
   login = <your-login>
   username = rizukirr
   token = <your-token>
   copr_url = https://copr.fedorainfracloud.org
   ```

3. **Install tools:**

   ```bash
   # On Fedora
   sudo dnf install copr-cli rpm-build

   # On Arch
   yay -S copr-cli rpm-tools
   ```

4. **Create the COPR project** (one-time):

   ```bash
   copr-cli create muslimtify \
       --chroot fedora-rawhide-x86_64 \
       --chroot fedora-41-x86_64 \
       --chroot fedora-40-x86_64 \
       --description "Prayer time notifier for the desktop" \
       --instructions 'sudo dnf copr enable rizukirr/muslimtify && sudo dnf install muslimtify'
   ```

   You can also create it via the web UI at https://copr.fedorainfracloud.org/coprs/rizukirr/

   To add more chroots later:

   ```bash
   copr-cli modify muslimtify --chroot fedora-42-x86_64
   ```

### Updating the COPR package

1. **Update the spec file:**

   ```bash
   # Edit .packages/fedora/muslimtify.spec
   # Change: Version: X.Y.Z
   # Reset:  Release: 1%{?dist}
   # Add new %changelog entry at the TOP of the %changelog section:
   ```

   ```
   %changelog
   * Mon Mar 03 2026 Rizki Rakasiwi <rizkirr.xyz@gmail.com> - X.Y.Z-1
   - Release vX.Y.Z
   ```

   > The date format is: `* Day-of-week Month DD YYYY Name <email> - Version-Release`
   > Generate the date portion with `date +"%a %b %d %Y"`.

2. **Build SRPM and upload:**

   ```bash
   .packages/fedora/upload-copr.sh
   ```

   This script will:
   - Read the version from `muslimtify.spec`
   - Download the source tarball from `https://github.com/rizukirr/muslimtify/archive/v{version}/...`
     (or use a cached copy if already downloaded)
   - Create a temporary `rpmbuild` tree in `/tmp`
   - Run `rpmbuild -bs` to produce an SRPM
   - Copy the SRPM to `.packages/fedora/`
   - If `copr-cli` is installed, prompt you to upload
   - If `copr-cli` is not installed, print manual instructions

3. **Check build status:**

   https://copr.fedorainfracloud.org/coprs/rizukirr/muslimtify/builds/

### Building an RPM locally (for testing)

```bash
# On Fedora — install build dependencies
sudo dnf install cmake gcc pkgconfig libnotify-devel libcurl-devel rpm-build

# Build the SRPM first (answer N to the upload prompt)
.packages/fedora/upload-copr.sh

# Option 1: Build RPM from SRPM using mock (recommended — builds in a clean chroot)
sudo dnf install mock
sudo usermod -aG mock $USER  # then re-login
mock -r fedora-41-x86_64 .packages/fedora/muslimtify-*.src.rpm

# Option 2: Build directly with rpmbuild (uses your system, not a chroot)
rpmbuild --rebuild .packages/fedora/muslimtify-*.src.rpm
# Output goes to ~/rpmbuild/RPMS/x86_64/
```

### User installation

Once published, Fedora users install with:

```bash
sudo dnf copr enable rizukirr/muslimtify
sudo dnf install muslimtify
```

To uninstall:

```bash
muslimtify daemon uninstall            # remove the systemd timer first
sudo dnf remove muslimtify
sudo dnf copr disable rizukirr/muslimtify
```

### COPR update checklist

```
[ ] Update Version in muslimtify.spec
[ ] Reset Release to 1%{?dist}
[ ] Add new %changelog entry
[ ] Ensure GitHub release tag exists
[ ] Run upload-copr.sh
[ ] Verify build at https://copr.fedorainfracloud.org/coprs/rizukirr/muslimtify/builds/
```

### Troubleshooting

- **`rpmbuild` fails with "File not found":** The GitHub release tag doesn't exist yet.
  Create and push the tag first.
- **`copr-cli` auth error:** Regenerate your API token at https://copr.fedorainfracloud.org/api/
  and save it to `~/.config/copr`.
- **COPR build fails on missing deps:** Check that `BuildRequires` in the spec matches
  the Fedora package names. Use `dnf provides '*/libnotify.pc'` to find the right package.
- **COPR rejects duplicate build:** Unlike Launchpad, COPR allows rebuilding the same
  version. But if you need a new build with changes, bump `Release` (e.g., `1%{?dist}` →
  `2%{?dist}`).

---

## Full Release Checklist

Complete walkthrough for publishing a new version to all three platforms.

### 1. Prepare the release

```
[ ] Update version string in src/cli.c
[ ] Update version in CMakeLists.txt: project(muslimtify VERSION X.Y.Z ...)
[ ] Update .packages/aur/PKGBUILD (pkgver=X.Y.Z, pkgrel=1)
[ ] Update .packages/debian/debian/changelog (new entry at top, date -R for timestamp)
[ ] Update .packages/fedora/muslimtify.spec (Version, Release, %changelog)
[ ] Commit all changes
[ ] Tag and push: git tag vX.Y.Z && git push && git push --tags
[ ] Create GitHub release from the tag at https://github.com/rizukirr/muslimtify/releases/new
```

### 2. Publish to AUR

```
[ ] cd .packages/aur && updpkgsums
[ ] makepkg --printsrcinfo > .SRCINFO
[ ] Test: makepkg -si
[ ] Copy PKGBUILD, .SRCINFO, muslimtify.install to ~/aur-muslimtify
[ ] cd ~/aur-muslimtify && git add -A && git commit -m "Update to vX.Y.Z" && git push
```

### 3. Publish to Launchpad PPA

```
[ ] rm -f .packages/debian/muslimtify_*
[ ] sudo .packages/debian/upload-ppa.sh
[ ] Verify: https://launchpad.net/~rizukirr/+archive/ubuntu/muslimtify/+packages
```

### 4. Publish to Fedora COPR

```
[ ] rm -f .packages/fedora/muslimtify-*.src.rpm
[ ] .packages/fedora/upload-copr.sh
[ ] Verify: https://copr.fedorainfracloud.org/coprs/rizukirr/muslimtify/builds/
```

### 5. Post-release verification

```
[ ] Arch:   yay -S muslimtify (or paru -S muslimtify)
[ ] Ubuntu: sudo apt update && sudo apt install muslimtify
[ ] Fedora: sudo dnf copr enable rizukirr/muslimtify && sudo dnf install muslimtify
```
