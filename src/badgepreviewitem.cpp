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

#include "badgepreviewitem.h"

#include <QColor>
#include <QPainter>

#include "badgeencoder.h"

namespace {

using Grid = QVector<QVector<bool>>;

constexpr int kBadgeHeight = 11;
constexpr int kBadgeWidth = 44;
constexpr int kModeCount = 9;
constexpr int kColorCount = 4;
constexpr int kSpeedCount = 8;
constexpr int kBaseAnimationIntervalMs = 200;
constexpr int kAnimationIntervalStepMs = 25;
constexpr int kMarqueeStepMs = 100;
constexpr int kFlashToggleMs = 500;

int safeModeIndex(int modeIndex)
{
    return qBound(0, modeIndex, kModeCount - 1);
}

int safeSpeedIndex(int speedIndex)
{
    return qBound(0, speedIndex, kSpeedCount - 1);
}

int safeColorIndex(int colorIndex)
{
    return qBound(0, colorIndex, kColorCount - 1);
}

int animationIntervalMs(int speedIndex)
{
    return kBaseAnimationIntervalMs - (safeSpeedIndex(speedIndex) * kAnimationIntervalStepMs);
}

Grid blankPreviewGrid(int width = kBadgeWidth)
{
    const int safeWidth = width > 0 ? width : kBadgeWidth;
    return Grid(kBadgeHeight, QVector<bool>(safeWidth, false));
}

int positiveModulo(int value, int modulus)
{
    const int remainder = value % modulus;
    return remainder < 0 ? remainder + modulus : remainder;
}

QColor ledColorForIndex(int colorIndex)
{
    switch (safeColorIndex(colorIndex)) {
    case 0:
        return QColor(QStringLiteral("#FF3B30"));
    case 1:
        return QColor(QStringLiteral("#34C759"));
    case 2:
        return QColor(QStringLiteral("#0A84FF"));
    case 3:
    default:
        return QColor(QStringLiteral("#F5F5F7"));
    }
}

Grid encodedTextToGrid(const QStringList &encodedText)
{
    if (encodedText.isEmpty()) {
        return blankPreviewGrid();
    }

    Grid grid(kBadgeHeight, QVector<bool>(encodedText.size() * 8, false));

    for (int glyphIndex = 0; glyphIndex < encodedText.size(); ++glyphIndex) {
        const QString &glyph = encodedText.at(glyphIndex);
        if (glyph.size() < kBadgeHeight * 2) {
            continue;
        }

        for (int row = 0; row < kBadgeHeight; ++row) {
            bool ok = false;
            const int value = glyph.mid(row * 2, 2).toInt(&ok, 16);
            if (!ok) {
                continue;
            }

            for (int bit = 0; bit < 8; ++bit) {
                grid[row][glyphIndex * 8 + bit] = ((value >> (7 - bit)) & 0x01) != 0;
            }
        }
    }

    return grid;
}

void renderLeftAnimation(const Grid &source, int animationIndex, Grid *canvas)
{
    const int sourceWidth = source.at(0).size();

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            const int scrollOffset = animationIndex % (sourceWidth + kBadgeWidth);
            const int sourceColumn = column + scrollOffset - kBadgeWidth;
            if (sourceColumn >= 0 && sourceColumn < sourceWidth) {
                (*canvas)[row][column] = source[row][sourceColumn];
            }
        }
    }
}

void renderRightAnimation(const Grid &source, int animationIndex, Grid *canvas)
{
    const int sourceWidth = source.at(0).size();

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            const int scrollOffset = animationIndex % (sourceWidth + kBadgeWidth);
            const int sourceColumn = sourceWidth - scrollOffset + column;
            if (sourceColumn >= 0 && sourceColumn < sourceWidth) {
                (*canvas)[row][column] = source[row][sourceColumn];
            }
        }
    }
}

void renderUpAnimation(const Grid &source, int animationIndex, Grid *canvas)
{
    const int sourceWidth = source.at(0).size();
    const qreal animationStep = static_cast<qreal>(sourceWidth) / kBadgeHeight;
    const int animationValue = animationStep > 0.0 ? static_cast<int>(animationIndex / animationStep) : 0;

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            if (column < sourceWidth) {
                (*canvas)[row][column] = source[positiveModulo(row + animationValue, kBadgeHeight)][column];
            }
        }
    }
}

