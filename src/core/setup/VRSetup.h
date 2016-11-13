#ifndef VRSETUP_H_INCLUDED
#define VRSETUP_H_INCLUDED

#include "windows/VRViewManager.h"
#include "windows/VRWindowManager.h"
#include "devices/VRDeviceManager.h"
#include "tracking/VRPN.h"
#include "tracking/ART.h"
#include "core/scripting/VRScriptManager.h"
#include "core/scene/VRCallbackManager.h"
#include "core/utils/VRName.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/setup/VRSetupFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/scripting/VRScriptFwd.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;
class VRVisualLayer;

class VRSetup : public VRViewManager, public VRWindowManager, public VRDeviceManager/*, public VRScriptManager, public VRCallbackManager*/, public ART, public VRPN, public VRName {
    private:
        string cfgfile;
        string tracking;

        //map<string, VRScriptPtr> scripts;

        VRTransformPtr real_root = 0;
        VRTransformPtr user = 0;
        VRCameraPtr setup_cam = 0;

        VivePtr vive = 0;

        VRVisualLayerPtr setup_layer;
        VRVisualLayerPtr stats_layer;
        VRToggleCbPtr layer_setup_toggle;
        VRToggleCbPtr layer_stats_toggle;

        VRNetworkPtr network;

        void showStats(bool b);

        void parseSetup(xmlpp::Element* setup);

        void processOptions();

        xmlpp::Element* getElementChild(xmlpp::Element* e, string name);

    public:
        VRSetup(string name);
        ~VRSetup();

        static VRSetupPtr create(string name);
        static VRSetupPtr getCurrent();

        VRNetworkPtr getNetwork();

        VRTransformPtr getUser();

        void addObject(VRObjectPtr o);
        VRTransformPtr getRoot();
        VRTransformPtr getTracker(string t);

        void setScene( shared_ptr<VRScene> s);
        void showSetup(bool b);

        /*void addScript(string name);
        VRScriptPtr getScript(string name);
        map<string, VRScriptPtr> getScripts();*/

        void printOSG();

        void updateTracking();

        void save(string file);
        void load(string file);



        void setupLESCCAVELights(VRScenePtr scene); // TODO: temporary until scripts for VRSetup implemented!
};

OSG_END_NAMESPACE;

#endif // VRSETUP_H_INCLUDED
