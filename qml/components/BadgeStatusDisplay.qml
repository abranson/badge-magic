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

Column {
    property string statusMessage
    property string lastError

    spacing: Theme.paddingSmall

    Label {
        width: parent.width
        color: Theme.highlightColor
        visible: parent.statusMessage.length > 0
        text: parent.statusMessage
        wrapMode: Text.Wrap
    }

    Label {
        width: parent.width
        color: Theme.errorColor
        visible: parent.lastError.length > 0
        text: parent.lastError
        wrapMode: Text.Wrap
    }
}
