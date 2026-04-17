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

#ifndef BADGEPREVIEWITEM_H
#define BADGEPREVIEWITEM_H

#include <QElapsedTimer>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QVector>

class QPainter;

class BadgePreviewItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool flash READ flash WRITE setFlash NOTIFY flashChanged)
    Q_PROPERTY(bool marquee READ marquee WRITE setMarquee NOTIFY marqueeChanged)
    Q_PROPERTY(int colorIndex READ colorIndex WRITE setColorIndex NOTIFY colorIndexChanged)
    Q_PROPERTY(int speedIndex READ speedIndex WRITE setSpeedIndex NOTIFY speedIndexChanged)
    Q_PROPERTY(int modeIndex READ modeIndex WRITE setModeIndex NOTIFY modeIndexChanged)

public:
    explicit BadgePreviewItem(QQuickItem *parent = nullptr);

    QString text() const;
    bool flash() const;
    bool marquee() const;
    int colorIndex() const;
    int speedIndex() const;
    int modeIndex() const;

    void setText(const QString &text);
    void setFlash(bool flash);
    void setMarquee(bool marquee);
    void setColorIndex(int colorIndex);
    void setSpeedIndex(int speedIndex);
    void setModeIndex(int modeIndex);

    void paint(QPainter *painter) override;

signals:
    void textChanged();
    void flashChanged();
    void marqueeChanged();
    void colorIndexChanged();
    void speedIndexChanged();
    void modeIndexChanged();

private:
    using Grid = QVector<QVector<bool>>;

    void rebuildSourceGrid();
    void renderFrame();
    void updateAnimationInterval();
    static Grid blankGrid(int width = 44);

    QString m_text;
    bool m_flash = false;
    bool m_marquee = false;
    int m_colorIndex = 0;
    int m_speedIndex = 0;
    int m_modeIndex = 0;
    int m_animationIndex = 0;
    Grid m_sourceGrid;
    Grid m_frameGrid;
    QElapsedTimer m_effectClock;
    QTimer m_timer;
};

#endif