void renderDownAnimation(const Grid &source, int animationIndex, Grid *canvas)
{
    const int sourceWidth = source.at(0).size();
    const qreal animationStep = static_cast<qreal>(sourceWidth) / kBadgeHeight;
    const int animationValue = animationStep > 0.0 ? static_cast<int>(animationIndex / animationStep) : 0;

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            if (column < sourceWidth) {
                (*canvas)[row][column] = source[positiveModulo(row - animationValue, kBadgeHeight)][column];
            }
        }
    }
}

void renderFixedAnimation(const Grid &source, Grid *canvas)
{
    const int sourceWidth = source.at(0).size();
    const int horizontalOffset = (kBadgeWidth - sourceWidth) / 2;

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            const int sourceColumn = column - horizontalOffset;
            if (sourceColumn >= 0 && sourceColumn < sourceWidth) {
                (*canvas)[row][column] = source[row][sourceColumn];
            }
        }
    }
}

void renderSnowflakeAnimation(const Grid &source, int animationIndex, Grid *canvas)
{
    const int sourceWidth = source.at(0).size();
    const int framesCount = qMax(1, (sourceWidth + kBadgeWidth - 1) / kBadgeWidth);
    const int currentFrame = (animationIndex / kBadgeWidth) % framesCount;
    const int startColumn = currentFrame * kBadgeWidth;

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            const int sourceColumn = startColumn + column;
            if (sourceColumn < sourceWidth) {
                (*canvas)[row][column] = source[row][sourceColumn];
            }
        }
    }
}

void renderPictureAnimation(const Grid &source, int animationIndex, Grid *canvas)
{
    const int sourceWidth = source.at(0).size();
    const int totalAnimationLength = kBadgeHeight * 16;
    const int frame = animationIndex % totalAnimationLength;
    const int horizontalOffset = (kBadgeWidth - sourceWidth) / 2;

    const bool phaseOne = frame < kBadgeHeight * 4;
    const bool phaseTwo = frame >= kBadgeHeight * 4 && frame < kBadgeHeight * 8;

    if (phaseOne) {
        for (int row = kBadgeHeight - 1; row >= 0; --row) {
            int fallPosition = frame - ((kBadgeHeight - 1 - row) * 2);
            const int stoppingPosition = row;
            fallPosition = fallPosition >= stoppingPosition ? stoppingPosition : fallPosition;

            if (fallPosition >= 0 && fallPosition < kBadgeHeight) {
                for (int column = 0; column < kBadgeWidth; ++column) {
                    const int sourceColumn = column - horizontalOffset;
                    if (sourceColumn >= 0 && sourceColumn < sourceWidth) {
                        (*canvas)[fallPosition][column] = source[row][sourceColumn];
                    }
                }
            }
        }
    } else if (phaseTwo) {
        for (int row = kBadgeHeight - 1; row >= 0; --row) {
            const int fallOutStartFrame = (kBadgeHeight - 1 - row) * 2;
            const int fallOutPosition = row + (frame - (kBadgeHeight * 4) - fallOutStartFrame);

            if (fallOutPosition < row) {
                for (int column = 0; column < kBadgeWidth; ++column) {
                    const int sourceColumn = column - horizontalOffset;
                    if (sourceColumn >= 0 && sourceColumn < sourceWidth) {
                        (*canvas)[row][column] = source[row][sourceColumn];
                    }
                }
            }

            if (fallOutPosition >= row && fallOutPosition < kBadgeHeight) {
                for (int column = 0; column < kBadgeWidth; ++column) {
                    const int sourceColumn = column - horizontalOffset;
                    if (sourceColumn >= 0 && sourceColumn < sourceWidth) {
                        (*canvas)[fallOutPosition][column] = source[row][sourceColumn];
                    }
                }
            }
        }
    }
}

