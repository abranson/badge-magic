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
import Harbour.BadgeMagic 1.0
import "../components"

Page {
    id: page

    //% "Left"
    readonly property string modeLeftText: qsTrId("badgemagic-sailfish-la-mode-left")
    //% "Right"
    readonly property string modeRightText: qsTrId("badgemagic-sailfish-la-mode-right")
    //% "Up"
    readonly property string modeUpText: qsTrId("badgemagic-sailfish-la-mode-up")
    //% "Down"
    readonly property string modeDownText: qsTrId("badgemagic-sailfish-la-mode-down")
    //% "Fixed"
    readonly property string modeFixedText: qsTrId("badgemagic-sailfish-la-mode-fixed")
    //% "Snowflake"
    readonly property string modeSnowflakeText: qsTrId("badgemagic-sailfish-la-mode-snowflake")
    //% "Picture"
    readonly property string modePictureText: qsTrId("badgemagic-sailfish-la-mode-picture")
    //% "Animation"
    readonly property string modeAnimationText: qsTrId("badgemagic-sailfish-la-mode-animation")
    //% "Laser"
    readonly property string modeLaserText: qsTrId("badgemagic-sailfish-la-mode-laser")
    //% "Saved badges"
    readonly property string savedBadgesText: qsTrId("badgemagic-sailfish-la-saved-badges")
    //% "About"
    readonly property string aboutText: qsTrId("badgemagic-sailfish-la-about")
    //% "Badge Magic"
    readonly property string badgeMagicText: qsTrId("badgemagic-sailfish-la-badge-magic")
    //% "Badge text"
    readonly property string badgeTextLabel: qsTrId("badgemagic-sailfish-la-badge-text")
    //% "Type the text to send to the LED badge"
    readonly property string badgeTextPlaceholder: qsTrId("badgemagic-sailfish-la-badge-text-placeholder")
    //% "Save as"
    readonly property string saveAsText: qsTrId("badgemagic-sailfish-la-save-as")
    //% "Optional badge preset name"
    readonly property string saveAsPlaceholder: qsTrId("badgemagic-sailfish-la-save-as-placeholder")
    //% "Flash"
    readonly property string flashText: qsTrId("badgemagic-sailfish-la-flash")
    //% "Blink the message while it scrolls or animates."
    readonly property string flashDescription: qsTrId("badgemagic-sailfish-la-flash-description")
    //% "Marquee"
    readonly property string marqueeText: qsTrId("badgemagic-sailfish-la-marquee")
    //% "Add a marquee border effect around the message."
    readonly property string marqueeDescription: qsTrId("badgemagic-sailfish-la-marquee-description")
    //% "Speed"
    readonly property string speedText: qsTrId("badgemagic-sailfish-la-speed")
    //% "Mode"
    readonly property string modeText: qsTrId("badgemagic-sailfish-la-mode")
    //% "Preview color"
    readonly property string previewColorText: qsTrId("badgemagic-sailfish-la-preview-color")
    //% "Red"
    readonly property string previewColorRedText: qsTrId("badgemagic-sailfish-la-preview-color-red")
    //% "Green"
    readonly property string previewColorGreenText: qsTrId("badgemagic-sailfish-la-preview-color-green")
    //% "Blue"
    readonly property string previewColorBlueText: qsTrId("badgemagic-sailfish-la-preview-color-blue")
    //% "White"
    readonly property string previewColorWhiteText: qsTrId("badgemagic-sailfish-la-preview-color-white")
    //% "Sending…"
    readonly property string sendingText: qsTrId("badgemagic-sailfish-la-sending")
    //% "Send to badge"
    readonly property string sendToBadgeText: qsTrId("badgemagic-sailfish-la-send-to-badge")
    //% "Save badge"
    readonly property string saveBadgeText: qsTrId("badgemagic-sailfish-la-save-badge")
    //% "Badge preview"
    readonly property string badgePreviewText: qsTrId("badgemagic-sailfish-la-badge-preview")
    property var modeLabels: [
        modeLeftText,
        modeRightText,
        modeUpText,
        modeDownText,
        modeFixedText,
        modeSnowflakeText,
        modePictureText,
        modeAnimationText,
        modeLaserText
    ]
    property var speedLabels: ["1", "2", "3", "4", "5", "6", "7", "8"]
    property var previewColorLabels: [
        previewColorRedText,
        previewColorGreenText,
        previewColorBlueText,
        previewColorWhiteText
    ]
    readonly property bool stickyPreviewActive: formFlickable.contentY > previewSection.y

    function applySavedBadge(draft) {
        if (!draft || !draft.hasRawText) {
            return
        }

        messageField.text = draft.rawText
        saveNameField.text = draft.name
        flashSwitch.checked = draft.flash
        marqueeSwitch.checked = draft.marquee
        speedBox.currentIndex = draft.speedIndex
        modeBox.currentIndex = draft.modeIndex
    }

    SilicaFlickable {
        id: formFlickable
        anchors.fill: parent
        contentHeight: contentColumn.height + Theme.paddingLarge
        contentWidth: width

        PullDownMenu {
            MenuItem {
                text: page.savedBadgesText
                onClicked: pageStack.push(Qt.resolvedUrl("SavedBadgesPage.qml"), { editorPage: page })
            }
            MenuItem {
                text: page.aboutText
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
        }

        Column {
            id: contentColumn
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: page.badgeMagicText
            }

            Item {
                id: previewSection
                width: parent.width
                height: previewInFlow.height

                BadgePreviewItem {
                    id: previewInFlow
                    width: parent.width - (Theme.horizontalPageMargin * 2)
                    height: width / 3.2
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: !page.stickyPreviewActive
                    text: messageField.text
                    flash: flashSwitch.checked
                    marquee: marqueeSwitch.checked
                    colorIndex: badgeApp.previewColorIndex
                    speedIndex: speedBox.currentIndex
                    modeIndex: modeBox.currentIndex
                }
            }

            TextArea {
                id: messageField
                width: parent.width
                label: page.badgeTextLabel
                placeholderText: page.badgeTextPlaceholder
                wrapMode: TextEdit.Wrap
            }

            TextField {
                id: saveNameField
                width: parent.width
                label: page.saveAsText
                placeholderText: page.saveAsPlaceholder
            }

            TextSwitch {
                id: flashSwitch
                text: page.flashText
                description: page.flashDescription
            }

            TextSwitch {
                id: marqueeSwitch
                text: page.marqueeText
                description: page.marqueeDescription
            }

            ComboBox {
                id: speedBox
                width: parent.width
                label: page.speedText
                currentIndex: 0
                menu: ContextMenu {
                    Repeater {
                        model: speedLabels
                        MenuItem {
                            text: modelData
                        }
                    }
                }
            }

            ComboBox {
                id: modeBox
                width: parent.width
                label: page.modeText
                currentIndex: 0
                menu: ContextMenu {
                    Repeater {
                        model: modeLabels
                        MenuItem {
                            text: modelData
                        }
                    }
                }
            }

            ComboBox {
                id: previewColorBox
                width: parent.width
                label: page.previewColorText
                currentIndex: badgeApp.previewColorIndex
                onCurrentIndexChanged: {
                    if (badgeApp.previewColorIndex !== currentIndex) {
                        badgeApp.previewColorIndex = currentIndex
                    }
                }
                menu: ContextMenu {
                    Repeater {
                        model: previewColorLabels
                        MenuItem {
                            text: modelData
                        }
                    }
                }
            }

            Button {
                width: parent.width - (Theme.horizontalPageMargin * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                text: badgeApp.busy ? page.sendingText : page.sendToBadgeText
                enabled: !badgeApp.busy
                onClicked: badgeApp.sendTextBadge(
                               messageField.text,
                               flashSwitch.checked,
                               marqueeSwitch.checked,
                               speedBox.currentIndex,
                               modeBox.currentIndex)
            }

            Button {
                width: parent.width - (Theme.horizontalPageMargin * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                text: page.saveBadgeText
                enabled: !badgeApp.busy
                onClicked: {
                    if (badgeApp.saveTextBadge(
                                saveNameField.text,
                                messageField.text,
                                flashSwitch.checked,
                                marqueeSwitch.checked,
                                speedBox.currentIndex,
                                modeBox.currentIndex)) {
                        if (saveNameField.text.length === 0) {
                            saveNameField.text = messageField.text.trim().slice(0, 24)
                        }
                    }
                }
            }

            BadgeStatusDisplay {
                width: parent.width - (Theme.horizontalPageMargin * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                statusMessage: badgeApp.statusMessage
                lastError: badgeApp.lastError
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }
    }

    Item {
        id: stickyPreviewContainer
        visible: page.stickyPreviewActive
        width: parent.width
        height: stickyPreview.height
        z: 2

        BadgePreviewItem {
            id: stickyPreview
            width: parent.width - (Theme.horizontalPageMargin * 2)
            height: width / 3.2
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            text: messageField.text
            flash: flashSwitch.checked
            marquee: marqueeSwitch.checked
            colorIndex: badgeApp.previewColorIndex
            speedIndex: speedBox.currentIndex
            modeIndex: modeBox.currentIndex
        }
    }
}
