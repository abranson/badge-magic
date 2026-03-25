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
import "../components"

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
    //% "Select slots"
    readonly property string selectSlotsText: qsTrId("badgemagic-sailfish-la-select-slots")
    //% "Send selected slots"
    readonly property string sendSelectedSlotsText: qsTrId("badgemagic-sailfish-la-send-selected-slots")
    //% "Clear slot selection"
    readonly property string clearSlotSelectionText: qsTrId("badgemagic-sailfish-la-clear-slot-selection")
    //% "Add to slots"
    readonly property string addToSlotsText: qsTrId("badgemagic-sailfish-la-add-to-slots")
    //% "Remove from slots"
    readonly property string removeFromSlotsText: qsTrId("badgemagic-sailfish-la-remove-from-slots")
    //% "Tap badges to choose up to %1 message slots for button cycling."
    readonly property string slotSelectionHint: qsTrId("badgemagic-sailfish-la-slot-selection-hint")
    //% "%1 of %2 message slots selected"
    readonly property string slotSelectionStatus: qsTrId("badgemagic-sailfish-la-slot-selection-status")
    //% "Slot %1"
    readonly property string slotNumberText: qsTrId("badgemagic-sailfish-la-slot-number")
    property var editorPage
    property var selectedIndexes: []
    property bool selectionMode: false
    readonly property int maxSelectedSlots: 8
    readonly property int selectedCount: selectedIndexes.length

    function selectionPosition(badgeIndex) {
        for (var i = 0; i < selectedIndexes.length; ++i) {
            if (selectedIndexes[i] === badgeIndex) {
                return i
            }
        }

        return -1
    }

    function isSelected(badgeIndex) {
        return selectionPosition(badgeIndex) >= 0
    }

    function canSelect(badgeIndex) {
        return isSelected(badgeIndex) || selectedIndexes.length < maxSelectedSlots
    }

    function toggleSelection(badgeIndex) {
        var nextSelection = selectedIndexes.slice(0)
        var position = selectionPosition(badgeIndex)

        if (position >= 0) {
            nextSelection.splice(position, 1)
        } else if (nextSelection.length < maxSelectedSlots) {
            nextSelection.push(badgeIndex)
        }

        selectedIndexes = nextSelection
    }

    function clearSelection() {
        selectedIndexes = []
        selectionMode = false
    }

    function sendSelectedSlots() {
        if (selectedIndexes.length === 0 || badgeApp.busy) {
            return
        }

        badgeApp.sendSavedBadgeSlots(selectedIndexes)
        clearSelection()
    }

    SilicaListView {
        anchors.fill: parent
        model: badgeApp.savedBadges
        PullDownMenu {
            MenuItem {
                text: page.sendSelectedSlotsText
                visible: page.selectedCount > 0
                enabled: !badgeApp.busy
                onClicked: page.sendSelectedSlots()
            }
            MenuItem {
                text: page.clearSlotSelectionText
                visible: page.selectionMode || page.selectedCount > 0
                onClicked: page.clearSelection()
            }
            MenuItem {
                text: page.selectSlotsText
                visible: !page.selectionMode && badgeApp.savedBadges.length > 0
                onClicked: page.selectionMode = true
            }
        }
        header: Column {
            width: page.width

            PageHeader {
                title: page.savedBadgesText
            }

            Label {
                width: page.width - (Theme.horizontalPageMargin * 2)
                x: Theme.horizontalPageMargin
                visible: page.selectionMode || page.selectedCount > 0
                color: Theme.secondaryHighlightColor
                text: page.selectedCount > 0
                      ? page.slotSelectionStatus.arg(page.selectedCount).arg(page.maxSelectedSlots)
                      : page.slotSelectionHint.arg(page.maxSelectedSlots)
                wrapMode: Text.Wrap
            }

            BadgeStatusDisplay {
                width: page.width - (Theme.horizontalPageMargin * 2)
                x: Theme.horizontalPageMargin
                statusMessage: badgeApp.statusMessage
                lastError: badgeApp.lastError
            }
        }
        ViewPlaceholder {
            enabled: badgeApp.savedBadges.length === 0
            text: page.noSavedBadgesText
            hintText: page.noSavedBadgesHint
        }
        delegate: ListItem {
            id: delegateItem
            width: ListView.view.width
            contentHeight: contentColumn.height + (Theme.paddingLarge * 2)

            onClicked: {
                if (page.selectionMode) {
                    if (page.canSelect(index)) {
                        page.toggleSelection(index)
                    }
                    return
                }

                if (badgeApp.busy) {
                    return
                }

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
                    enabled: !badgeApp.busy
                    onClicked: badgeApp.sendSavedBadge(index)
                }
                MenuItem {
                    text: page.isSelected(index) ? page.removeFromSlotsText : page.addToSlotsText
                    enabled: page.canSelect(index)
                    onClicked: {
                        page.selectionMode = true
                        page.toggleSelection(index)
                    }
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
                    onClicked: {
                        if (badgeApp.deleteSavedBadge(index)) {
                            page.clearSelection()
                        }
                    }
                }
            }

            Column {
                id: contentColumn
                width: parent.width - (Theme.horizontalPageMargin * 2)
                       - (slotLabel.visible ? slotLabel.implicitWidth + Theme.paddingMedium : 0)
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

            Label {
                id: slotLabel
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                visible: page.isSelected(index)
                color: Theme.highlightColor
                text: page.slotNumberText.arg(page.selectionPosition(index) + 1)
            }
        }
    }
}
