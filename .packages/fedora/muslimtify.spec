Name:           muslimtify
Version:        0.2.0
Release:        1%{?dist}
Summary:        An Islamic prayer time notification daemon for Linux
License:        MIT
URL:            https://github.com/rizukirr/muslimtify
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.22
BuildRequires:  gcc
BuildRequires:  pkgconfig(libnotify)
BuildRequires:  pkgconfig(libcurl)

Requires:       libnotify
Requires:       libcurl

%description
Muslimtify is a lightweight CLI tool that sends desktop notifications
for Islamic prayer times with customizable reminders. It integrates
with systemd user timers and supports per-prayer configuration.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
%cmake_build

%install
%cmake_install

%post
echo ""
echo "  Run 'muslimtify daemon install' to enable the prayer time daemon."
echo ""

%files
%license LICENSE
%{_bindir}/muslimtify
%{_datadir}/icons/hicolor/128x128/apps/muslimtify.png
%{_datadir}/pixmaps/muslimtify.png
%{_prefix}/lib/systemd/user/muslimtify.service
%{_prefix}/lib/systemd/user/muslimtify.timer

%changelog
* Fri Mar 27 2026 Rizki Rakasiwi <rizkirr.xyz@gmail.com> - 0.2.0-1
- Add full Windows support for Muslimtify, including toast notifications with icon, Task Scheduler daemon, service helper, install/uninstall scripts, and Inno Setup installer for winget distribution
- Add 23 international prayer time calculation methods (MWL, Makkah, ISNA, Egypt, and more)
- Add muslimtify method command for calculation method management
- Add fajr_angle/isha_angle config fields for custom method parameters
- Display full method name in config output
- Add platform abstraction layer (Linux/Windows)
- Extract shared check cycle, decouple from CLI
- Route display, persistence, and system calls through platform layer
- Downgrade from C23 to C99 for MSVC compatibility
- Add comprehensive multi-method validation (~108 data points)
- Add unit tests for json.h, platform boundary, and Windows components

* Mon Mar 02 2026 Rizki Rakasiwi <rizkirr.xyz@gmail.com> - 0.1.4-1
- Release v0.1.4
