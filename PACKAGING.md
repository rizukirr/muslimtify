# Packaging and Distribution

This document outlines the plan for future package distribution.

## Current Status

✅ **Source distribution** - Available via git clone  
❌ **Binary packages** - Not yet available  
❌ **Repository hosting** - Planned for future  

## Planned Distribution Methods

### 1. Arch Linux (AUR) [DONE]

**Package name:** `muslimtify`

**Files needed:**
- `PKGBUILD` - Build script for Arch Linux
- `.SRCINFO` - Package metadata

**Installation method:**
```bash
# Via AUR helper (yay, paru, etc.)
yay -S muslimtify

# Manual
git clone https://aur.archlinux.org/muslimtify-git.git
cd muslimtify-git
makepkg -si
```

**Status:** 📋 Planned

### 2. Debian/Ubuntu (.deb)

**Package name:** `muslimtify`

**Files needed:**
- `debian/control` - Package metadata
- `debian/rules` - Build rules
- `debian/changelog` - Version history
- `debian/copyright` - License info

**Installation method:**
```bash
# Via PPA (once available)
sudo add-apt-repository ppa:username/muslimtify
sudo apt update
sudo apt install muslimtify

# Manual .deb file
sudo dpkg -i muslimtify_1.0.0_amd64.deb
sudo apt-get install -f
```

**Status:** 📋 Planned

### 3. Fedora/RHEL (.rpm)

**Package name:** `muslimtify`

**Files needed:**
- `muslimtify.spec` - RPM spec file

**Installation method:**
```bash
# Via COPR (once available)
sudo dnf copr enable username/muslimtify
sudo dnf install muslimtify

# Manual .rpm file
sudo dnf install muslimtify-1.0.0-1.fc39.x86_64.rpm
```

**Status:** 📋 Planned

### 4. Flatpak

**Application ID:** `io.github.username.muslimtify`

**Files needed:**
- `io.github.username.muslimtify.yml` - Flatpak manifest
- Desktop file
- AppStream metadata

**Installation method:**
```bash
flatpak install flathub io.github.username.muslimtify
```

**Status:** 📋 Planned (low priority)

### 5. Snap

**Package name:** `muslimtify`

**Files needed:**
- `snap/snapcraft.yaml` - Snap definition

**Installation method:**
```bash
sudo snap install muslimtify
```

**Status:** 📋 Planned (low priority)

## Dependencies

### Runtime Dependencies (Required)
- `libnotify` (>= 0.7.0) - Desktop notifications
- `libcurl` (>= 7.0.0) - HTTP requests for location
- `glibc` (>= 2.31) - Standard C library
- `systemd` - Optional, for timer functionality

### Build Dependencies (Source installation only)
- `gcc` or `clang` (C23 support)
- `cmake` (>= 3.10)
- `pkg-config`
- `make`
- Development headers: `libnotify-dev`, `libcurl-dev`

## Package Metadata

### Basic Information
- **Name:** muslimtify
- **Version:** 0.1.0
- **License:** MIT
- **Homepage:** https://github.com/yourusername/muslimtify
- **Description:** Prayer time notification daemon with customizable reminders
- **Category:** Utility, Religion
- **Keywords:** prayer, islam, notification, daemon, adhan

### Package Size Estimates
- Binary: ~100 KB
- Source: ~200 KB
- Installed: ~300 KB

### System Integration
- Binary location: `/usr/bin/muslimtify` or `/usr/local/bin/muslimtify`
- Config: `~/.config/muslimtify/config.json` (per-user)
- Systemd service: `~/.config/systemd/user/muslimtify.{service,timer}`
- No system-wide services or root required

## Release Checklist

Before creating packages, ensure:

- [ ] Version number updated in:
  - [ ] `CMakeLists.txt`
  - [ ] `src/cli.c` (version command)
  - [ ] `README.md`
