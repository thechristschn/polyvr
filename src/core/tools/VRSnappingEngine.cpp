#include "VRSnappingEngine.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRScene.h"
#include "core/math/Octree.h"
#include "core/utils/toString.h"
#include "core/utils/VRDoublebuffer.h"
#include "core/setup/devices/VRSignalT.h"

using namespace OSG;

template<> string typeName(const VRSnappingEngine::PRESET& o) { return "VRSnappingEngine::PRESET"; }
template<> string typeName(const VRSnappingEngine::Type& o) { return "VRSnappingEngine::Type"; }
template<> string typeName(const VRSnappingEngine::VRSnapCbPtr& o) { return "VRSnappingEngine::VRSnapCbPtr"; }

template<> int toValue(stringstream& ss, VRSnappingEngine::PRESET& e) {
    string s = ss.str();
    if (s == "SIMPLE_ALIGNMENT") { e = VRSnappingEngine::SIMPLE_ALIGNMENT; return true; }
    if (s == "SNAP_BACK") { e = VRSnappingEngine::SNAP_BACK; return true; }
    return false;
}

template<> int toValue(stringstream& ss, VRSnappingEngine::Type& e) {
    string s = ss.str();
    if (s == "NONE") { e = VRSnappingEngine::NONE; return true; }
    if (s == "POINT") { e = VRSnappingEngine::POINT; return true; }
    if (s == "LINE") { e = VRSnappingEngine::LINE; return true; }
    if (s == "PLANE") { e = VRSnappingEngine::PLANE; return true; }
    if (s == "POINT_LOCAL") { e = VRSnappingEngine::POINT_LOCAL; return true; }
    if (s == "LINE_LOCAL") { e = VRSnappingEngine::LINE_LOCAL; return true; }
    if (s == "PLANE_LOCAL") { e = VRSnappingEngine::PLANE_LOCAL; return true; }
    return false;
}

struct VRSnappingEngine::Rule {
    unsigned long long ID = 0;
    Type translation = NONE;
    Type orientation = NONE;
    Line prim_t, prim_o;
    Vec3d snapP;

    VRTransformPtr csys = 0;
    float distance = 1;
    float weight = 1;
    Matrix4d C;

    Rule(Type t, Type o, Line pt, Line po, float d, float w, VRTransformPtr l) :
        translation(t), orientation(o),
        prim_t(pt), prim_o(po), csys(l),
        distance(d), weight(w) {
        static unsigned long long i = 0;
        ID = i++;
    }

    Vec3d local(Vec3d p) {
        if (csys) {
            C = csys->getWorldMatrix();
            C.invert();
            Pnt3d pL;
            C.mult(p,pL);
            return Vec3d(pL);
        } else return p;
    }

    Vec3d getSnapPoint(Vec3d p) {
        if (translation == POINT) snapP = Vec3d(prim_t.getPosition());
        if (translation == LINE) snapP = Vec3d( prim_t.getClosestPoint( Vec3f(local(p)) ) ); // project on line
        if (translation == PLANE) {
            Plane pl(prim_t.getDirection(), prim_t.getPosition());
            p = local(p);
            float d = pl.distance(Pnt3f(p)); // project on plane
            snapP = p + Vec3d(d*pl.getNormal());
        }
        return snapP;
    }

    void snap(Matrix4d& m) {
        if (csys) C = csys->getWorldMatrix();

        if (orientation == POINT) {
            MatrixLookAt(m, snapP, snapP+Vec3d(prim_o.getPosition()), Vec3d(prim_o.getDirection()));
            m.multLeft(C);
        }
    }

    bool inRange(Vec3d pa, double& dmin) {
        Vec3d paL = local( pa );
        Vec3d psnap = getSnapPoint(pa);
        float D = (psnap-paL).length(); // check distance
        bool b = (D <= distance && D < dmin);
        if (b) dmin = D;
        return b;
    }
};

VRSnappingEngine::VRSnappingEngine() {
    hintGeo = VRGeometry::create("snapping_engine_hint");
    positions = new Octree(0.1);
    event = new EventSnap();
    snapSignal = VRSignal::create();

    updatePtr = VRUpdateCb::create("snapping engine update", boost::bind(&VRSnappingEngine::update, this) );
    VRScene::getCurrent()->addUpdateFkt(updatePtr, 999);
}

VRSnappingEngine::~VRSnappingEngine() {
    clear();
    if (event) delete event;
}

shared_ptr<VRSnappingEngine> VRSnappingEngine::create() { return shared_ptr<VRSnappingEngine>(new VRSnappingEngine()); }

void VRSnappingEngine::addCallback(VRSnapCbPtr cb) { callbacks.push_back(cb); }

void VRSnappingEngine::clear() {
    anchors.clear();
    positions->clear();
    objects.clear();
    for (auto r : rules) delete r.second;
    rules.clear();
    if (event) delete event;
    event = new EventSnap();
}


VRSnappingEngine::Type VRSnappingEngine::typeFromStr(string t) {
    if (t == "NONE") return NONE;
    if (t == "POINT") return POINT;
    if (t == "LINE") return LINE;
    if (t == "PLANE") return PLANE;
    if (t == "POINT_LOCAL") return POINT_LOCAL;
    if (t == "LINE_LOCAL") return LINE_LOCAL;
    if (t == "PLANE_LOCAL") return PLANE_LOCAL;
    cout << "Warning: VRSnappingEngine::" << t << " is not a Type.\n";
    return NONE;
}

