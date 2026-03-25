/*
 * SPDX-FileCopyrightText: 2026 Badge Magic for SailfishOS contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Badge Magic for SailfishOS contributors
 *
 * Based on the original Badge Magic application by FOSSASIA.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    //% "About"
    readonly property string aboutText: qsTrId("badgemagic-sailfish-la-about")
    //% "Badge Magic for SailfishOS is a native SailfishOS port based on the original Badge Magic application by FOSSASIA and its LED badge protocol."
    readonly property string descriptionText: qsTrId("badgemagic-sailfish-la-about-description")
    //% "The Sailfish target focuses on composing text badges, saving JSON presets, and sending them over BLE to badges advertising service FEE0 and writable characteristic FEE1. Saved badges can also be transferred together into the badge's message slots for hardware button cycling."
    readonly property string protocolText: qsTrId("badgemagic-sailfish-la-about-protocol")
    //% "Credits to FOSSASIA for the original Badge Magic application, the badge protocol implementation, and the upstream project on which this SailfishOS port is based."
    readonly property string acknowledgementsText: qsTrId("badgemagic-sailfish-la-about-acknowledgements")
    //% "FOSSASIA repository: %1"
    readonly property string repositoryText: qsTrId("badgemagic-sailfish-la-about-fossasia-repository")
    //% "Version %1"
    readonly property string versionText: qsTrId("badgemagic-sailfish-la-about-version")

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height + Theme.paddingLarge

        Column {
            id: contentColumn
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: page.aboutText
            }

            Image {
                width: Math.min(parent.width - (Theme.horizontalPageMargin * 4), Theme.itemSizeHuge * 2)
                height: width
                anchors.horizontalCenter: parent.horizontalCenter
                source: "../cover/background.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            Label {
                width: parent.width - (Theme.horizontalPageMargin * 2)
                x: Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                text: page.descriptionText
            }

            Label {
                width: parent.width - (Theme.horizontalPageMargin * 2)
                x: Theme.horizontalPageMargin
                color: Theme.highlightColor
                text: page.versionText.arg(appVersion)
                wrapMode: Text.Wrap
            }

            Label {
                width: parent.width - (Theme.horizontalPageMargin * 2)
                x: Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                text: page.protocolText
            }

            Label {
                width: parent.width - (Theme.horizontalPageMargin * 2)
                x: Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                text: page.acknowledgementsText
            }

            LinkedLabel {
                width: parent.width - (Theme.horizontalPageMargin * 2)
                x: Theme.horizontalPageMargin
                plainText: page.repositoryText.arg("https://github.com/fossasia/badgemagic-app")
                color: Theme.secondaryHighlightColor
            }
        }
    }
}
