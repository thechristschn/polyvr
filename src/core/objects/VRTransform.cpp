
#include "core/utils/toString.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/VRDoublebuffer.h"
#include "core/scene/VRAnimationManagerT.h"
#include "VRTransform.h"
#include "geometry/VRPhysics.h"
#include "core/math/path.h"
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMatrixUtility.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRObject* VRTransform::copy(vector<VRObject*> children) {
    VRTransform* geo = new VRTransform(getBaseName());
    geo->setPickable(isPickable());
    geo->setMatrix(getMatrix());
    return geo;
}

void VRTransform::computeMatrix() {
    Matrix mm;
    MatrixLookAt(mm, _from, _at, _up);

    if (_scale != Vec3f(1,1,1)) {
        Matrix ms;
        ms.setScale(_scale);
        mm.mult(ms);
    }

    dm->write(mm);
}

//read matrix from doublebuffer and apply it to transformation
//should be called from the main thread only
void VRTransform::updatePhysics() {
    //update bullets transform
    if (noBlt and !held) { noBlt = false; return; }
    if (!physics->isPhysicalized()) return;

    Matrix m;
    dm->read(m);
    Matrix pm;
    getWorldMatrix(pm, true);
    pm.mult(m);

    physics->updateTransformation(pm);
    physics->pause();
    physics->resetForces();
}

void VRTransform::updateTransformation() {
    Matrix m;
    dm->read(m);
    t->setMatrix(m);
}

void VRTransform::reg_change() {
    if (change == false) {
        if (fixed) changedObjects.push_back(this);
        change = true;
        change_time_stamp = VRGlobals::get()->CURRENT_FRAME;
    }
}

//multiplizirt alle matrizen in dem vector zusammen
Matrix VRTransform::computeMatrixVector(vector<Matrix> tv) {
    if (tv.size() == 0) return Matrix();

    Matrix m;
    for (unsigned int i=0; i<tv.size(); i++) {
        m.mult(tv[tv.size()-i-1]);
    }

    return m;
}

void VRTransform::printInformation() { Matrix m; getMatrix(m); cout << " pos " << m[3]; }

/** initialise a point 3D object with his name **/
VRTransform::VRTransform(string name) : VRObject(name) {
    dm = new doubleBuffer;
    t = Transform::create();
    setCore(t, "Transform");

    _from = Vec3f(0,0,0);
    _at = Vec3f(0,0,-1); //equivalent to identity matrix!
    _up = Vec3f(0,1,0);
    _scale = Vec3f(1,1,1);

    orientation_mode = true;
    change = false;
    held = false;
    fixed = true;
    cam_invert_z = false;
    frame = 0;

    physics = new VRPhysics(this);
    noBlt = false;
    doTConstraint = false;
    doRConstraint = false;
    tConPlane = true;
    tConstraint = Vec3f(0,1,0);
    rConstraint = Vec3i(0,0,0);
    addAttachment("transform", 0);

    coords = 0;
    translator = 0;
}

VRTransform::~VRTransform() {
    dynamicObjects.remove(this);
    changedObjects.remove(this);
    delete physics;
    delete dm;
}

uint VRTransform::getLastChange() { return change_time_stamp; }

void VRTransform::initCoords() {
    if (coords != 0) return;

    coords = makeCoordAxis(0.3, 3, false);
    coords->setTravMask(0);
    addChild(coords);
    GeometryRecPtr geo = dynamic_cast<Geometry*>(coords->getCore());

    string shdr_vp =
    "void main( void ) {"
    "   gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;"
    "   gl_Position.z = -0.1;"
    "   gl_FrontColor = gl_Color;"
    "}";

    ChunkMaterialRecPtr mat = ChunkMaterial::create();
    mat->setSortKey(100);// render last
    SimpleSHLChunkRecPtr shader_chunk = SimpleSHLChunk::create();
    shader_chunk->setVertexProgram(shdr_vp.c_str());
    mat->addChunk(shader_chunk);

    geo->setMaterial(mat);
}

void VRTransform::initTranslator() { // TODO
    if (translator != 0) return;

    translator = makeCoordAxis(0.3, 3, false);
    translator->setTravMask(0);
    addChild(translator);
    GeometryRecPtr geo = dynamic_cast<Geometry*>(translator->getCore());

    string shdr_vp =
    "void main( void ) {"
    "   gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;"
    "   gl_Position.z = -0.1;"
    "   gl_FrontColor = gl_Color;"
    "}";

    /*string shdr_fp =
    "void main( void ) {"
    "   gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;"
    "   gl_Position.z = -0.1;"
    "}";*/

    ChunkMaterialRecPtr mat = ChunkMaterial::create();
    mat->setSortKey(100);// render last
    SimpleSHLChunkRecPtr shader_chunk = SimpleSHLChunk::create();
    shader_chunk->setVertexProgram(shdr_vp.c_str());
    //shader_chunk->setVertexProgram(shdr_fp.c_str());
    mat->addChunk(shader_chunk);

    geo->setMaterial(mat);
}

