#ifndef VRROADNETWORK_H_INCLUDED
#define VRROADNETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "core/math/graph.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRoadNetwork : public VRObject {
    private:
        GraphPtr graph;
        VRAsphaltPtr asphalt;
        VROntologyPtr ontology;
        VRPathtoolPtr tool;

        pathPtr toPath( VREntityPtr pathEntity, int resolution );

    public:
        VRRoadNetwork();
        ~VRRoadNetwork();

        static VRRoadNetworkPtr create();

        void setOntology(VROntologyPtr ontology);
        GraphPtr getGraph();
        void updateTexture();

        VREntityPtr addNode( Vec3f pos );
        VREntityPtr addLane( int direction, VREntityPtr road, float width );
        VREntityPtr addRoad( string name, vector<VREntityPtr> paths, int rID, string type );
        VREntityPtr addPath( string type, string name, vector<VREntityPtr> nodes, vector<Vec3f> normals );

        void computeIntersectionLanes( VREntityPtr intersection );
        void computeLanePaths( VREntityPtr road );
};

OSG_END_NAMESPACE;

#endif // VRROADNETWORK_H_INCLUDED