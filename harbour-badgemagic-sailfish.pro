#
# SPDX-FileCopyrightText: 2026 Andrew Branson
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 Andrew Branson
#
# Based on the original Badge Magic application by FOSSASIA.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

TARGET = harbour-badgemagic-sailfish
VERSION = 0.4.0

QT += dbus qml quick

CONFIG += link_pkgconfig

CONFIG += sailfishapp c++17
CONFIG += sailfishapp_i18n
CONFIG += sailfishapp_i18n_idbased
CONFIG += sailfishapp_i18n_unfinished
CONFIG += warn_on

SAILFISHAPP_ICONS += 86x86 108x108 128x128 172x172

include(qble/qble.pri)

SOURCES += \
    main.cpp \
    src/badgeapp.cpp \
    src/badgeblemanager.cpp \
    src/badgeencoder.cpp \
    src/badgepreviewitem.cpp \
    src/badgestore.cpp

HEADERS += \
    src/badgeapp.h \
    src/badgeblemanager.h \
    src/badgeencoder.h \
    src/badgepreviewitem.h \
    src/badgestore.h

INCLUDEPATH += src qble
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

DISTFILES += \
    harbour-badgemagic-sailfish.desktop \
    icons/86x86/harbour-badgemagic-sailfish.png \
    icons/108x108/harbour-badgemagic-sailfish.png \
    icons/128x128/harbour-badgemagic-sailfish.png \
    icons/172x172/harbour-badgemagic-sailfish.png \
    qml/harbour-badgemagic-sailfish.qml \
    qml/components/BadgeStatusDisplay.qml \
    qml/cover/background.png \
    qml/pages/AboutPage.qml \
    qml/pages/MainPage.qml \
    qml/pages/SavedBadgesPage.qml \
    qml/cover/CoverPage.qml \
    rpm/harbour-badgemagic-sailfish.spec \
    rpm/harbour-badgemagic-sailfish.changes \
    translations/harbour-badgemagic-sailfish.ts \
    README.md

TRANSLATIONS += \
    translations/harbour-badgemagic-sailfish.ts \
    translations/harbour-badgemagic-sailfish-en_GB.ts \
