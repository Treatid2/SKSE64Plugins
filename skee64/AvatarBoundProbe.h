// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace SKEE::BoundProbe
{
    using Point = std::array<double, 3>;
    struct Sphere { Point center{}; double radius{}; };
    inline bool Finite(const Point& p)
    { return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]); }
    inline bool Valid(const Sphere& s)
    { return Finite(s.center) && std::isfinite(s.radius) && s.radius >= 0; }
    inline double Distance(const Point& a, const Point& b)
    { return std::hypot(a[0]-b[0], a[1]-b[1], a[2]-b[2]); }
    // A sphere about the pivot conservatively contains every yaw of each
    // sampled sphere. This says nothing about stale/unreported skin bounds.
    inline bool Include(Sphere& envelope, const Sphere& sample)
    {
        if (!Valid(envelope) || !Valid(sample)) return false;
        if (sample.radius == 0) return true; // Engine empty bound; ignore center.
        envelope.radius = std::max(envelope.radius,
            Distance(envelope.center, sample.center) + sample.radius);
        return std::isfinite(envelope.radius);
    }
    inline bool Contains(const Sphere& container, const Sphere& envelope, double margin)
    {
        return Valid(container) && Valid(envelope) && container.radius > 0 &&
            envelope.radius > 0 && std::isfinite(margin) && margin >= 0 &&
            Distance(container.center, envelope.center) + envelope.radius + margin <= container.radius;
    }
}