int VRSnappingEngine::addRule(Type t, Type o, Line pt, Line po, float d, float w, VRTransformPtr l) {
    Rule* r = new Rule(t,o,pt,po,d,w,l);
    rules[r->ID] = r;
    return r->ID;
}

void VRSnappingEngine::remRule(int i) {
    if (rules.count(i) == 0) return;
    delete rules[i];
    rules.erase(i);
}

void VRSnappingEngine::addObjectAnchor(VRTransformPtr obj, VRTransformPtr a) {
    if (anchors.count(obj) == 0) anchors[obj] = vector<VRTransformPtr>();
    anchors[obj].push_back(a);
}

void VRSnappingEngine::clearObjectAnchors(VRTransformPtr obj) {
    if (anchors.count(obj)) anchors[obj].clear();
}

void VRSnappingEngine::remLocalRules(VRTransformPtr obj) {
    vector<int> d;
    for (auto r : rules) if (r.second->csys == obj) d.push_back(r.first);
    for (int i : d) remRule(i);
}

void VRSnappingEngine::addObject(VRTransformPtr obj, float weight) {
    if (!obj) return;
    objects[obj] = obj->getWorldMatrix();
    Vec3d p = obj->getWorldPosition();
    positions->add(p, obj.get());
}

void VRSnappingEngine::remObject(VRTransformPtr obj) {
    if (objects.count(obj)) objects.erase(obj);
}

void VRSnappingEngine::addTree(VRObjectPtr obj, float weight) {
    vector<VRObjectPtr> objs = obj->getObjectListByType("Geometry");
    for (auto o : objs) addObject(static_pointer_cast<VRTransform>(o), weight);
}

VRSignalPtr VRSnappingEngine::getSignalSnap() { return snapSignal; }

void VRSnappingEngine::update() {
    for (auto dev : VRSetup::getCurrent()->getDevices()) { // get dragged objects
        VRTransformPtr obj = dev.second->getDraggedObject();
        VRTransformPtr gobj = dev.second->getDraggedGhost();
        if (obj == 0 || gobj == 0) continue;
        if (objects.count(obj) == 0) continue;

        Matrix4d m = gobj->getWorldMatrix();
        Vec3d p = Vec3d(m[3]);

        bool lastEvent = event->snap;
        int lastEventID = event->snapID;
        event->snap = 0;
        int snapID = -1;

        double dmin = 1e9;

        for (auto ri : rules) {
            Rule* r = ri.second;
            if (r->csys == obj) continue;

            if (anchors.count(obj)) {
                for (auto a : anchors[obj]) {
                    Matrix4d maL = a->getMatrix();
                    Matrix4d maW = m; maW.mult(maL);
                    Vec3d pa = Vec3d(maW[3]);
                    maL.invert();

                    if (r->csys && anchors.count(r->csys)) { // TODO: not working yet!
                        Matrix4d m2 = r->csys->getWorldMatrix();
                        for (auto a : anchors[r->csys]) {
                            Matrix4d ma2L = a->getMatrix();
                            //Matrix4d ma2W = m; ma2W.mult(maL);
                            //Vec3d pa2 = Vec3d(ma2W[3]);
                            Vec3d pa2 = Vec3d(ma2L[3]);
                            snapID++;
                            if (!r->inRange(pa+pa2, dmin)) continue;

                            r->snapP += pa2;
                            Matrix4d mm = m;
                            r->snap(mm);
                            mm.mult(maL);
                            //ma2L.invert();
                            //mm.mult(ma2L);
                            event->set(obj, r->csys, mm, dev.second, 1, snapID);
                        }
                    } else {
                        snapID++;
                        if (!r->inRange(pa, dmin)) continue;
                        Matrix4d mm = m;
                        r->snap(mm);
                        mm.mult(maL);
                        event->set(obj, r->csys, mm, dev.second, 1, snapID);
                    }
                }
            } else {
                snapID++;
                if (!r->inRange(p, dmin)) continue;
                Matrix4d mm = m;
                r->snap(mm);
                event->set(obj, r->csys, mm, dev.second, 1, snapID);
            }

        }

        if (event->snap) obj->setWorldMatrix(event->m);
        if (lastEvent != event->snap || lastEventID != event->snapID) {
            if (event->o1 == obj) {
                snapSignal->trigger<EventSnap>(event);
                for (auto cb : callbacks) (*cb)(*event);
            }
        }
    }

    // update geo
    if (!hintGeo->isVisible()) return;
}

void VRSnappingEngine::setVisualHints(bool b) {
    showHints = b;
    hintGeo->setVisible(b);
}

void VRSnappingEngine::setPreset(PRESET preset) {
    clear();

    Line t0(Pnt3f(0,0,0), Vec3f(0,0,0));
    Line o0(Pnt3f(0,0,-1), Vec3f(0,1,0));

    switch(preset) {
        case SIMPLE_ALIGNMENT:
            addRule(POINT, POINT, t0, o0, 1, 1, 0);
            addRule(LINE, POINT, Line(Pnt3f(), Vec3f(1,0,0)), o0, 1, 1, 0);
            addRule(LINE, POINT, Line(Pnt3f(), Vec3f(0,1,0)), o0, 1, 1, 0);
            addRule(LINE, POINT, Line(Pnt3f(), Vec3f(0,0,1)), o0, 1, 1, 0);
            break;
        case SNAP_BACK:
            addRule(POINT, POINT, t0, o0, 1, 1, 0);
            break;
    }
}