void renderAnimationMode(const Grid &source, int animationIndex, Grid *canvas)
{
    const int sourceWidth = source.at(0).size();
    const int displayWidth = sourceWidth > kBadgeWidth ? kBadgeWidth : sourceWidth;
    const int horizontalOffset = (kBadgeWidth - displayWidth) / 2;
    const int totalAnimationLength = kBadgeWidth;
    const int frame = animationIndex % totalAnimationLength;
    const bool firstHalf = frame < (kBadgeWidth / 2);
    const bool secondHalf = frame >= (kBadgeWidth / 2);
    const int leftCenterColumn = (kBadgeWidth / 2) - 1;
    const int rightCenterColumn = kBadgeWidth / 2;
    const int maxDistance = leftCenterColumn;
    const int currentAnimationIndex = animationIndex % (maxDistance + 1);

    int leftColumnPosition = leftCenterColumn - currentAnimationIndex;
    int rightColumnPosition = rightCenterColumn + currentAnimationIndex;
    if (leftColumnPosition < 0) {
        leftColumnPosition += kBadgeWidth;
    }
    if (rightColumnPosition >= kBadgeWidth) {
        rightColumnPosition -= kBadgeWidth;
    }

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            const int sourceColumn = column - horizontalOffset;
            const bool isWithinSource = sourceColumn >= 0
                    && sourceColumn < displayWidth;
            const bool lineVisible = column == leftColumnPosition || column == rightColumnPosition;
            bool bitmapVisibleInside = false;
            bool bitmapVisibleOutside = false;

            if (firstHalf && isWithinSource && column > leftColumnPosition && column < rightColumnPosition) {
                bitmapVisibleInside = source[row][sourceColumn];
            }
            if (secondHalf && isWithinSource
                    && (column < leftColumnPosition || column > rightColumnPosition)) {
                bitmapVisibleOutside = source[row][sourceColumn];
            }

            (*canvas)[row][column] = lineVisible || bitmapVisibleInside || bitmapVisibleOutside;
        }
    }
}

void renderLaserAnimation(const Grid &source, int animationIndex, Grid *canvas)
{
    const int sourceWidth = source.at(0).size();
    const int framesCount = qMax(1, (sourceWidth + kBadgeWidth - 1) / kBadgeWidth);
    const int currentFrame = (animationIndex / kBadgeWidth) % framesCount;
    const int startColumn = currentFrame * kBadgeWidth;
    const int horizontalOffset = qBound(0, kBadgeWidth - sourceWidth, kBadgeWidth) / 2;
    const int totalAnimationLength = kBadgeWidth * 2;
    const int frame = animationIndex % totalAnimationLength;
    const bool firstHalf = frame < kBadgeWidth;
    const bool secondHalf = frame >= kBadgeWidth;
    const int laserColumn = frame % kBadgeWidth;

    if (firstHalf) {
        if (laserColumn < sourceWidth) {
            for (int row = 0; row < kBadgeHeight; ++row) {
                const int sourceColumn = startColumn + laserColumn;
                if (sourceColumn >= 0 && sourceColumn < sourceWidth && source[row][sourceColumn]) {
                    for (int column = laserColumn + horizontalOffset; column < kBadgeWidth; ++column) {
                        (*canvas)[row][column] = true;
                    }
                }
            }
        }

        for (int column = 0; column < laserColumn; ++column) {
            for (int row = 0; row < kBadgeHeight; ++row) {
                const int sourceColumn = startColumn + column;
                const int canvasColumn = column + horizontalOffset;
                if (sourceColumn >= 0 && sourceColumn < sourceWidth
                        && canvasColumn >= 0 && canvasColumn < kBadgeWidth) {
                    (*canvas)[row][canvasColumn] = source[row][sourceColumn];
                }
            }
        }
    }

    if (secondHalf) {
        if (laserColumn < sourceWidth) {
            for (int row = 0; row < kBadgeHeight; ++row) {
                for (int column = 0; column < kBadgeWidth; ++column) {
                    const int sourceColumn = startColumn + column - horizontalOffset;
                    if (sourceColumn >= 0 && sourceColumn < sourceWidth) {
                        (*canvas)[row][column] = source[row][sourceColumn];
                    }
                }
            }

            for (int row = 0; row < kBadgeHeight; ++row) {
                if (startColumn + laserColumn < sourceWidth && source[row][startColumn + laserColumn]) {
                    for (int column = 0; column < laserColumn + horizontalOffset; ++column) {
                        if (column >= 0 && column < kBadgeWidth) {
                            (*canvas)[row][column] = true;
                        }
                    }
                } else {
                    for (int column = 0; column < laserColumn + horizontalOffset; ++column) {
                        if (column >= 0 && column < kBadgeWidth) {
                            (*canvas)[row][column] = false;
                        }
                    }
                }
            }
        }
    }
}