Vec3f VRTransform::getFrom() { return _from; }
Vec3f VRTransform::getAt() { return _at; }
Vec3f VRTransform::getUp() { return _up; }

/** Returns the local matrix **/
//void VRTransform::getMatrix(Matrix& _m) { if(change) update(); dm.read(_m); }
void VRTransform::getMatrix(Matrix& _m) {
    if(change) {
        computeMatrix();
        updateTransformation();
    }
    dm->read(_m);
}

Matrix VRTransform::getMatrix() {
    Matrix m;
    getMatrix(m);
    return m;
}

bool VRTransform::checkWorldChange() {
    if (frame == 0) {
        frame = 1;
        return true;
    }

    if (hasGraphChanged()) return true;

    VRObject* obj = this;
    VRTransform* ent;
    while(obj) {
        if (obj->hasAttachment("transform")) {
            ent = (VRTransform*)obj;
            if (ent->change) return true;
        }
        obj = obj->getParent();
    }

    return false;
}

/** Returns the world matrix **/
void VRTransform::getWorldMatrix(Matrix& _m, bool parentOnly) {
    //if (checkWorldChange()) {
    if (true) {
        vector<Matrix> tv;

        Matrix m;
        VRObject* obj = this;
        if (parentOnly and obj->getParent() != 0) obj = obj->getParent();

        VRTransform* tmp;
        while(true) {
            if (obj->hasAttachment("transform")) {
                tmp = (VRTransform*)obj;
                tmp->getMatrix(m);
                //cout << "\nPARENT: " << tmp->getName();
                //cout << "\n" << m;
                tv.push_back(m);
            }

            if (obj->getParent() == 0) break;
            else obj = obj->getParent();
        }

        WorldTransformation = computeMatrixVector(tv);
    }

    _m = WorldTransformation;
    //if (getName() == "Box") cout << WorldTransformation << endl;
    //cout << "\nGETWM: " << getName() << endl << _m << endl;
}

Matrix VRTransform::getWorldMatrix(bool parentOnly) {
    Matrix m;
    getWorldMatrix(m, parentOnly);
    return m;
}

/** Returns the world Position **/
Vec3f VRTransform::getWorldPosition() {
    Matrix m;
    getWorldMatrix(m);
    return Vec3f(m[3]);
}

/** Returns the direction vector (not normalized) **/
Vec3f VRTransform::getDir() { return _at-_from; }

/** Returns the world direction vector (not normalized) **/
Vec3f VRTransform::getWorldDirection() {
    Matrix m;
    getWorldMatrix(m);
    return Vec3f(m[2]);
}

/** Set the object fixed or not **/
void VRTransform::setFixed(bool b) {
    if (b == fixed) return;
    fixed = b;

    if(b) { dynamicObjects.remove(this); return; }

    bool inDO = (std::find(dynamicObjects.begin(), dynamicObjects.end(),this) != dynamicObjects.end());
    if(!inDO) dynamicObjects.push_back(this);
}

/** Set the world matrix of the object **/
void VRTransform::setWorldMatrix(Matrix _m) {
    Matrix wm, lm;
    getWorldMatrix(wm);
    getMatrix(lm);

    wm.invert();

    lm.mult(wm);
    lm.mult(_m);

    setMatrix(lm);
}

/** Set the world position of the object **/
void VRTransform::setWorldPosition(Vec3f pos) {
    Vec3f tmp = getWorldPosition();

    _from = pos - tmp + _from;
    reg_change();
}

//local pose setter--------------------
/** Set the from vector **/
void VRTransform::setFrom(Vec3f pos) {
    //cout << "\nSet From : " << name << getID() << " : " << pos;
    Vec3f dir;
    if (orientation_mode) dir = _at - _from; // TODO: there may a better way
    _from = pos;
    if (orientation_mode) _at = _from + dir;
    reg_change();
}

/** Set the at vector **/
void VRTransform::setAt(Vec3f at) {
    _at = at;
    orientation_mode = false;
    reg_change();
}

/** Set the up vector **/
void VRTransform::setUp(Vec3f up) {
    _up = up;
    reg_change();
}

