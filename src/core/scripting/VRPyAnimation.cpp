#include "VRPyAnimation.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

#include "core/scene/VRAnimationManagerT.h"

using namespace OSG;

simpleVRPyType(Animation, New_named_ptr);

PyMethodDef VRPyAnimation::methods[] = {
    {"start", PyWrapOpt(Animation, start, "Start animation, pass an optional offset in seconds", "0", void, float) },
    {"stop", PyWrap(Animation, stop, "Stop animation", void) },
    {"isActive", PyWrap(Animation, isActive, "Check if running", bool) },
    {"setCallback", PyWrap(Animation, setCallback, "Set animation callback", void, VRAnimCbPtr) },
    {"setDuration", PyWrap(Animation, setDuration, "Set animation duration", void, float) },
    {"setLoop", PyWrap(Animation, setLoop, "Set animation loop flag", void, bool) },
    {NULL}  /* Sentinel */
};


