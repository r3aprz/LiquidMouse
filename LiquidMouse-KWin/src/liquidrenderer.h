#pragma once

#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QImage>
#include "liquidbody.h"
#include "cursorcontext.h"

class LiquidRenderer {
public:
    static constexpr double rimWidth = 1.8;
    static constexpr double shadowRadius = 2.5;

    struct Colors {
        QColor fill;
        QColor rim;
        QColor shadow;
    };

    static Colors resolveColors(BodyLook look, bool systemIsDark);
    static QPainterPath shapeToPath(const LiquidBody::Shape &shape, double margin, bool includeSplash = false);
    static QPainterPath splashToPath(const std::vector<std::vector<LiquidBody::Blob>> &splash, double margin);

    /// Paints the shape onto a given QPainter at (0, 0) relative origin.
    static void render(QPainter &painter, const LiquidBody::Shape &shape, const Colors &colors);

    /// Renders the shape onto a transparent QImage, returning the image and the top-left offset
    /// relative to the pointer position.
    static QImage renderToImage(const LiquidBody::Shape &shape, const Colors &colors, QPointF &outTopLeft);
};