void VRTransform::setDir(Vec3f dir) {
    _at = _from + dir;
    orientation_mode = true;
    reg_change();
}

bool VRTransform::get_orientation_mode() { return orientation_mode; }
void VRTransform::set_orientation_mode(bool b) { orientation_mode = b; }

/** Set the orientation of the object with the at and up vectors **/
void VRTransform::setOrientation(Vec3f at, Vec3f up) {
    _at = at;
    _up = up;
    reg_change();
}

/** Set the pose of the object with the from, at and up vectors **/
void VRTransform::setPose(Vec3f from, Vec3f at, Vec3f up) {
    _from = from;
    _at = at;
    _up = up;
    reg_change();
}

/** Set the local matrix **/
void VRTransform::setMatrix(Matrix _m) {
    setPose(Vec3f(_m[3]), Vec3f(-_m[2]+_m[3]), Vec3f(_m[1]));
}
//-------------------------------------

void VRTransform::showCoordAxis(bool b) {
    initCoords();
    if (b) coords->setTravMask(0xffffffff);
    else coords->setTravMask(0);
}

/** Set the scale of the object, not implemented **/
void VRTransform::setScale(float s) { setScale(Vec3f(s,s,s)); }

void VRTransform::setScale(Vec3f s) {
    _scale = s;
    reg_change();
}

Vec3f VRTransform::getScale() { return _scale; }

/** Rotate the object around its up axis **/
void VRTransform::rotate(float a) {//rotate around up axis
    Vec3f d = _at - _from;

    Quaternion q = Quaternion(_up, a);
    q.multVec(d,d);

    _at = _from + d;

    reg_change();
    //cout << "\nRotating " << name << " " << a ;
}

void VRTransform::rotate(float a, Vec3f v) {//rotate around up axis
    Vec3f d = _at - _from;

    v.normalize();
    Quaternion q = Quaternion(v, a);
    q.multVec(d,d);
    q.multVec(_up,_up);

    _at = _from + d;

    reg_change();
}

        /** Rotate the object around its dir axis **/
void VRTransform::rotateUp(float a) {//rotate around _at axis
    Vec3f d = _at - _from;
    d.normalize();

    Quaternion q = Quaternion(d, a);
    q.multVec(_up,_up);

    reg_change();
    //cout << "\nRotating " << name << " " << a ;
}

        /** Rotate the object around its x axis **/
void VRTransform::rotateX(float a) {//rotate around x axis
    Vec3f dir = _at - _from;
    Vec3f d = dir.cross(_up);
    d.normalize();

    Quaternion q = Quaternion(d, a);
    q.multVec(_up,_up);
    q.multVec(dir,dir);
    _at = _from + dir;

    reg_change();
    //cout << "\nRotating " << name << " " << a ;
}

/** Rotate the object around the point where at indicates and the up axis **/
void VRTransform::rotateAround(float a) {//rotate around focus using up axis
    orientation_mode = false;
    Vec3f d = _at - _from;

    Quaternion q = Quaternion(_up, -a);
    q.multVec(d,d);

    _from = _at - d;

    reg_change();
}

/** translate the object with a vector v, this changes the from and at vector **/
void VRTransform::translate(Vec3f v) {
    _at += v;
    _from += v;
    reg_change();
}

/** translate the object by changing the from in direction of the at vector **/
void VRTransform::zoom(float d) {
    Vec3f dv = _at-_from;
    /*float norm = dv.length();
    dv /= norm;
    _from += dv*d*norm;*/
    _from += dv*d;
    reg_change();
}

/** Translate the object towards at **/
void VRTransform::move(float d) {
    Vec3f dv = _at-_from;
    dv.normalize();
    _at += dv*d;
    _from += dv*d;
    reg_change();
}

/** Drag the object with another **/
void VRTransform::drag(VRTransform* new_parent) {
    if (held) return;
    held = true;
    old_parent = getParent();
    setFixed(false);

    //showTranslator(true); //TODO

    Matrix m;
    getWorldMatrix(m);
    switchParent(new_parent);
    setWorldMatrix(m);
    physics->updateTransformation(m);
    physics->resetForces();
    physics->pause(true);
    reg_change();
}

/** Drop the object, this returns the object at its old place in hirarchy **/
void VRTransform::drop() {
    if (!held) return;
    held = false;
    setFixed(true);

    Matrix m;
    getWorldMatrix(m);

    switchParent(old_parent);

    setWorldMatrix(m);
    physics->updateTransformation(m);
    physics->resetForces();
    physics->pause(false);
    reg_change();
}

