#pragma once
#include <array>
#include <cmath>

namespace SKEE::VR
{
    struct PolarFrame { std::array<float,3> radial, right, up; };
    // Coordinate-system independent horizontal heading. Positive azimuth is
    // left (world-up cross forward), not NiMatrix3's clockwise MakeZRotation.
    inline PolarFrame MakePolarFrame(float x, float y, float azimuth, float elevation)
    {
        constexpr float radians=0.01745329251994329577F;
        const float a=azimuth*radians,e=elevation*radians;
        const float fx=x*std::cos(a)-y*std::sin(a),fy=x*std::sin(a)+y*std::cos(a);
        return {{fx*std::cos(e),fy*std::cos(e),std::sin(e)},
                {fy,-fx,0}, {-fx*std::sin(e),-fy*std::sin(e),std::cos(e)}};
    }
}
