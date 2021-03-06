#include "VRPyTrafficSimulation.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(TrafficSimulation, New_ptr);

PyMethodDef VRPyTrafficSimulation::methods[] = {
    {"setRoadNetwork", PyWrap( TrafficSimulation, setRoadNetwork, "Set road network", void, VRRoadNetworkPtr ) },
    {"setTrafficDensity", PyWrap( TrafficSimulation, setTrafficDensity, "Set overall traffic", void, float, int ) },
    {"addVehicleModel", PyWrap( TrafficSimulation, addVehicleModel, "Set vehicle geometry", int, VRObjectPtr ) },
    {"addUser", PyWrap( TrafficSimulation, addUser, "Set vehicle geometry", void, VRTransformPtr ) },
    {NULL}  /* Sentinel */
};
