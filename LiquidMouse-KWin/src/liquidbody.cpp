#include "liquidbody.h"
#include <cmath>
#include <algorithm>
#include <random>

static double dist(const QPointF &a, const QPointF &b) {
    return std::hypot(a.x() - b.x(), a.y() - b.y());
}

static QPointF catmullRom(const QPointF &p0, const QPointF &p1, const QPointF &p2, const QPointF &p3, double t) {
    double t2 = t * t;
    double t3 = t2 * t;
    QPointF linear = (p2 - p0) * t;
    QPointF bend = 2.0 * p0 - 5.0 * p1;
    QPointF quadratic = (bend + 4.0 * p2 - p3) * t2;
    QPointF cubic = (3.0 * p1 - p0 - 3.0 * p2 + p3) * t3;
    return 0.5 * (2.0 * p1 + linear + quadratic + cubic);
}

LiquidBody::LiquidBody(const QPointF &point)
    : head(point), nodes(nodeCount, point) {
    Glyph g;
    auto vals = g.values();
    glyphSprings.reserve(vals.size());
    for (double v : vals) {
        glyphSprings.emplace_back(v);
    }
}

bool LiquidBody::isSettled() const {
    for (const auto &s : glyphSprings) {
        if (!s.isSettled()) return false;
    }
    if (!liveliness.isSettled() || !press.isSettled() || !aiming.isSettled() || !arrowness.isSettled() || !presence.isSettled() || !shakeScale.isSettled()) {
        return false;
    }
    if (!droplets.empty()) return false;
    if (m_shakeHoldTimer > 0.0 || shakeScale.target > 1.001) return false;
    QPointF anc = anchor();
    for (const auto &n : nodes) {
        if (n != anc) return false;
    }
    return true;
}

