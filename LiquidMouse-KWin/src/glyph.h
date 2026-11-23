#pragma once

#include <QPointF>
#include <vector>
#include "cursorcontext.h"

/// The body's shape for one mouse function, described by numbers that can each be animated on their own:
/// a core on the pointer (a dot, or the arrow), a ring and a few limbs.
/// Ported from Sources/LiquidMouse/Glyph.swift.
struct Glyph {
    struct Limb {
        QPointF start = QPointF(0.0, 0.0);
        QPointF end = QPointF(0.0, 0.0);
        double startRadius = 0.0;
        double endRadius = 0.0;
    };

    static constexpr int limbCount = 6;
    static constexpr double designRadius = 6.5;

    double core = 0.0;
    double tail = 0.0;
    QPointF ringCenter = QPointF(0.0, 0.0);
    double ringRadius = 0.0;
    double ringWidth = 0.0;
    std::vector<Limb> limbs = std::vector<Limb>(limbCount);
    bool isLively = false;
    bool isArrow = false;
    bool shrinksWhenAiming = false;
    double pressedScale = 0.85;

    Glyph();
    Glyph(const CursorContext &context, CursorForm form, double dotRadius);

    std::vector<double> values() const;
    void setValues(const std::vector<double> &v);

private:
    void dot(const CursorContext &context, double r);
    void arrow(const CursorContext &context, double r);

    void bar(const QPointF &axis, double halfLength, double halfWidth);
    void plus(const QPointF &center, double arm, double radius);
    void alias(const QPointF &center, double size, double radius);
    void menu(const QPointF &center, double halfWidth, double spacing, double radius);
    void lens(bool magnifies, double r, double thin);
    void line(int index, const QPointF &start, const QPointF &end, double radius);
};