/** Cast a ray in world coordinates from the object in its local coordinates, -z axis defaults **/
Line VRTransform::castRay(Vec3f dir) {
    Matrix m;
    getWorldMatrix(m);

    m.mult(dir,dir); dir.normalize();
    Pnt3f p0 = Vec3f(m[3]) + dir*0.5;

    Line ray;
    ray.setValue(p0, dir);

    return ray;
}

/** Print the position of the object in local and world coords **/
void VRTransform::printPos() {
    Matrix wm, wm_osg, lm;
    getWorldMatrix(wm);
    wm_osg = getNode()->getToWorld();
    getMatrix(lm);
    cout << "Position of " << getName() << ", local: " << Vec3f(lm[3]) << ", world: " << Vec3f(wm[3]) << "  " << Vec3f(wm_osg[3]);
}

/** Print the positions of all the subtree **/
void VRTransform::printTransformationTree(int indent) {
    if(indent == 0) cout << "\nPrint Transformation Tree : ";

    cout << "\n";
    for (int i=0;i<indent;i++) cout << "  ";
    if (getType() == "Transform" or getType() == "Geometry") {
        VRTransform* _this = (VRTransform*) this;
        _this->printPos();
    }

    for (uint i=0;i<getChildrenCount();i++) {
        if (getChild(i)->getType() == "Transform" or getChild(i)->getType() == "Geometry") {
            VRTransform* tmp = (VRTransform*) getChild(i);
            tmp->printTransformationTree(indent+1);
        }
    }

    if(indent == 0) cout << "\n";
}

/** enable constraints on the object, 0 leaves the DOF free, 1 restricts it **/
void VRTransform::apply_constraints() {
    if (!doTConstraint and !doRConstraint) return;

    Matrix t = getWorldMatrix();//current position

    //rotation
    if (doRConstraint) {
        int qs = rConstraint[0]+rConstraint[1]+rConstraint[2];
        Matrix t0 = constraints_reference;

        if (qs == 3) for (int i=0;i<3;i++) t[i] = t0[i];

        if (qs == 2) {
            int u,v,w;
            if (rConstraint[0] == 0) { u = 0; v = 1; w = 2; }
            if (rConstraint[1] == 0) { u = 1; v = 0; w = 2; }
            if (rConstraint[2] == 0) { u = 2; v = 0; w = 1; }

            for (int i=0;i<3;i++) t[i][u] = t0[i][u]; //copy old transformation

            //normiere so das die b komponennte konstant bleibt
            for (int i=0;i<3;i++) {
                float a = 1-t[i][u]*t[i][u];
                if (a < 1e-6) {
                    t[i][v] = t0[i][v];
                    t[i][w] = t0[i][w];
                } else {
                    a /= (t0[i][v]*t0[i][v] + t0[i][w]*t0[i][w]);
                    a = sqrt(a);
                    t[i][v] *= a;
                    t[i][w] *= a;
                }
            }
        }

        if (qs == 1) {
            /*int u,v,w;
            if (rConstraint[0] == 1) { u = 0; v = 1; w = 2; }
            if (rConstraint[1] == 1) { u = 1; v = 0; w = 2; }
            if (rConstraint[2] == 1) { u = 2; v = 0; w = 1; }*/

            // TODO
        }
    }

    //translation
    if (doTConstraint) {
        //cout << "\nA";
        if (tConPlane) {
            float d = Vec3f(t[3] - constraints_reference[3]).dot(tConstraint);
            for (int i=0; i<3; i++) t[3][i] -= d*tConstraint[i];
        }
        else {
            Vec3f d = Vec3f(t[3] - constraints_reference[3]);
            d = d.dot(tConstraint)*tConstraint;
            for (int i=0; i<3; i++) t[3][i] = constraints_reference[3][i] + d[i];
        }
    }

    setWorldMatrix(t);
}

//void VRTransform::setBulletObject(btRigidBody* b) { physics->setObj(b); }
//btRigidBody* VRTransform::getBulletObject() { return physics->obj(); }

void VRTransform::updateFromBullet() {
    if (held) return;
    Matrix m = physics->getTransformation();
    setWorldMatrix(m);
    setNoBltFlag();
}

void VRTransform::setNoBltFlag() { noBlt = true; }