void LiquidBody::step(const QPointF &point, double dt, const CursorContext &context, bool pressed, bool present) {
    m_context = context;
    QPointF moveDelta = point - head;
    double moveDist = std::hypot(moveDelta.x(), moveDelta.y());
    bool warped = dt <= 0.0 || dt > 0.1 || moveDist > 600.0;
    double moved = warped ? 0.0 : moveDist;
    head = point;
    if (warped) {
        std::fill(nodes.begin(), nodes.end(), anchor());
        droplets.clear();
        m_shakeStreak = 0;
        m_strokeDist = 0.0;
    }
    if (dt <= 0.0) {
        wasPressed = pressed;
        return;
    }
    dt = std::min(dt, 0.1);

    Glyph target(context, tuning.form, tuning.dotRadius);
    auto targetVals = target.values();
    for (size_t i = 0; i < targetVals.size() && i < glyphSprings.size(); ++i) {
        glyphSprings[i].target = targetVals[i];
        glyphSprings[i].step(dt, 8.0, tuning.damping(0.85));
    }
    liveliness.target = target.isLively ? 1.0 : 0.0;
    liveliness.step(dt, 6.0, 1.0);

    press.target = pressed ? target.pressedScale : 1.0;
    if (pressed != wasPressed && target.isLively) {
        press.kick(pressed ? -10.0 : 6.0);
        if (pressed && tuning.splashes) {
            splash();
        }
    }
    wasPressed = pressed;
    press.step(dt, 9.0, tuning.damping(0.6));

    double slowdown = std::exp(-dropletDrag * dt);
    for (auto &drop : droplets) {
        drop.velocity *= slowdown;
        drop.position += drop.velocity * dt;
        drop.age += dt;
    }
    droplets.erase(std::remove_if(droplets.begin(), droplets.end(), [](const Droplet &d) {
        return d.age >= dropletLife;
    }), droplets.end());

    presence.target = present ? 1.0 : 0.0;
    if (present) {
        presence.step(dt, 5.0, tuning.damping(0.8));
    } else {
        presence.step(dt, 2.2, 1.0);
    }

    speed += (moved / dt - speed) * (1.0 - std::exp(-dt / speedSmoothing));
    if (!tuning.shrinksWhenAiming || !target.shrinksWhenAiming) {
        aiming.target = 0.0;
    } else if (moved > 0.0 && speed < aimingSpeed) {
        aiming.target = 1.0;
    } else if (speed > travellingSpeed) {
        aiming.target = 0.0;
    }
    aiming.step(dt, 7.0, tuning.damping(0.85));

    arrowness.target = target.isArrow ? 1.0 : 0.0;
    arrowness.step(dt, 7.0, tuning.damping(0.85));

    // Shake-to-enlarge: scuotimento iniziale non sensibile (richiede una ventina di cambi di direzione rapidi),
    // poi cresce continuamente mentre si continua a scuotere; quando ci si ferma aspetta ~1.8s e poi torna normale.
    if (dt > 0.0) {
        m_time += dt;

        // Se l'utente smette di cambiare direzione per più di 0.45s, la serie di scuotimento si resetta
        if ((m_time - m_lastReversalTime) > 0.45) {
            m_shakeStreak = 0;
        }

        // Riconoscimento inversioni di traiettoria nette (avanti/indietro)
        if (!warped && moveDist >= 2.5) {
            QPointF curDir = moveDelta / moveDist;
            if (m_strokeDist > 0.0) {
                double dot = curDir.x() * m_strokeDir.x() + curDir.y() * m_strokeDir.y();
                if (dot < -0.15) {
                    if (m_strokeDist >= 10.0) {
                        // Inversione valida!
                        if ((m_time - m_lastReversalTime) <= 0.45) {
                            m_shakeStreak++;
                        } else {
                            m_shakeStreak = 1;
                        }
                        m_lastReversalTime = m_time;

                        // Richiede un bel po' di scuotimento (~20 cambi rapidi di direzione)
                        // prima che inizi a crescere
                        const int shakeThreshold = 18;
                        if (m_shakeStreak >= shakeThreshold) {
                            if (shakeScale.target < 1.3) {
                                shakeScale.target = 1.35;
                            } else {
                                // Cresce costantemente a ogni cambio di direzione rapido (all'infinito fino a max safety)
                                shakeScale.target = std::min(15.0, shakeScale.target + 0.22);
                            }
                            m_shakeHoldTimer = 1.8; // Aspetta ~1.8s quando ci si ferma
                        }
                    }
                    m_strokeDir = curDir;
                    m_strokeDist = moveDist;
                } else {
                    // Si continua nella stessa direzione della corsa
                    m_strokeDist += moveDist;
                }
            } else {
                m_strokeDir = curDir;
                m_strokeDist = moveDist;
            }
        }

        // Mentre si continua a scuotere oltre la soglia dei ~20 cambi, alimenta la crescita continua
        if (m_shakeStreak >= 18 && !warped && moveDist > 2.5) {
            shakeScale.target = std::min(15.0, shakeScale.target + 0.7 * dt);
            m_shakeHoldTimer = 1.8;
        }

        // Quando ci si ferma, aspetta m_shakeHoldTimer (~1.8s) e poi torna normale
        if (m_shakeHoldTimer > 0.0) {
            m_shakeHoldTimer -= dt;
        } else {
            shakeScale.target = 1.0;
        }

        shakeScale.step(dt, 9.5, tuning.damping(0.8));
    }

    double follow = 1.0 - std::exp(-tuning.trailResponse * dt);
    double longestGap = tuning.trailLength / static_cast<double>(nodeCount);
    QPointF ahead = anchor();
    for (auto &node : nodes) {
        QPointF nextNode = node + (ahead - node) * follow;
        double gap = dist(nextNode, ahead);
        if (gap > longestGap) {
            nextNode = longestGap > 0.0 ? ahead + (nextNode - ahead) * (longestGap / gap) : ahead;
        } else if (gap < 0.02) {
            nextNode = ahead;
        }
        node = nextNode;
        ahead = node;
    }
}

