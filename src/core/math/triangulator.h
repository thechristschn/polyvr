#ifndef TRIANGULATOR_H_INCLUDED
#define TRIANGULATOR_H_INCLUDED

#include "polygon.h"
#include "core/objects/VRObjectFwd.h"
#include <string>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Triangulator {
    public:
        struct GeoData;
        GeoData* geo = 0;

    private:
        vector<VRPolygon> outer_bounds;
        vector<VRPolygon> inner_bounds;

        void testQuad();
        void tessellate();

    public:
        Triangulator();
        ~Triangulator();
        static shared_ptr<Triangulator> create();

        void add(VRPolygon p, bool outer = true);

        VRGeometryPtr compute();
};

OSG_END_NAMESPACE;

#endif // TRIANGULATOR_H_INCLUDED