void VRTransform::setRestrictionReference(Matrix m) { constraints_reference = m; }
void VRTransform::toggleTConstraint(bool b) { doTConstraint = b; if (b) getWorldMatrix(constraints_reference); if(!doRConstraint) setFixed(!b); }
void VRTransform::toggleRConstraint(bool b) { doRConstraint = b; if (b) getWorldMatrix(constraints_reference); if(!doTConstraint) setFixed(!b); }
void VRTransform::setTConstraint(Vec3f trans) { tConstraint = trans; tConstraint.normalize(); }
void VRTransform::setTConstraintMode(bool plane) { tConPlane = plane; }
void VRTransform::setRConstraint(Vec3i rot) { rConstraint = rot; }

bool VRTransform::getTConstraintMode() { return tConPlane; }
Vec3f VRTransform::getTConstraint() { return tConstraint; }
Vec3i VRTransform::getRConstraint() { return rConstraint; }

bool VRTransform::hasTConstraint() { return doTConstraint; }
bool VRTransform::hasRConstraint() { return doRConstraint; }

VRPhysics* VRTransform::getPhysics() { return physics; }

/** Update the object OSG transformation **/
void VRTransform::update() {
    frame = 0;

    //if (checkWorldChange()) {
    apply_constraints();
    // }
    if (held) updatePhysics();

    if (!change) return;
    computeMatrix();
    updateTransformation();
    updatePhysics();
    change = false;
}

void VRTransform::saveContent(xmlpp::Element* e) {
    VRObject::saveContent(e);

    e->set_attribute("from", toString(_from).c_str());
    e->set_attribute("at", toString(_at).c_str());
    e->set_attribute("up", toString(_up).c_str());
    e->set_attribute("scale", toString(_scale).c_str());

    e->set_attribute("cT", toString(tConstraint).c_str());
    e->set_attribute("cR", toString(rConstraint).c_str());
    e->set_attribute("do_cT", toString(doTConstraint).c_str());
    e->set_attribute("do_cR", toString(doRConstraint).c_str());
    e->set_attribute("cT_mode", toString(int(tConPlane)).c_str());

    e->set_attribute("at_dir", toString(orientation_mode).c_str());
}

void VRTransform::loadContent(xmlpp::Element* e) {
    VRObject::loadContent(e);

    Vec3f f, a, u;
    f = toVec3f(e->get_attribute("from")->get_value());
    a = toVec3f(e->get_attribute("at")->get_value());
    u = toVec3f(e->get_attribute("up")->get_value());
    if (e->get_attribute("scale")) _scale = toVec3f(e->get_attribute("scale")->get_value());

    setPose(f, a, u);

    if (e->get_attribute("cT_mode")) tConPlane = toBool(e->get_attribute("cT_mode")->get_value());
    if (e->get_attribute("do_cT")) doTConstraint = toBool(e->get_attribute("do_cT")->get_value());
    if (e->get_attribute("do_cR")) doRConstraint = toBool(e->get_attribute("do_cR")->get_value());
    if (e->get_attribute("cT")) tConstraint = toVec3f(e->get_attribute("cT")->get_value());
    if (e->get_attribute("cR")) rConstraint = toVec3i(e->get_attribute("cR")->get_value());

    if (e->get_attribute("at_dir")) orientation_mode = toBool(e->get_attribute("at_dir")->get_value());

    if(doTConstraint or doRConstraint) setFixed(false);
}

void VRTransform::startPathAnimation(path* p, float t, bool redirect) {
    VRFunction<Vec3f> *fkt1, *fkt2;
    fkt1 = new VRFunction<Vec3f>("3DEntSetFrom", boost::bind(&VRTransform::setFrom, this, _1));
    fkt2 = new VRFunction<Vec3f>("3DEntSetDir", boost::bind(&VRTransform::setDir, this, _1));

    Vec3f p0, p1, n0, n1, c, d;
    p->getStartPoint(p0,n0,c);
    p->getEndPoint(p1,n1,c);
    d = getDir();

    VRScene* scene = VRSceneManager::get()->getActiveScene();
    int a1, a2;
    a1 = scene->addAnimation(t, 0, fkt1, p0, p1, false);
    if (redirect) a2 = scene->addAnimation(t, 0, fkt2, n0, n1, false);
    else a2 = scene->addAnimation(t, 0, fkt2, d, d, false);
    animations.push_back(a1);
    animations.push_back(a2);
}

void VRTransform::stopAnimation() {
    VRScene* scene = VRSceneManager::get()->getActiveScene();
    for (auto a : animations)
        scene->stopAnimation(a);
    animations.clear();
}

list<VRTransform* > VRTransform::dynamicObjects = list<VRTransform* >();
list<VRTransform* > VRTransform::changedObjects = list<VRTransform* >();

OSG_END_NAMESPACE;