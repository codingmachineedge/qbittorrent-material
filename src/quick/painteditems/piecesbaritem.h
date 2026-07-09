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

#pragma once

#include <QBitArray>
#include <QColor>
#include <QImage>
#include <QList>
#include <QQmlEngine>
#include <QQuickPaintedItem>

class QPainter;

/**
 * @file piecesbaritem.h
 * @brief The @c PiecesBarItem QML painted item — renders the per-piece state of a
 *        torrent (downloaded/partial/missing) or per-piece availability.
 *
 * This is the Material replacement for the legacy @c DownloadedPiecesBar /
 * @c PieceAvailabilityBar widgets. A single item covers both use-cases via the
 * @ref Mode property:
 *   - @c Downloaded  paints completed pieces (@ref pieceColor), pieces that are
 *     currently being downloaded (@ref partialColor overlay), and missing pieces
 *     (@ref missingColor); driven by @ref pieces + @ref downloadingPieces.
 *   - @c Availability paints a 0..N availability heat scale between
 *     @ref missingColor and @ref pieceColor; driven by @ref availability.
 *
 * Colors are @c Q_PROPERTY inputs so the QML side binds them straight to the
 * @c Theme singleton (PiecesBar palette per DESIGN_SYSTEM: Piece→primary,
 * PartialPiece→primaryContainer, MissingPiece→surfaceVariant, Border→outlineVariant).
 * The item never hard-codes a palette.
 */
class PiecesBarItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
    Q_PROPERTY(QBitArray pieces READ pieces WRITE setPieces NOTIFY piecesChanged FINAL)
    Q_PROPERTY(QBitArray downloadingPieces READ downloadingPieces WRITE setDownloadingPieces NOTIFY piecesChanged FINAL)
    Q_PROPERTY(QList<int> availability READ availability WRITE setAvailability NOTIFY availabilityChanged FINAL)
    Q_PROPERTY(QColor pieceColor READ pieceColor WRITE setPieceColor NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor partialColor READ partialColor WRITE setPartialColor NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor missingColor READ missingColor WRITE setMissingColor NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY colorsChanged FINAL)

public:
    /// What the bar visualizes.
    enum Mode
    {
        Downloaded = 0,   ///< completed / partial (downloading) / missing pieces
        Availability = 1  ///< per-piece availability heat scale
    };
    Q_ENUM(Mode)

    explicit PiecesBarItem(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    [[nodiscard]] Mode mode() const { return m_mode; }
    void setMode(Mode mode);

    [[nodiscard]] const QBitArray &pieces() const { return m_pieces; }
    void setPieces(const QBitArray &pieces);

    [[nodiscard]] const QBitArray &downloadingPieces() const { return m_downloadingPieces; }
    void setDownloadingPieces(const QBitArray &downloadingPieces);

    [[nodiscard]] const QList<int> &availability() const { return m_availability; }
    void setAvailability(const QList<int> &availability);

    [[nodiscard]] QColor pieceColor() const { return m_pieceColor; }
    void setPieceColor(const QColor &color);

    [[nodiscard]] QColor partialColor() const { return m_partialColor; }
    void setPartialColor(const QColor &color);

    [[nodiscard]] QColor missingColor() const { return m_missingColor; }
    void setMissingColor(const QColor &color);

    [[nodiscard]] QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor &color);

    /// Drop all data and repaint an empty (missing) bar.
    Q_INVOKABLE void clear();

signals:
    void modeChanged();
    void piecesChanged();
    void availabilityChanged();
    void colorsChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    static constexpr int BORDER_WIDTH = 1;

    /// Blend two colors linearly, @p ratio in [0, 1] (0 -> @p rgb1, 1 -> @p rgb2).
    static QRgb mixTwoColors(QRgb rgb1, QRgb rgb2, float ratio);

    /// Down/up-scale a bitfield to @p reqSize buckets, each in [0, 1] = fill fraction.
    static QList<float> bitfieldToFloatVector(const QBitArray &bits, int reqSize);
    /// Down/up-scale an availability vector to @p reqSize buckets, each in [0, 1].
    static QList<float> availabilityToFloatVector(const QList<int> &avail, int reqSize);

    /// Rebuild the 256-level missing->piece gradient lookup table.
    void rebuildGradient();
    /// Recompute @c m_image from the current data + colors, then schedule a repaint.
    void rebuildImage();

    [[nodiscard]] int imageWidth() const;

    Mode m_mode = Mode::Downloaded;

    QBitArray m_pieces;
    QBitArray m_downloadingPieces;
    QList<int> m_availability;

    QColor m_pieceColor {0x09, 0x69, 0xda};    // primary (light) fallback
    QColor m_partialColor {0xa3, 0x71, 0xf7};  // done/partial fallback
    QColor m_missingColor {0xf6, 0xf8, 0xfa};  // surfaceVariant fallback
    QColor m_borderColor {0xd0, 0xd7, 0xde};   // outlineVariant fallback

    // Buffered 256-level gradient from missing color to piece color.
    QList<QRgb> m_gradient;
    QImage m_image;
};
