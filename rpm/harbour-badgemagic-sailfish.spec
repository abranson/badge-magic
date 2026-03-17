Name:       harbour-badgemagic-sailfish
Summary:    Badge Magic for SailfishOS
Version:    0.1.0
Release:    1
License:    Apache-2.0
Group:      Qt/Qt
URL:        https://github.com/abranson/badge-magic
Source0:    %{name}-%{version}.tar.bz2

BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(KF5BluezQt)
BuildRequires:  pkgconfig(sailfishapp)
Requires:       sailfishsilica-qt5 >= 0.10.9

%description
Native SailfishOS front-end for Badge Magic. The application composes text
badges, stores Badge Magic JSON presets, and transfers the generated payload
over Bluetooth Low Energy.

%prep
%autosetup

%build
%qmake5
%make_build

%install
%qmake5_install

%files
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/86x86/apps/%{name}.png
%{_datadir}/icons/hicolor/108x108/apps/%{name}.png
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%{_datadir}/icons/hicolor/172x172/apps/%{name}.png
