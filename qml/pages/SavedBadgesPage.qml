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

    //% "Saved badges"
    readonly property string savedBadgesText: qsTrId("badgemagic-sailfish-la-saved-badges")
    //% "No saved badges yet"
    readonly property string noSavedBadgesText: qsTrId("badgemagic-sailfish-la-no-saved-badges")
    //% "Save a text badge from the main page to reuse it here."
    readonly property string noSavedBadgesHint: qsTrId("badgemagic-sailfish-la-no-saved-badges-hint")
    //% "Send to badge"
    readonly property string sendToBadgeText: qsTrId("badgemagic-sailfish-la-send-to-badge")
    //% "Load into editor"
    readonly property string loadIntoEditorText: qsTrId("badgemagic-sailfish-la-load-into-editor")
    //% "Delete"
    readonly property string deleteText: qsTrId("badgemagic-sailfish-la-delete")
    //% "Mode %1 • Speed %2"
    readonly property string modeSpeedFormat: qsTrId("badgemagic-sailfish-la-mode-speed-format")
    //% "Flash"
    readonly property string flashText: qsTrId("badgemagic-sailfish-la-flash")
    //% "No flash"
    readonly property string noFlashText: qsTrId("badgemagic-sailfish-la-no-flash")
    //% "Marquee"
    readonly property string marqueeText: qsTrId("badgemagic-sailfish-la-marquee")
    //% "No marquee"
    readonly property string noMarqueeText: qsTrId("badgemagic-sailfish-la-no-marquee")

    property var editorPage

    SilicaListView {
        anchors.fill: parent
        model: badgeApp.savedBadges
        header: PageHeader {
            title: page.savedBadgesText
        }
        ViewPlaceholder {
            enabled: badgeApp.savedBadges.length === 0
            text: page.noSavedBadgesText
            hintText: page.noSavedBadgesHint
        }
        delegate: ListItem {
            id: delegateItem
            width: ListView.view.width
            height: contentColumn.height + (Theme.paddingLarge * 2)

            onClicked: {
                if (editorPage && modelData.hasRawText) {
                    editorPage.applySavedBadge(modelData)
                    pageStack.pop()
                } else {
                    badgeApp.sendSavedBadge(index)
                }
            }

            menu: ContextMenu {
                MenuItem {
                    text: page.sendToBadgeText
                    onClicked: badgeApp.sendSavedBadge(index)
                }
                MenuItem {
                    text: page.loadIntoEditorText
                    enabled: modelData.hasRawText && editorPage
                    onClicked: {
                        if (editorPage && modelData.hasRawText) {
                            editorPage.applySavedBadge(modelData)
                            pageStack.pop()
                        }
                    }
                }
                MenuItem {
                    text: page.deleteText
                    onClicked: badgeApp.deleteSavedBadge(index)
                }
            }

            Column {
                id: contentColumn
                width: parent.width - (Theme.horizontalPageMargin * 2)
                x: Theme.horizontalPageMargin
                y: Theme.paddingLarge
                spacing: Theme.paddingSmall

                Label {
                    width: parent.width
                    text: modelData.name
                    truncationMode: TruncationMode.Fade
                }

                Label {
                    width: parent.width
                    color: Theme.secondaryColor
                    visible: modelData.rawText.length > 0
                    text: modelData.rawText
                    truncationMode: TruncationMode.Fade
                }

                Label {
                    width: parent.width
                    color: Theme.secondaryHighlightColor
                    text: page.modeSpeedFormat.arg(modelData.modeLabel).arg(modelData.speedLabel)
                }

                Label {
                    width: parent.width
                    color: Theme.secondaryColor
                    text: (modelData.flash ? page.flashText : page.noFlashText)
                          + " • "
                          + (modelData.marquee ? page.marqueeText : page.noMarqueeText)
                }
            }
        }
    }
}