void applyFlashEffect(Grid *canvas, int animationIndex)
{
    const bool flashOn = positiveModulo(animationIndex, 2) == 0;

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            (*canvas)[row][column] = (*canvas)[row][column] && flashOn;
        }
    }
}

void applyMarqueeEffect(Grid *canvas, int animationIndex)
{
    const int marqueeIndex = animationIndex / 2;

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            bool showMarquee = row == 0
                    || column == 0
                    || row == kBadgeHeight - 1
                    || column == kBadgeWidth - 1;

            if (showMarquee) {
                if ((row == 0 || column == kBadgeWidth - 1)
                        && !(row == kBadgeHeight - 1 && column == kBadgeWidth - 1)) {
                    showMarquee = positiveModulo(row + column, 4) == positiveModulo(marqueeIndex, 4);
                } else {
                    showMarquee = positiveModulo(row + column - 1, 4) == (3 - positiveModulo(marqueeIndex, 4));
                }
            }

            (*canvas)[row][column] = (*canvas)[row][column] || showMarquee;
        }
    }
}

} // namespace

BadgePreviewItem::BadgePreviewItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_sourceGrid(blankGrid())
    , m_frameGrid(blankGrid())
{
    setAntialiasing(true);
    m_effectClock.start();

    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        ++m_animationIndex;
        renderFrame();
    });

    updateAnimationInterval();
    renderFrame();
    m_timer.start();
}

QString BadgePreviewItem::text() const
{
    return m_text;
}

bool BadgePreviewItem::flash() const
{
    return m_flash;
}

bool BadgePreviewItem::marquee() const
{
    return m_marquee;
}

int BadgePreviewItem::colorIndex() const
{
    return m_colorIndex;
}

int BadgePreviewItem::speedIndex() const
{
    return m_speedIndex;
}

int BadgePreviewItem::modeIndex() const
{
    return m_modeIndex;
}

void BadgePreviewItem::setText(const QString &text)
{
    const QString trimmedText = text.trimmed();
    if (m_text == trimmedText) {
        return;
    }

    m_text = trimmedText;
    rebuildSourceGrid();
    m_animationIndex = 0;
    renderFrame();
    emit textChanged();
}

void BadgePreviewItem::setFlash(bool flash)
{
    if (m_flash == flash) {
        return;
    }

    m_flash = flash;
    renderFrame();
    emit flashChanged();
}

void BadgePreviewItem::setMarquee(bool marquee)
{
    if (m_marquee == marquee) {
        return;
    }

    m_marquee = marquee;
    renderFrame();
    emit marqueeChanged();
}

void BadgePreviewItem::setColorIndex(int colorIndex)
{
    const int safeIndex = safeColorIndex(colorIndex);
    if (m_colorIndex == safeIndex) {
        return;
    }

    m_colorIndex = safeIndex;
    update();
    emit colorIndexChanged();
}

void BadgePreviewItem::setSpeedIndex(int speedIndex)
{
    const int safeIndex = safeSpeedIndex(speedIndex);
    if (m_speedIndex == safeIndex) {
        return;
    }

    m_speedIndex = safeIndex;
    updateAnimationInterval();
    emit speedIndexChanged();
}

void BadgePreviewItem::setModeIndex(int modeIndex)
{
    const int safeIndex = safeModeIndex(modeIndex);
    if (m_modeIndex == safeIndex) {
        return;
    }

    m_modeIndex = safeIndex;
    m_animationIndex = 0;
    renderFrame();
    emit modeIndexChanged();
}

