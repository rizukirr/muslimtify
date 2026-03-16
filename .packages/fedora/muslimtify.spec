Name:           muslimtify
Version:        0.1.5
Release:        1%{?dist}
Summary:        Muslim prayer time notification daemon for linux
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
* Mon Mar 02 2026 Rizki Rakasiwi <rizkirr.xyz@gmail.com> - 0.1.4-1
- Release v0.1.4
