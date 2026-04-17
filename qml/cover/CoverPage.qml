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

CoverBackground {
    id: coverPage
    clip: true

    //% "Sending…"
    readonly property string sendingText: qsTrId("badgemagic-sailfish-la-sending")
    //% "Badge Magic"
    readonly property string titleText: qsTrId("badgemagic-sailfish-la-badge-magic")

    Image {
        anchors.fill: parent
        anchors.margins: -Theme.paddingLarge
        source: "./background.png"
        fillMode: Image.PreserveAspectCrop
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignVCenter
        smooth: true
        opacity: 0.9
    }

    Rectangle {
        anchors.fill: parent
        color: "#aa000000"
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - (Theme.paddingLarge * 2)
        horizontalAlignment: Text.AlignHCenter
        color: Theme.primaryColor
        font.pixelSize: Theme.fontSizeLarge
        font.bold: true
        text: badgeApp.busy ? coverPage.sendingText : coverPage.titleText
        wrapMode: Text.Wrap
    }
}
