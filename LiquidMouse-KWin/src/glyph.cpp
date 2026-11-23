#include "glyph.h"
#include <cmath>
#include <algorithm>

Glyph::Glyph() = default;

Glyph::Glyph(const CursorContext &context, CursorForm form, double dotRadius) {
    switch (form) {
    case CursorForm::Dot:
        dot(context, dotRadius);
        break;
    case CursorForm::Arrow:
        arrow(context, dotRadius);
        break;
    }
}

void Glyph::dot(const CursorContext &context, double r) {
    double u = r / designRadius;
    double thin = std::max(0.9, 0.15 * r);

    switch (context.kind) {
    case CursorContextKind::Pointer:
    case CursorContextKind::Other:
    case CursorContextKind::Hidden:
        core = r;
        tail = r;
        isLively = true;
        shrinksWhenAiming = true;
        pressedScale = 0.62;
        break;
    case CursorContextKind::Link:
        core = std::max(0.50 * r, 3.5);
        tail = core;
        isLively = true;
        pressedScale = 0.8;
        break;
    case CursorContextKind::Text:
        bar(QPointF(0.0, 1.0), context.halfHeight * u, 1.75 * u);
        break;
    case CursorContextKind::VerticalText:
        bar(QPointF(1.0, 0.0), 9.0 * u, 1.75 * u);
        break;
    case CursorContextKind::Grab:
        core = 0.42 * r;
        ringRadius = 1.3 * r;
        ringWidth = 0.3 * r;
        break;
    case CursorContextKind::Grabbing:
        core = 0.85 * r;
        ringRadius = 0.45 * r;
        ringWidth = 0.3 * r;
        pressedScale = 1.0;
        break;
    case CursorContextKind::Resize: {
        QPointF way = context.direction;
        double tip = std::max(0.65, 0.1 * r);
        core = 0.6 * r;
        limbs[0] = Limb{QPointF(0.0, 0.0), way * 2.7 * r, 0.55 * r, tip};
        if (context.bothWays) {
            limbs[1] = Limb{QPointF(0.0, 0.0), -way * 2.7 * r, 0.55 * r, tip};
        }
        if (context.divider) {
            QPointF across(-way.y(), way.x());
            across *= 1.9 * r;
            line(2, QPointF(0.0, 0.0), across, thin);
            line(3, QPointF(0.0, 0.0), -across, thin);
        }
        break;
    }
    case CursorContextKind::Crosshair: {
        core = thin;
        const QPointF ways[] = {QPointF(1.0, 0.0), QPointF(-1.0, 0.0), QPointF(0.0, 1.0), QPointF(0.0, -1.0)};
        for (int i = 0; i < 4; ++i) {
            line(i, ways[i] * 0.8 * r, ways[i] * 2.2 * r, thin);
        }
        break;
    }
    case CursorContextKind::NotAllowed:
        ringRadius = 1.35 * r;
        ringWidth = 0.34 * r;
        line(0, QPointF(-0.9, -0.9) * r, QPointF(0.9, 0.9) * r, std::max(thin, 0.17 * r));
        break;
    case CursorContextKind::Copy:
        core = r;
        plus(QPointF(1.9, 1.9) * r, 0.62 * r, thin);
        break;
    case CursorContextKind::Alias:
        core = r;
        alias(QPointF(1.8, 1.8) * r, 0.6 * r, thin);
        break;
    case CursorContextKind::Menu:
        core = r;
        menu(QPointF(1.875, 1.8) * r, 0.575 * r, 0.55 * r, thin);
        break;
    case CursorContextKind::Zoom:
        lens(context.magnifies, r, thin);
        break;
    }
}