void BadgePreviewItem::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillRect(boundingRect(), Qt::transparent);

    const QRectF bounds = boundingRect();
    const qreal backgroundOffsetY = bounds.height() * 0.05;
    const qreal backgroundOffsetX = bounds.width() * 0.05;
    const QRectF badgeRect(backgroundOffsetX,
                           backgroundOffsetY,
                           bounds.width() - (backgroundOffsetX * 2.0),
                           bounds.height() - (backgroundOffsetY * 2.0));

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(QStringLiteral("#000000")));
    painter->drawRoundedRect(badgeRect, 10.0, 10.0);

    const qreal cellSize = badgeRect.width() / kBadgeWidth;
    const qreal totalCellsWidth = (cellSize * 0.92) * kBadgeWidth;
    const qreal totalCellsHeight = cellSize * kBadgeHeight;
    const qreal cellStartX = backgroundOffsetX + (badgeRect.width() - totalCellsWidth) / 2.0;
    const qreal cellStartY = backgroundOffsetY + (badgeRect.height() - totalCellsHeight) / 2.0;
    const QSizeF cellSizeRect(cellSize / 2.5, cellSize);
    const QColor offColor(QStringLiteral("#212121"));
    const QColor onColor = ledColorForIndex(m_colorIndex);

    for (int row = 0; row < kBadgeHeight; ++row) {
        for (int column = 0; column < kBadgeWidth; ++column) {
            const QRectF cellRect(cellStartX + (column * (cellSize * 0.93)),
                                  cellStartY + (row * cellSize),
                                  cellSizeRect.width(),
                                  cellSizeRect.height());

            painter->save();
            painter->translate(cellRect.center());
            painter->rotate(45.0);
            painter->translate(-cellRect.center());
            painter->setBrush(m_frameGrid[row][column] ? onColor : offColor);
            painter->drawRect(cellRect);
            painter->restore();
        }
    }
}

void BadgePreviewItem::rebuildSourceGrid()
{
    m_sourceGrid = encodedTextToGrid(BadgeEncoder::encodeText(m_text));
}

void BadgePreviewItem::renderFrame()
{
    m_frameGrid = blankGrid();
    const qint64 effectElapsedMs = m_effectClock.isValid() ? m_effectClock.elapsed() : 0;
    const int marqueeIndex = static_cast<int>(effectElapsedMs / kMarqueeStepMs);
    const int flashIndex = static_cast<int>(effectElapsedMs / kFlashToggleMs);

    switch (safeModeIndex(m_modeIndex)) {
    case 0:
        renderLeftAnimation(m_sourceGrid, m_animationIndex, &m_frameGrid);
        break;
    case 1:
        renderRightAnimation(m_sourceGrid, m_animationIndex, &m_frameGrid);
        break;
    case 2:
        renderUpAnimation(m_sourceGrid, m_animationIndex, &m_frameGrid);
        break;
    case 3:
        renderDownAnimation(m_sourceGrid, m_animationIndex, &m_frameGrid);
        break;
    case 4:
        renderFixedAnimation(m_sourceGrid, &m_frameGrid);
        break;
    case 5:
        renderSnowflakeAnimation(m_sourceGrid, m_animationIndex, &m_frameGrid);
        break;
    case 6:
        renderPictureAnimation(m_sourceGrid, m_animationIndex, &m_frameGrid);
        break;
    case 7:
        renderAnimationMode(m_sourceGrid, m_animationIndex, &m_frameGrid);
        break;
    case 8:
    default:
        renderLaserAnimation(m_sourceGrid, m_animationIndex, &m_frameGrid);
        break;
    }

    if (m_flash) {
        applyFlashEffect(&m_frameGrid, flashIndex);
    }
    if (m_marquee) {
        applyMarqueeEffect(&m_frameGrid, marqueeIndex * 2);
    }

    update();
}

void BadgePreviewItem::updateAnimationInterval()
{
    m_timer.setInterval(animationIntervalMs(m_speedIndex));
}

Grid BadgePreviewItem::blankGrid(int width)
{
    return blankPreviewGrid(width);
}