- [ ] All tests passing
- [ ] Documentation up to date
- [ ] CHANGELOG.md updated
- [ ] Git tag created: `git tag -a v1.0.0 -m "Release v1.0.0"`
- [ ] Source tarball created
- [ ] Checksums generated (SHA256)

## Building Packages

### AUR Package (PKGBUILD)

```bash
# pkgname=muslimtify-git
# pkgver=1.0.0
# pkgrel=1
# pkgdesc="Prayer time notification daemon with customizable reminders"
# arch=('x86_64')
# url="https://github.com/yourusername/muslimtify"
# license=('MIT')
# depends=('libnotify' 'curl' 'systemd')
# makedepends=('git' 'cmake')
# source=("git+$url.git")
# sha256sums=('SKIP')

build() {
  cd "$srcdir/muslimtify"
  mkdir -p build && cd build
  cmake .. -DCMAKE_INSTALL_PREFIX=/usr
  make
}

package() {
  cd "$srcdir/muslimtify/build"
  make DESTDIR="$pkgdir" install
}
```

### Debian Package

```bash
# Create debian directory structure
mkdir -p debian

# Create control file
cat > debian/control <<EOF
Source: muslimtify
Section: misc
Priority: optional
Maintainer: Your Name <email@example.com>
Build-Depends: debhelper (>= 11), cmake, pkg-config, libnotify-dev, libcurl4-openssl-dev
Standards-Version: 4.5.0
Homepage: https://github.com/yourusername/muslimtify

Package: muslimtify
Architecture: any
Depends: \${shlibs:Depends}, \${misc:Depends}, libnotify4, libcurl4
Description: Prayer time notification daemon
 Muslimtify is a lightweight daemon that provides desktop notifications
 for Islamic prayer times with customizable reminders.
EOF

# Build package
dpkg-buildpackage -us -uc -b
```

### RPM Package

```bash
# Create spec file structure
mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Create muslimtify.spec
cat > ~/rpmbuild/SPECS/muslimtify.spec <<'EOF'
Name:           muslimtify
Version:        1.0.0
Release:        1%{?dist}
Summary:        Prayer time notification daemon

License:        MIT
URL:            https://github.com/yourusername/muslimtify
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc cmake pkg-config libnotify-devel libcurl-devel
Requires:       libnotify libcurl systemd

%description
Muslimtify is a lightweight daemon that provides desktop notifications
for Islamic prayer times with customizable reminders.

%prep
%autosetup

%build
mkdir build && cd build
%cmake ..
%make_build

%install
cd build
%make_install

%files
%{_bindir}/muslimtify
%doc README.md
%license LICENSE

%changelog
* Sun Feb 23 2026 Your Name <email@example.com> - 1.0.0-1
- Initial release
EOF

# Build RPM
rpmbuild -ba ~/rpmbuild/SPECS/muslimtify.spec
```

## Distribution Hosting

### GitHub Releases
- Upload source tarball
- Upload checksums
- Upload binary packages (.deb, .rpm)
- Include release notes

### AUR
- Submit PKGBUILD to AUR
- Maintain package updates

### COPR (Fedora)
- Create COPR repository
- Set up automated builds

### PPA (Ubuntu)
- Create Launchpad PPA
- Upload package sources

## Installation Statistics

Once distributed, track:
- Download counts
- Popular distributions
- User feedback
- Bug reports

## Future Improvements

- [ ] Automated package building via CI/CD
- [ ] Multi-architecture support (ARM, ARM64)
- [ ] Signed packages
- [ ] Repository mirroring
- [ ] Auto-update mechanism
- [ ] Version check command

## Contributing Packages

If you want to help package muslimtify for a distribution:

1. Fork the repository
2. Create packaging files for your distribution
3. Test the package thoroughly
4. Submit a pull request
5. Maintain the package going forward

## Support

For packaging questions:
- Open an issue on GitHub
- Tag with `packaging` label
- Provide distribution details

---

**Note:** This document will be updated as packages become available.
