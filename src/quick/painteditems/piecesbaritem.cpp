/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "piecesbaritem.h"

#include <algorithm>
#include <cmath>

#include <QPainter>

#include "base/logging.h"

PiecesBarItem::PiecesBarItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    // Smooth-ish upscaling of a 1px-tall image reads best with FastTransformation
    // for a crisp per-pixel bar; the item is redrawn on every data tick.
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    rebuildGradient();
    qCDebug(lcUi) << "PiecesBarItem constructed";
}

void PiecesBarItem::setMode(const Mode mode)
{
    if (m_mode == mode)
        return;

    m_mode = mode;
    qCDebug(lcUi) << "PiecesBarItem mode ->" << mode;
    emit modeChanged();
    rebuildImage();
}

void PiecesBarItem::setPieces(const QBitArray &pieces)
{
    if (m_pieces == pieces)
        return;

    m_pieces = pieces;
    emit piecesChanged();
    rebuildImage();
}

void PiecesBarItem::setDownloadingPieces(const QBitArray &downloadingPieces)
{
    if (m_downloadingPieces == downloadingPieces)
        return;

    m_downloadingPieces = downloadingPieces;
    emit piecesChanged();
    rebuildImage();
}

void PiecesBarItem::setAvailability(const QList<int> &availability)
{
    if (m_availability == availability)
        return;

    m_availability = availability;
    emit availabilityChanged();
    rebuildImage();
}

void PiecesBarItem::setPieceColor(const QColor &color)
{
    if (m_pieceColor == color)
        return;

    m_pieceColor = color;
    emit colorsChanged();
    rebuildGradient();
    rebuildImage();
}

void PiecesBarItem::setPartialColor(const QColor &color)
{
    if (m_partialColor == color)
        return;

    m_partialColor = color;
    emit colorsChanged();
    rebuildImage();
}

void PiecesBarItem::setMissingColor(const QColor &color)
{
    if (m_missingColor == color)
        return;

    m_missingColor = color;
    emit colorsChanged();
    rebuildGradient();
    rebuildImage();
}

void PiecesBarItem::setBorderColor(const QColor &color)
{
    if (m_borderColor == color)
        return;

    m_borderColor = color;
    emit colorsChanged();
    update();
}

void PiecesBarItem::clear()
{
    qCDebug(lcUi) << "PiecesBarItem cleared";
    m_pieces.clear();
    m_downloadingPieces.clear();
    m_availability.clear();
    m_image = QImage();
    update();
}

void PiecesBarItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (static_cast<int>(newGeometry.width()) != static_cast<int>(oldGeometry.width()))
        rebuildImage();
}

int PiecesBarItem::imageWidth() const
{
    return std::max(1, static_cast<int>(width()) - (2 * BORDER_WIDTH));
}

QRgb PiecesBarItem::mixTwoColors(const QRgb rgb1, const QRgb rgb2, const float ratio)
{
    const float ratioN = 1.0f - ratio;
    const int r = static_cast<int>((qRed(rgb1) * ratioN) + (qRed(rgb2) * ratio));
    const int g = static_cast<int>((qGreen(rgb1) * ratioN) + (qGreen(rgb2) * ratio));
    const int b = static_cast<int>((qBlue(rgb1) * ratioN) + (qBlue(rgb2) * ratio));
    return qRgb(r, g, b);
}

void PiecesBarItem::rebuildGradient()
{
    m_gradient = QList<QRgb>(256);
    const QRgb from = m_missingColor.rgb();
    const QRgb to = m_pieceColor.rgb();
    for (int i = 0; i < 256; ++i)
        m_gradient[i] = mixTwoColors(from, to, (i / 255.0f));
}

QList<float> PiecesBarItem::bitfieldToFloatVector(const QBitArray &bits, const int reqSize)
{
    QList<float> result(reqSize, 0.0f);
    if (bits.isEmpty())
        return result;

    const float ratio = bits.size() / static_cast<float>(reqSize);

    for (int x = 0; x < reqSize; ++x)
    {
        const float fromR = x * ratio;
        const float toR = (x + 1) * ratio;

        int fromC = static_cast<int>(fromR); // floor
        int toC = static_cast<int>(std::ceil(toR));
        if (toC > bits.size())
            --toC;

        int x2 = fromC;
        const int toCMinusOne = toC - 1;
        float value = 0.0f;

        if (x2 == toCMinusOne)
        {
            if (bits[x2])
                value += ratio;
            ++x2;
        }
        else
        {
            if (x2 != fromR)
            {
                if (bits[x2])
                    value += 1.0f - (fromR - fromC);
                ++x2;
            }

            for (; x2 < toCMinusOne; ++x2)
            {
                if (bits[x2])
                    value += 1.0f;
            }

            if (x2 == toCMinusOne)
            {
                if (bits[x2])
                    value += 1.0f - (toC - toR);
                ++x2;
            }
        }

        value /= ratio;
        value = std::min(value, 1.0f);
        result[x] = value;
    }

    return result;
}

