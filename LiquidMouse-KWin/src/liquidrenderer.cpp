#include "liquidrenderer.h"
#include <cmath>
#include <algorithm>

LiquidRenderer::Colors LiquidRenderer::resolveColors(BodyLook look, bool systemIsDark) {
    bool darkCursor = false;
    if (look == BodyLook::Dark) {
        darkCursor = true; // Cursore scuro (riempimento nero, bordo bianco)
    } else if (look == BodyLook::Light) {
        darkCursor = false; // Cursore chiaro (riempimento bianco, bordo nero)
    } else {
        // Automatico: di default il cursore chiaro con bordo nero è visibile sia su sfondi scuri che chiari
        // Ma se il desktop è in tema chiaro, possiamo usare cursore scuro con bordo bianco
        darkCursor = !systemIsDark;
    }

    if (darkCursor) {
        return {
            QColor(30, 30, 30, 255),        // Riempimento scuro solido
            QColor(255, 255, 255, 255),    // Bordo bianco candido solido
            QColor(0, 0, 0, 110)            // Ombra sottostante
        };
    } else {
        return {
            QColor(255, 255, 255, 255),    // Riempimento bianco candido solido
            QColor(15, 15, 15, 255),        // Bordo nero intenso solido (contrasto 100% su sfondo bianco!)
            QColor(0, 0, 0, 95)             // Ombra sottostante
        };
    }
}

static void addBridge(const LiquidBody::Blob &a, const LiquidBody::Blob &b, double margin, QPainterPath &path) {
    double dx = b.center.x() - a.center.x();
    double dy = b.center.y() - a.center.y();
    double length = std::hypot(dx, dy);
    if (length <= 0.05) return;
    double nx = -dy / length;
    double ny = dx / length;
    double ra = a.radius + margin;
    double rb = b.radius + margin;

    QPolygonF quad;
    quad.reserve(4);
    quad.append(QPointF(a.center.x() - nx * ra, a.center.y() - ny * ra));
    quad.append(QPointF(b.center.x() - nx * rb, b.center.y() - ny * rb));
    quad.append(QPointF(b.center.x() + nx * rb, b.center.y() + ny * rb));
    quad.append(QPointF(a.center.x() + nx * ra, a.center.y() + ny * ra));
    path.addPolygon(quad);
    path.closeSubpath();
}

QPainterPath LiquidRenderer::shapeToPath(const LiquidBody::Shape &shape, double margin, bool includeSplash) {
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);

    if (shape.outline.size() > 2) {
        int count = static_cast<int>(shape.outline.size());
        QPolygonF poly;
        poly.reserve(count);
        for (int i = 0; i < count; ++i) {
            poly.append(shape.outline[i]);
        }
        path.addPolygon(poly);
        path.closeSubpath();
    }

    for (const auto &outl : shape.outlines) {
        if (outl.size() > 2) {
            int count = static_cast<int>(outl.size());
            QPolygonF poly;
            poly.reserve(count);
            for (int i = 0; i < count; ++i) {
                poly.append(outl[i]);
            }
            path.addPolygon(poly);
            path.closeSubpath();
        }
    }

    for (const auto &stroke : shape.strokes) {
        for (size_t i = 0; i < stroke.size(); ++i) {
            const auto &blob = stroke[i];
            double r = blob.radius + margin;
            path.addEllipse(blob.center, r, r);
            if (i > 0) {
                addBridge(stroke[i - 1], blob, margin, path);
            }
        }
    }

    for (const auto &ring : shape.rings) {
        double rOuter = ring.radius + ring.width * 0.5 + margin;
        path.addEllipse(ring.center, rOuter, rOuter);
        double hole = ring.radius - ring.width * 0.5 - margin;
        if (hole > 0.0) {
            QPainterPath holePath;
            holePath.addEllipse(ring.center, hole, hole);
            path = path.subtracted(holePath);
        }
    }

    if (includeSplash) {
        for (const auto &stroke : shape.splash) {
            for (const auto &blob : stroke) {
                double r = blob.radius + margin;
                path.addEllipse(blob.center, r, r);
            }
        }
    }

    return path;
}

QPainterPath LiquidRenderer::splashToPath(const std::vector<std::vector<LiquidBody::Blob>> &splash, double margin) {
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    for (const auto &stroke : splash) {
        for (const auto &blob : stroke) {
            double r = blob.radius + margin;
            path.addEllipse(blob.center, r, r);
        }
    }
    return path;
}

void LiquidRenderer::render(QPainter &painter, const LiquidBody::Shape &shape, const Colors &colors) {
    if (shape.isEmpty()) return;

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    QPainterPath fill = shapeToPath(shape, 0.0, false);
    QPainterPath splashFill = splashToPath(shape.splash, 0.0);

    // Unifica tutti i contorni sovrapposti per eliminare giunzioni interne
    QPainterPath unified = fill.simplified();

    // 1. Ombra proiettata morbida
    painter.save();
    painter.translate(0.0, 1.5);
    painter.fillPath(unified, colors.shadow);
    painter.restore();

    // 2. Riempimento solido del corpo del cursore
    painter.fillPath(unified, colors.fill);

    // 3. Bordicino perimetrale di contrasto netto (nero su bianco o bianco su nero)
    // Disegnato rigorosamente DOPO il riempimento con QPen: in questo modo il riempimento
    // non può mai coprire né mascherare il bordo, neanche quando il cursore si rimpicciolisce!
    QPen rimPen(colors.rim, rimWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.strokePath(unified, rimPen);

    // 4. Schizzi / gocce splash con ombra, riempimento e bordicino
    if (!splashFill.isEmpty()) {
        QPainterPath unifiedSplash = splashFill.simplified();
        painter.save();
        painter.translate(0.0, 1.5);
        painter.fillPath(unifiedSplash, colors.shadow);
        painter.restore();

        painter.fillPath(unifiedSplash, colors.fill);
        painter.strokePath(unifiedSplash, rimPen);
    }
}

QImage LiquidRenderer::renderToImage(const LiquidBody::Shape &shape, const Colors &colors, QPointF &outTopLeft) {
    if (shape.isEmpty()) {
        outTopLeft = QPointF(0.0, 0.0);
        return QImage();
    }

    QPainterPath full = shapeToPath(shape, 0.0, true);
    QPainterPathStroker stroker;
    stroker.setWidth((rimWidth + shadowRadius + 4.0) * 2.0);
    stroker.setJoinStyle(Qt::RoundJoin);
    stroker.setCapStyle(Qt::RoundCap);
    QPainterPath stroke = stroker.createStroke(full);
    full.addPath(stroke);

    QRectF bounds = full.boundingRect();
    if (bounds.isEmpty() || bounds.width() <= 0.0 || bounds.height() <= 0.0) {
        outTopLeft = QPointF(0.0, 0.0);
        return QImage();
    }

    // Espandi ai pixel interi con margine di sicurezza
    int x0 = static_cast<int>(std::floor(bounds.left())) - 2;
    int y0 = static_cast<int>(std::floor(bounds.top())) - 2;
    int x1 = static_cast<int>(std::ceil(bounds.right())) + 2;
    int y1 = static_cast<int>(std::ceil(bounds.bottom())) + 2;
    int w = std::max(1, x1 - x0);
    int h = std::max(1, y1 - y0);

    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.translate(-x0, -y0);
    render(p, shape, colors);
    p.end();

    outTopLeft = QPointF(x0, y0);
    return img;
}
