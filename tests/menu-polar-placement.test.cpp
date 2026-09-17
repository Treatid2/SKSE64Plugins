#include "../skee64/MenuPolarPlacementPolicy.h"
#include <cassert>
#include <cstdio>
using V=std::array<float,3>;
float Dot(const V& a,const V& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
bool Near(float a,float b) { return std::abs(a-b)<0.00001F; }
int main()
{
    auto f=SKEE::VR::MakePolarFrame(0,1,30,0);
    assert(Near(f.radial[0],-0.5F)); // left of +Y is -X, not +X
    assert(Near(f.radial[1],std::sqrt(0.75F)));
    for (float heading : {0.F,30.F,90.F,160.F}) for(float azimuth:{-85.F,0.F,38.F,85.F}) for(float elevation:{-60.F,0.F,25.F,60.F}) {
        const float h=heading*0.01745329251994329577F;
        f=SKEE::VR::MakePolarFrame(std::cos(h),std::sin(h),azimuth,elevation);
        assert(Near(Dot(f.radial,f.radial),1)); assert(Near(Dot(f.right,f.right),1)); assert(Near(Dot(f.up,f.up),1));
        assert(Near(Dot(f.radial,f.right),0)); assert(Near(Dot(f.radial,f.up),0)); assert(Near(Dot(f.up,f.right),0));
        // Menu U/V axes are orthogonal to the viewer-to-centre radial vector.
        // Radius changes magnitude, not angles; uniform scaling preserves aspect.
        assert(Near(Dot(f.radial,f.right)*100,0));
    }
    for(float elevation:{-60.F,0.F,60.F}) for(float distance:{30.F,100.F,300.F}) for(float height:{-150.F,-10.F,0.F,10.F,150.F}) {
        const auto original=SKEE::VR::MakePolarFrame(0,1,30,elevation);
        const auto p=SKEE::VR::MakeRaisedPolarPlacement(0,1,30,elevation,distance,height);
        assert(Near(p.offset[0],original.radial[0]*distance));
        assert(Near(p.offset[1],original.radial[1]*distance));
        assert(Near(p.offset[2],original.radial[2]*distance+height));
        auto direction=p.offset;
        const auto length=std::sqrt(Dot(direction,direction));
        for(auto& c:direction) c/=length;
        assert(Near(Dot(direction,p.frame.radial),1));
        assert(Near(Dot(direction,p.frame.right),0));
        assert(Near(Dot(direction,p.frame.up),0));
        if(height==0) { assert(p.frame.radial==original.radial); assert(p.frame.up==original.up); }
    }
    std::puts("Polar placement: positive-left convention and level/raised orthonormal viewer-facing frames passed.");
}