QList<float> PiecesBarItem::availabilityToFloatVector(const QList<int> &avail, const int reqSize)
{
    QList<float> result(reqSize, 0.0f);
    if (avail.isEmpty())
        return result;

    const float ratio = static_cast<float>(avail.size()) / reqSize;
    const int maxElement = *std::ranges::max_element(avail);
    if (maxElement == 0)
        return result;

    for (int x = 0; x < reqSize; ++x)
    {
        const float fromR = x * ratio;
        const float toR = (x + 1) * ratio;

        int fromC = static_cast<int>(fromR);
        int toC = static_cast<int>(std::ceil(toR));
        if (toC > avail.size())
            --toC;

        int x2 = fromC;
        const int toCMinusOne = toC - 1;
        float value = 0.0f;

        if (x2 == toCMinusOne)
        {
            if (avail[x2])
                value += ratio * avail[x2];
            ++x2;
        }
        else
        {
            if (x2 != fromR)
            {
                if (avail[x2])
                    value += (1.0f - (fromR - fromC)) * avail[x2];
                ++x2;
            }

            for (; x2 < toCMinusOne; ++x2)
            {
                if (avail[x2])
                    value += avail[x2];
            }

            if (x2 == toCMinusOne)
            {
                if (avail[x2])
                    value += (1.0f - (toC - toR)) * avail[x2];
                ++x2;
            }
        }

        value /= (ratio * maxElement);
        value = std::min(value, 1.0f);
        result[x] = value;
    }

    return result;
}

void PiecesBarItem::rebuildImage()
{
    const int w = imageWidth();
    QImage image(w, 1, QImage::Format_RGB888);
    if (image.isNull())
    {
        qCWarning(lcUi) << "PiecesBarItem: QImage allocation failed, width" << w;
        m_image = QImage();
        update();
        return;
    }

    if (m_mode == Mode::Availability)
    {
        if (m_availability.isEmpty())
        {
            image.fill(m_missingColor);
        }
        else
        {
            const QList<float> scaled = availabilityToFloatVector(m_availability, w);
            for (int x = 0; x < w; ++x)
                image.setPixel(x, 0, m_gradient.at(static_cast<int>(scaled.at(x) * 255)));
        }
    }
    else // Downloaded
    {
        if (m_pieces.isEmpty())
        {
            image.fill(m_missingColor);
        }
        else
        {
            const QList<float> scaledPieces = bitfieldToFloatVector(m_pieces, w);
            const QList<float> scaledDl = bitfieldToFloatVector(m_downloadingPieces, w);
            const QRgb pieceRgb = m_pieceColor.rgb();
            const QRgb partialRgb = m_partialColor.rgb();
            const QRgb missingRgb = m_missingColor.rgb();

            for (int x = 0; x < w; ++x)
            {
                const float have = scaledPieces.at(x);
                const float downloading = scaledDl.at(x);
                if (downloading != 0.0f)
                {
                    const float fillRatio = have + downloading;
                    const float ratio = downloading / fillRatio;
                    QRgb mixed = mixTwoColors(pieceRgb, partialRgb, ratio);
                    mixed = mixTwoColors(missingRgb, mixed, std::min(fillRatio, 1.0f));
                    image.setPixel(x, 0, mixed);
                }
                else
                {
                    image.setPixel(x, 0, m_gradient.at(static_cast<int>(have * 255)));
                }
            }
        }
    }

    m_image = image;
    update();
}

void PiecesBarItem::paint(QPainter *painter)
{
    const QRectF outer(0, 0, width(), height());
    const QRectF inner = outer.adjusted(BORDER_WIDTH, BORDER_WIDTH, -BORDER_WIDTH, -BORDER_WIDTH);

    if (m_image.isNull())
    {
        painter->fillRect(inner, m_missingColor);
    }
    else
    {
        // The image is 1px tall and imageWidth() wide; stretch to the inner rect.
        if (m_image.width() != static_cast<int>(inner.width()))
            rebuildImage();
        painter->drawImage(inner, m_image);
    }

    QPen pen(m_borderColor);
    pen.setWidth(BORDER_WIDTH);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(outer.adjusted(0, 0, -1, -1));
}