QPointF LiquidBody::anchor() const {
    double scale = (tuning.dotRadius / Glyph::designRadius) * std::max(arrowness.value, 0.0) * std::max(shakeScale.value, 0.5);
    return head + getArrowMiddle() * scale;
}

void LiquidBody::splash() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> distAngle(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> distJitter(-0.25, 0.25);
    std::uniform_real_distribution<double> distSpeed(200.0, 320.0);

    double phase = distAngle(rng);
    double size = tuning.dotRadius / Glyph::designRadius;
    for (int i = 0; i < dropletCount; ++i) {
        double angle = phase + 2.0 * M_PI * (static_cast<double>(i) + distJitter(rng)) / static_cast<double>(dropletCount);
        double spd = distSpeed(rng) * size;
        droplets.push_back(Droplet{head, QPointF(std::cos(angle) * spd, std::sin(angle) * spd), 0.0});
    }
}

double LiquidBody::tailLength() const {
    double length = 0.0;
    QPointF ahead = anchor();
    for (const auto &node : nodes) {
        length += dist(node, ahead);
        ahead = node;
    }
    return length;
}

double LiquidBody::smoothstep(double x) {
    double t = std::clamp(x, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

LiquidBody::Shape LiquidBody::shape() const {
    double scale = std::max(press.value, 0.0) * std::max(presence.value, 0.0) * std::max(shakeScale.value, 0.5);
    Glyph current;
    std::vector<double> scaledVals;
    scaledVals.reserve(glyphSprings.size());
    for (const auto &s : glyphSprings) {
        scaledVals.push_back(s.value * scale);
    }
    current.setValues(scaledVals);

    double lively = std::clamp(liveliness.value, 0.0, 1.0);
    double aimed = 1.0 - aimingShrink * std::clamp(aiming.value, 0.0, 1.0);
    current.core *= aimed;
    current.tail *= aimed;
    current.ringRadius *= aimed;
    current.ringWidth *= aimed;

    Shape s;
    if (m_context.kind == CursorContextKind::Resize) {
        s.outlines = doubleArrow(m_context.direction, tuning.dotRadius * std::max(shakeScale.value, 0.5));
        return s;
    }

    double arrow = std::clamp(arrowness.value, 0.0, 1.1);
    double tail = tailLength();
    double melt = smoothstep(tail / (6.0 * std::max(current.core, 1.0))) * lively;

    if (arrow > 0.001 && current.core > thinnestPiece) {
        s.outline = liquidArrow(current.core, arrow, melt);
    } else if (current.core > thinnestPiece) {
        s.strokes.push_back({Blob{QPointF(0.0, 0.0), current.core}});
    }

    for (const auto &limb : current.limbs) {
        if (std::max(limb.startRadius, limb.endRadius) > thinnestPiece) {
            s.strokes.push_back({
                Blob{limb.start, std::max(limb.startRadius, 0.0)},
                Blob{limb.end, std::max(limb.endRadius, 0.0)}
            });
        }
    }

    if (current.ringWidth > thinnestPiece) {
        s.rings.push_back(Ring{current.ringCenter, std::max(current.ringRadius, 0.0), current.ringWidth});
    }

    double emerging = smoothstep((tail - 1.5 * current.tail) / (6.0 * std::max(current.tail, 1.0)));
    double arrowTail = (0.42 + 0.3 * melt) * emerging;
    double thickest = current.tail * (0.9 + (arrowTail - 0.9) * std::min(arrow, 1.0)) * lively;

    if (thickest > thinnestPiece && nodes.back() != anchor()) {
        std::vector<QPointF> points;
        points.reserve(nodes.size() + 1);
        points.push_back(anchor());
        points.insert(points.end(), nodes.begin(), nodes.end());

        int subdivisions = 3;
        std::vector<Blob> trail;
        int totalSegments = static_cast<int>(points.size()) - 1;

        for (int segment = 0; segment < totalSegments; ++segment) {
            QPointF p0 = points[std::max(segment - 1, 0)];
            QPointF p1 = points[segment];
            QPointF p2 = points[segment + 1];
            QPointF p3 = points[std::min(segment + 2, totalSegments)];

            for (int step = 0; step < subdivisions; ++step) {
                double t = static_cast<double>(step) / static_cast<double>(subdivisions);
                double along = (static_cast<double>(segment) + t) / static_cast<double>(totalSegments);
                double radius = thickest * std::pow(1.0 - along, 1.5);
                if (radius <= thinnestPiece) break;
                trail.push_back(Blob{relative(catmullRom(p0, p1, p2, p3, t)), radius});
            }
        }
        if (!trail.empty()) {
            s.strokes.push_back(trail);
        }
    }

    for (const auto &drop : droplets) {
        double life = drop.age / dropletLife;
        double radius = tuning.dotRadius * 0.34 * (1.0 - life * life);
        if (radius > thinnestPiece) {
            s.splash.push_back({Blob{relative(drop.position), radius}});
        }
    }

    return s;
}

std::vector<QPointF> LiquidBody::liquidArrow(double size, double arrow, double softness) const {
    double scale = size / Glyph::designRadius;
    double roundness = arrowMelt * softness;
    double shrink = 1.0 - 0.15 * softness;
    QPointF middle = getArrowMiddle() * scale;

    const auto &outline = getArrowOutline();
    std::vector<QPointF> result;
    result.reserve(outline.size());

    for (const auto &sample : outline) {
        QPointF pointed = sample.arrow * scale;
        QPointF unit = sample.circle;
        QPointF rounded = middle + unit * (arrowDropRadius * scale);
        QPointF liquid = middle + (pointed + (rounded - pointed) * roundness - middle) * shrink;
        QPointF dot = unit * size;
        QPointF pt = dot + (liquid - dot) * arrow;
        result.push_back(pt);
    }
    return result;
}

QPointF LiquidBody::relative(const QPointF &pt) const {
    return QPointF(pt.x() - head.x(), pt.y() - head.y());
}

const std::vector<LiquidBody::ArrowSample>& LiquidBody::getArrowOutline() {
    static std::vector<ArrowSample> samples = []() {
        std::vector<QPointF> points = {
            QPointF(0.0, 0.0),
            QPointF(0.0, 16.0),
            QPointF(4.4, 12.0),
            QPointF(11.5, 11.5)
        };
        double cut = 0.1;
        for (int iter = 0; iter < 3; ++iter) {
            std::vector<QPointF> smoother;
            int count = points.size();
            for (int i = 0; i < count; ++i) {
                const auto &p = points[i];
                const auto &next = points[(i + 1) % count];
                smoother.push_back(p * (1.0 - cut) + next * cut);
                smoother.push_back(p * cut + next * (1.0 - cut));
            }
            points = std::move(smoother);
        }

        // Align tip on (0, 0)
        auto tipIt = std::min_element(points.begin(), points.end(), [](const QPointF &a, const QPointF &b) {
            return (a.x() + a.y()) < (b.x() + b.y());
        });
        QPointF tip = *tipIt;
        for (auto &p : points) {
            p -= tip;
        }
        size_t startIndex = std::distance(points.begin(), tipIt);
        std::vector<QPointF> reordered(points.begin() + startIndex, points.end());
        reordered.insert(reordered.end(), points.begin(), points.begin() + startIndex);
        reordered.push_back(QPointF(0.0, 0.0));

        std::vector<double> lengths = {0.0};
        for (size_t i = 1; i < reordered.size(); ++i) {
            lengths.push_back(lengths.back() + dist(reordered[i], reordered[i - 1]));
        }
        double total = lengths.back();
        int count = 72;
        size_t segment = 1;
        std::vector<ArrowSample> result;
        result.reserve(count);

        for (int sample = 0; sample < count; ++sample) {
            double distance = total * static_cast<double>(sample) / static_cast<double>(count);
            while (segment < lengths.size() && lengths[segment] < distance) {
                ++segment;
            }
            if (segment >= lengths.size()) segment = lengths.size() - 1;
            double segLen = lengths[segment] - lengths[segment - 1];
            double t = segLen > 1e-9 ? (distance - lengths[segment - 1]) / segLen : 0.0;
            QPointF a = reordered[segment - 1];
            QPointF b = reordered[segment];
            QPointF arrowPt = a + (b - a) * t;

            double angle = 0.67 * M_PI + 2.0 * M_PI * static_cast<double>(sample) / static_cast<double>(count);
            QPointF circlePt(std::cos(angle), std::sin(angle));
            result.push_back(ArrowSample{arrowPt, circlePt});
        }
        return result;
    }();
    return samples;
}

QPointF LiquidBody::getArrowMiddle() {
    static QPointF mid = []() {
        const auto &outline = getArrowOutline();
        QPointF sum(0.0, 0.0);
        for (const auto &sample : outline) {
            sum += sample.arrow;
        }
        return sum / static_cast<double>(outline.size());
    }();
    return mid;
}

static std::vector<QPointF> smoothArrowPolygon(const std::vector<QPointF> &rawPoints) {
    std::vector<QPointF> points = rawPoints;
    double cut = 0.10;
    for (int iter = 0; iter < 3; ++iter) {
        std::vector<QPointF> smoother;
        smoother.reserve(points.size() * 2);
        int count = static_cast<int>(points.size());
        for (int i = 0; i < count; ++i) {
            const auto &p = points[i];
            const auto &next = points[(i + 1) % count];
            smoother.push_back(p * (1.0 - cut) + next * cut);
            smoother.push_back(p * cut + next * (1.0 - cut));
        }
        points = std::move(smoother);
    }
    return points;
}

std::vector<std::vector<QPointF>> LiquidBody::doubleArrow(const QPointF &direction, double size) const {
    double scale = size / Glyph::designRadius;
    double len = std::hypot(direction.x(), direction.y());
    if (len < 1e-6) len = 1.0;
    QPointF way = direction / len;
    QPointF normal(-way.y(), way.x());

    // Due frecce identiche alla sagoma del cursore principale (Arrow),
    // specchiate, allineate diritte lungo l'asse e con uno spazio netto e pulito al centro (stile ◄   ►).
    double tipDist = 14.0 * scale;      // Posizione della punta rispetto al centro (span totale 28px)
    double sideLen = 11.0 * scale;      // Lunghezza laterale alette della singola freccia
    double halfWidth = 4.8 * scale;     // Semi-ampiezza alette (larghezza cuspide = 9.6px)
    double notchDepth = 8.2 * scale;    // Profondità dell'incavo posteriore
    // Spazio vuoto tra le due frecce al centro: ~6px tra le alette, ~11.6px tra gli incavi

    // 1. Freccia positiva (+way)
    std::vector<QPointF> raw1 = {
        way * tipDist,                                                  // Punta 1
        way * (tipDist - sideLen) - normal * halfWidth,                 // Aletta destra
        way * (tipDist - notchDepth),                                   // Incavo posteriore
        way * (tipDist - sideLen) + normal * halfWidth                  // Aletta sinistra
    };

    // 2. Freccia negativa (-way, specchiata a 180° rispetto al centro)
    std::vector<QPointF> raw2 = {
        -way * tipDist,                                                 // Punta 2
        -way * (tipDist - sideLen) + normal * halfWidth,                // Aletta opposta
        -way * (tipDist - notchDepth),                                  // Incavo opposto
        -way * (tipDist - sideLen) - normal * halfWidth                 // Aletta opposta
    };

    return {
        smoothArrowPolygon(raw1),
        smoothArrowPolygon(raw2)
    };
}

