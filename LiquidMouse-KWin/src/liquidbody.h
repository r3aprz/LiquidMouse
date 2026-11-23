#pragma once

#include <QPointF>
#include <vector>
#include <deque>
#include "spring.h"
#include "glyph.h"
#include "cursorcontext.h"

/// The cursor as a drop of liquid: a dot at rest that stretches into a long tail while it moves,
/// squeezes while the mouse is pressed and drains away when left alone.
/// Ported from Sources/LiquidMouse/LiquidBody.swift.
struct LiquidBody {
    struct Tuning {
        BodyLook appearance = BodyLook::Automatic;
        double dotRadius = Glyph::designRadius;
        double trailLength = 160.0;
        double trailResponse = 80.0;
        double bounce = 0.5;
        bool shrinksWhenAiming = false;
        bool splashes = true;
        CursorForm form = CursorForm::Arrow;

        double reach() const { return trailLength + 3.0 * dotRadius; }

        double damping(double stiff) const {
            double loose = std::max(0.15, stiff - 0.45 * bounce);
            if (bounce >= 0.5) return loose;
            double middle = stiff - 0.45 * 0.5;
            return middle + (1.0 - middle) * (1.0 - bounce / 0.5);
        }
    };

    struct Blob {
        QPointF center;
        double radius = 0.0;
    };

    struct Ring {
        QPointF center;
        double radius = 0.0;
        double width = 0.0;
    };

    struct Shape {
        std::vector<std::vector<Blob>> strokes;
        std::vector<Ring> rings;
        std::vector<std::vector<Blob>> splash;
        std::vector<QPointF> outline;
        std::vector<std::vector<QPointF>> outlines;

        bool isEmpty() const {
            return strokes.empty() && rings.empty() && splash.empty() && outline.empty() && outlines.empty();
        }
    };

    struct Droplet {
        QPointF position;
        QPointF velocity;
        double age = 0.0;
    };

    static constexpr int nodeCount = 10;
    static constexpr int dropletCount = 7;
    static constexpr double dropletLife = 0.32;
    static constexpr double dropletDrag = 7.0;
    static constexpr double aimingSpeed = 200.0;
    static constexpr double travellingSpeed = 450.0;
    static constexpr double aimingShrink = 0.55;
    static constexpr double speedSmoothing = 0.06;
    static constexpr double thinnestPiece = 0.6;
    static constexpr double arrowMelt = 0.55;
    static constexpr double arrowDropRadius = 5.0;

    Tuning tuning;
    QPointF head;
    std::vector<QPointF> nodes;
    std::vector<Spring> glyphSprings;
    Spring liveliness{0.0};
    Spring press{1.0};
    Spring presence{0.0};
    Spring aiming{0.0};
    Spring arrowness{0.0};
    Spring shakeScale{1.0};
    std::vector<Droplet> droplets;
    double speed = 0.0;
    bool wasPressed = false;

    // Shake-to-enlarge detection (~20 rapid direction reversals threshold, continuous growth)
    double m_time = 0.0;
    double m_shakeHoldTimer = 0.0;
    double m_lastReversalTime = 0.0;
    int m_shakeStreak = 0;
    QPointF m_strokeDir{0.0, 0.0};
    double m_strokeDist = 0.0;

    CursorContext m_context = CursorContext::pointer();

    explicit LiquidBody(const QPointF &point = QPointF(0.0, 0.0));

    bool isSettled() const;
    void step(const QPointF &point, double dt, const CursorContext &context, bool pressed, bool present);
    Shape shape() const;

private:
    QPointF anchor() const;
    void splash();
    double tailLength() const;
    static double smoothstep(double x);
    std::vector<QPointF> liquidArrow(double size, double arrow, double softness) const;
    std::vector<std::vector<QPointF>> doubleArrow(const QPointF &direction, double size) const;
    QPointF relative(const QPointF &pt) const;

    struct ArrowSample {
        QPointF arrow;
        QPointF circle;
    };
    static const std::vector<ArrowSample>& getArrowOutline();
    static QPointF getArrowMiddle();
};