void Glyph::arrow(const CursorContext &context, double r) {
    double u = r / designRadius;
    double thin = std::max(0.9, u);
    QPointF badge = QPointF(15.5, 18.0) * u;
    isArrow = true;

    switch (context.kind) {
    case CursorContextKind::Pointer:
    case CursorContextKind::Other:
    case CursorContextKind::Hidden:
        core = r;
        tail = r;
        isLively = true;
        shrinksWhenAiming = true;
        pressedScale = 0.62;
        break;
    case CursorContextKind::Link:
        core = std::max(0.65 * r, 4.0);
        tail = core;
        isLively = true;
        pressedScale = 0.8;
        break;
    case CursorContextKind::Text:
    case CursorContextKind::VerticalText:
    case CursorContextKind::Grab:
    case CursorContextKind::Grabbing:
    case CursorContextKind::Resize:
    case CursorContextKind::Crosshair:
        dot(context, r);
        isArrow = false;
        break;
    case CursorContextKind::NotAllowed:
        core = r;
        ringCenter = badge;
        ringRadius = 4.2 * u;
        ringWidth = 1.6 * u;
        line(0, badge + QPointF(-2.9, -2.9) * u, badge + QPointF(2.9, 2.9) * u, 0.8 * thin);
        break;
    case CursorContextKind::Copy:
        core = r;
        plus(badge, 3.0 * u, thin);
        break;
    case CursorContextKind::Alias:
        core = r;
        alias(badge, 3.0 * u, thin);
        break;
    case CursorContextKind::Menu:
        core = r;
        menu(badge, 3.2 * u, 2.8 * u, thin);
        break;
    case CursorContextKind::Zoom:
        lens(context.magnifies, r, thin);
        break;
    }
}

std::vector<double> Glyph::values() const {
    std::vector<double> v;
    v.reserve(6 + limbCount * 6);
    v.push_back(core);
    v.push_back(tail);
    v.push_back(ringCenter.x());
    v.push_back(ringCenter.y());
    v.push_back(ringRadius);
    v.push_back(ringWidth);
    for (const auto &limb : limbs) {
        v.push_back(limb.start.x());
        v.push_back(limb.start.y());
        v.push_back(limb.end.x());
        v.push_back(limb.end.y());
        v.push_back(limb.startRadius);
        v.push_back(limb.endRadius);
    }
    return v;
}

void Glyph::setValues(const std::vector<double> &v) {
    if (v.size() < 6 + limbCount * 6) return;
    core = v[0];
    tail = v[1];
    ringCenter = QPointF(v[2], v[3]);
    ringRadius = v[4];
    ringWidth = v[5];
    for (int i = 0; i < limbCount; ++i) {
        int o = 6 + i * 6;
        limbs[i].start = QPointF(v[o], v[o + 1]);
        limbs[i].end = QPointF(v[o + 2], v[o + 3]);
        limbs[i].startRadius = v[o + 4];
        limbs[i].endRadius = v[o + 5];
    }
}

void Glyph::bar(const QPointF &axis, double halfLength, double halfWidth) {
    QPointF reach = axis * std::max(halfLength - halfWidth, 0.0);
    core = halfWidth;
    line(0, QPointF(0.0, 0.0), reach, halfWidth);
    line(1, QPointF(0.0, 0.0), -reach, halfWidth);
}

void Glyph::plus(const QPointF &center, double arm, double radius) {
    line(0, center - QPointF(arm, 0.0), center + QPointF(arm, 0.0), radius);
    line(1, center - QPointF(0.0, arm), center + QPointF(0.0, arm), radius);
}

void Glyph::alias(const QPointF &center, double size, double radius) {
    QPointF tip = center + QPointF(size, -size);
    line(0, center - QPointF(size, -size), tip, radius);
    line(1, tip, tip - QPointF(1.4 * size, 0.0), radius);
    line(2, tip, tip + QPointF(0.0, 1.4 * size), radius);
}

void Glyph::menu(const QPointF &center, double halfWidth, double spacing, double radius) {
    for (int i = 0; i < 3; ++i) {
        double y = center.y() + spacing * static_cast<double>(i - 1);
        line(i, QPointF(center.x() - halfWidth, y), QPointF(center.x() + halfWidth, y), radius);
    }
}

void Glyph::lens(bool magnifies, double r, double thin) {
    double invSqrt2 = 1.0 / std::sqrt(2.0);
    QPointF handle = QPointF(1.0, 1.0) * invSqrt2;
    ringRadius = 1.4 * r;
    ringWidth = 0.3 * r;
    line(0, QPointF(-0.65 * r, 0.0), QPointF(0.65 * r, 0.0), thin);
    if (magnifies) {
        line(1, QPointF(0.0, -0.65 * r), QPointF(0.0, 0.65 * r), thin);
    }
    line(2, handle * 1.55 * r, handle * 2.7 * r, std::max(thin, 0.24 * r));
}

void Glyph::line(int index, const QPointF &start, const QPointF &end, double radius) {
    if (index >= 0 && index < limbCount) {
        limbs[index] = Limb{start, end, radius, radius};
    }
}
