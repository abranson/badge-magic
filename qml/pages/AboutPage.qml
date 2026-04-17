/*
 * SPDX-FileCopyrightText: 2026 Andrew Branson
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Andrew Branson
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
    //% "Credits to FOSSASIA for the original Badge Magic application, the badge protocol implementation, and the upstream project on which this SailfishOS port is based."
    readonly property string acknowledgementsText: qsTrId("badgemagic-sailfish-la-about-acknowledgements")
    //% "FOSSASIA repository: %1"
    readonly property string repositoryText: qsTrId("badgemagic-sailfish-la-about-fossasia-repository")
    //% "Version %1"
    readonly property string versionText: qsTrId("badgemagic-sailfish-la-about-version")
    //% "BADGE MAGIC"
    readonly property string heroTitleText: qsTrId("badgemagic-sailfish-la-about-hero-title")
    //% "SFOS Edition"
    readonly property string heroSubtitleText: qsTrId("badgemagic-sailfish-la-about-hero-subtitle")

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

            Item {
                id: heroBlock

                width: Math.min(parent.width - (Theme.horizontalPageMargin * 4), Theme.itemSizeHuge * 2)
                height: width
                anchors.horizontalCenter: parent.horizontalCenter

                Image {
                    anchors.fill: parent
                    source: "../cover/background.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }

                Item {
                    id: heroText

                    width: page.width
                    height: heroTextColumn.height
                    anchors.centerIn: parent
                    transformOrigin: Item.Center

                    SequentialAnimation on rotation {
                        loops: Animation.Infinite
                        running: page.status === PageStatus.Active

                        NumberAnimation {
                            from: -3.5
                            to: 3.5
                            duration: 900
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            from: 3.5
                            to: -2.5
                            duration: 1000
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            from: -2.5
                            to: 2
                            duration: 850
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            from: 2
                            to: -3.5
                            duration: 1100
                            easing.type: Easing.InOutSine
                        }
                    }

                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        running: page.status === PageStatus.Active

                        NumberAnimation {
                            from: 0.98
                            to: 1.03
                            duration: 1100
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            from: 1.03
                            to: 0.99
                            duration: 1200
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            from: 0.99
                            to: 0.98
                            duration: 900
                            easing.type: Easing.InOutSine
                        }
                    }

                    Column {
                        id: heroTextColumn

                        anchors.centerIn: parent
                        width: parent.width
                        spacing: Theme.paddingSmall / 3

                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: page.heroTitleText
                            color: "#fff46b"
                            style: Text.Outline
                            styleColor: "#f03a7f"
                            fontSizeMode: Text.HorizontalFit
                            minimumPixelSize: Theme.fontSizeLarge
                            font.bold: true
                            font.pixelSize: Math.round(heroText.width * 0.23)
                            font.letterSpacing: 1.2
                        }

                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: page.heroSubtitleText
                            color: "#7ffbff"
                            style: Text.Outline
                            styleColor: "#00466e"
                            fontSizeMode: Text.HorizontalFit
                            minimumPixelSize: Theme.fontSizeMedium
                            font.bold: true
                            font.pixelSize: Math.round(heroText.width * 0.12)
                            font.letterSpacing: 0.8
                        }
                    }
                }
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
