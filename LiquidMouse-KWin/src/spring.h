#pragma once

#include <cmath>
#include <algorithm>

/// A value that chases its target like a mass on a damped spring.
/// Ported directly from LiquidMouse (Sources/LiquidMouse/LiquidBody.swift).
struct Spring {
    double value = 0.0;
    double target = 0.0;
    double velocity = 0.0;

    explicit Spring(double initial = 0.0) : value(initial), target(initial) {}

    bool isSettled() const {
        return value == target && velocity == 0.0;
    }

    /// damping is the damping ratio: below 1 the value overshoots and bounces before settling.
    void step(double dt, double frequency, double damping) {
        double omega = 2.0 * M_PI * frequency;
        // Stiff springs go unstable when a slow frame is integrated in one go.
        int steps = std::max(1, static_cast<int>(std::ceil(dt * 480.0)));
        double h = dt / static_cast<double>(steps);
        for (int i = 0; i < steps; ++i) {
            velocity += (omega * omega * (target - value) - 2.0 * damping * omega * velocity) * h;
            value += velocity * h;
        }
        if (std::abs(target - value) < 0.01 && std::abs(velocity) < 0.5) {
            value = target;
            velocity = 0.0;
        }
    }

    /// A sudden push, in units per second.
    void kick(double impulse) {
        velocity += impulse;
    }
};
