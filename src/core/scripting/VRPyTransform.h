#ifndef VRPyTransform_H_INCLUDED
#define VRPyTransform_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRPhysics.h"

struct VRPyCollision : VRPyBaseT<OSG::VRCollision> {
    static PyMethodDef methods[];
};

struct VRPyTransform : VRPyBaseT<OSG::VRTransform> {
    static PyMethodDef methods[];

    static PyObject* fromSharedPtr(OSG::VRTransformPtr obj);

    static PyObject* setIdentity(VRPyTransform* self);
    static PyObject* translate(VRPyTransform* self, PyObject* args);
    static PyObject* move(VRPyTransform* self, PyObject* args);
    static PyObject* rotate(VRPyTransform* self, PyObject* args);
    static PyObject* rotateAround(VRPyTransform* self, PyObject* args);

    static PyObject* getWFrom(VRPyTransform* self);
    static PyObject* getFrom(VRPyTransform* self);
    static PyObject* getAt(VRPyTransform* self);
    static PyObject* getWorldDir(VRPyTransform* self);
    static PyObject* getWorldUp(VRPyTransform* self);
    static PyObject* getDir(VRPyTransform* self);
    static PyObject* getUp(VRPyTransform* self);
    static PyObject* getScale(VRPyTransform* self);
    static PyObject* getPose(VRPyTransform* self);
    static PyObject* getWorldPose(VRPyTransform* self);
    static PyObject* getEuler(VRPyTransform* self);

    static PyObject* setPose(VRPyTransform* self, PyObject* args);
    static PyObject* setWPose(VRPyTransform* self, PyObject* args);
    static PyObject* setWFrom(VRPyTransform* self, PyObject* args);
    static PyObject* setWOrientation(VRPyTransform* self, PyObject* args);
    static PyObject* setFrom(VRPyTransform* self, PyObject* args);
    static PyObject* setAt(VRPyTransform* self, PyObject* args);
    static PyObject* setDir(VRPyTransform* self, PyObject* args);
    static PyObject* setEuler(VRPyTransform* self, PyObject* args);
    static PyObject* setUp(VRPyTransform* self, PyObject* args);
    static PyObject* setScale(VRPyTransform* self, PyObject* args);

    static PyObject* drag(VRPyTransform* self, PyObject* args);
    static PyObject* drop(VRPyTransform* self);
    static PyObject* castRay(VRPyTransform* self, PyObject* args);
    static PyObject* getDragParent(VRPyTransform* self);

    static PyObject* duplicate(VRPyTransform* self);
    static PyObject* physicalize(VRPyTransform* self, PyObject *args);
    static PyObject* setGhost(VRPyTransform* self, PyObject *args);
    static PyObject* setPhysicsConstraintTo(VRPyTransform* self, PyObject *args);
    static PyObject* setMass(VRPyTransform* self, PyObject *args);
    static PyObject* setGravity(VRPyTransform* self, PyObject *args);
    static PyObject* setCollisionMargin(VRPyTransform* self, PyObject *args);
    static PyObject* setCollisionGroup(VRPyTransform* self, PyObject *args);
    static PyObject* setCollisionMask(VRPyTransform* self, PyObject *args);
    static PyObject* setCollisionShape(VRPyTransform* self, PyObject *args);
    static PyObject* getCollisions(VRPyTransform* self);
    static PyObject* setPhysicsActivationMode(VRPyTransform* self, PyObject *args);
    static PyObject* setPhysicalizeTree(VRPyTransform* self, PyObject *args);
    static PyObject* applyImpulse(VRPyTransform* self, PyObject *args);
    static PyObject* applyTorqueImpulse(VRPyTransform* self, PyObject *args);
    static PyObject* applyForce(VRPyTransform* self, PyObject *args);
    static PyObject* applyTorque(VRPyTransform* self, PyObject *args);
    static PyObject* applyConstantForce(VRPyTransform* self, PyObject *args);
    static PyObject* applyConstantTorque(VRPyTransform* self, PyObject *args);
    static PyObject* getForce(VRPyTransform* self);
    static PyObject* getTorque(VRPyTransform* self);
    static PyObject* getConstraintAngleWith(VRPyTransform* self, PyObject *args);
    static PyObject* deletePhysicsConstraints(VRPyTransform* self, PyObject* args);
    static PyObject* setDamping(VRPyTransform* self, PyObject* args);
    static PyObject* applyChange(VRPyTransform* self);
    static PyObject* setCenterOfMass(VRPyTransform* self, PyObject* args);
    static PyObject* lastChanged(VRPyTransform* self);

    static PyObject* setConstraint(VRPyTransform* self, PyObject* args);
    static PyObject* getConstraint(VRPyTransform* self);
    static PyObject* setPointConstraints(VRPyTransform* self, PyObject* args);
    static PyObject* setPlaneConstraints(VRPyTransform* self, PyObject* args);
    static PyObject* setAxisConstraints(VRPyTransform* self, PyObject* args);
    static PyObject* setRotationConstraints(VRPyTransform* self, PyObject* args);

    static PyObject* animate(VRPyTransform* self, PyObject* args);
    static PyObject* animationStop(VRPyTransform* self);
    static PyObject* getAnimations(VRPyTransform* self);
    static PyObject* applyTransformation(VRPyTransform* self, PyObject *args);
};

#endif // VRPyTransform_H_INCLUDED
