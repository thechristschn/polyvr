#include "PolyVR.h"

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetupManager.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/coreDumpHandler.h"
#include "core/gui/VRGuiManager.h"
#include "core/networking/VRMainInterface.h"
#include "core/networking/VRSharedMemory.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scene/sound/VRSoundManager.h"
#include "core/objects/material/VRMaterial.h"
#include "addons/CEF/CEF.h"

#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGNode.h>
#include <GL/glut.h>
#include <unistd.h>
#include <termios.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

void printFieldContainer() {
    int N = FieldContainerFactory::the()->getNumTotalContainers();
    for (int i=0;i<N;++i) {
        FieldContainer* fc = FieldContainerFactory::the()->getContainer(i);
        if(fc == 0) continue;
        if(fc->getId() < 343) continue; // stuff created in osgInit()

        // skip prototypes
        if(fc->getType().getPrototype() == 0 || fc->getType().getPrototype() == fc  ) continue;

        //cout << "\nFC id: " << fc->getId() << flush;

        AttachmentContainer* ac = dynamic_cast<AttachmentContainer*>(fc);
        if (ac == 0) {
            Attachment* a = dynamic_cast<Attachment*>(fc);
            if (a != 0) {
                FieldContainer* dad = 0;
                if (a->getMFParents()->size() > 0) dad = a->getParents(0);
                ac = dynamic_cast<AttachmentContainer*>(dad);
            }
        }

        const Char8* name = getName(ac);
        if (name != 0) printf("Detected living FC %s (%s) %p refcount %d ID %d\n", fc->getTypeName(), name, fc, fc->getRefCount(), fc->getId());
        else printf( "Detected living FC %s (no name) %p refcount %d ID %d\n", fc->getTypeName(), fc, fc->getRefCount(), fc->getId() );
    }
}

PolyVR::PolyVR() {}
PolyVR::~PolyVR() {}

PolyVR* PolyVR::get() {
    static PolyVR* pvr = new PolyVR();
    return pvr;
}

void PolyVR::shutdown() {
    VRSharedMemory sm("PolyVR_System");
    int* i = sm.addObject<int>("identifier");
    *i = 0;

    auto pvr = get();
    pvr->scene_mgr->closeScene();
    pvr->scene_mgr->stopAllThreads();
    //pvr->setup_mgr->closeSetup();
    delete pvr;
    printFieldContainer();
    osgExit();
    std::exit(0);
}

void printNextOSGID(int i) {
    NodeRefPtr n = Node::create();
    cout << "next OSG ID: " << n->getId() << " at maker " << i << endl;
}

void PolyVR::setOption(string name, bool val) { options->setOption(name, val); }
void PolyVR::setOption(string name, string val) { options->setOption(name, val); }
void PolyVR::setOption(string name, int val) { options->setOption(name, val); }
void PolyVR::setOption(string name, float val) { options->setOption(name, val); }

void PolyVR::init(int argc, char **argv) {
    checkProcessesAndSockets();
    cout << "Init PolyVR\n\n";
    enableCoreDump(true);
    setlocale(LC_ALL, "C");
    options = shared_ptr<VROptions>(VROptions::get());
    options->parse(argc,argv);

    //GLUT
    glutInit(&argc, argv);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    //OSG
    ChangeList::setReadWriteDefault();
    OSG::preloadSharedObject("OSGFileIO");
    OSG::preloadSharedObject("OSGImageFileIO");
    cout << "Init OSG\n";
    osgInit(argc,argv);

    VRSharedMemory sm("PolyVR_System");
    int* i = sm.addObject<int>("identifier");
    *i = 1;
}

void PolyVR::run() {
    cout << "Start main loop\n";
    while(true) {
        VRSceneManager::get()->update();

        if (VRGlobals::CURRENT_FRAME == 300) {
            string app = options->getOption<string>("application");
            if (app != "") VRSceneManager::get()->loadScene(app);
        }
    }
}

void PolyVR::start(bool runit) {
    if (VROptions::get()->getOption<bool>("active_stereo"))
        glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STEREO | GLUT_STENCIL);
    else glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);

    cout << "Init Modules\n";
    sound_mgr = VRSoundManager::get();
    setup_mgr = shared_ptr<VRSetupManager>(VRSetupManager::get());
    scene_mgr = shared_ptr<VRSceneManager>(VRSceneManager::get());
    interface = shared_ptr<VRMainInterface>(VRMainInterface::get());
    monitor = shared_ptr<VRInternalMonitor>(VRInternalMonitor::get());
    gui_mgr = shared_ptr<VRGuiManager>(VRGuiManager::get());
    loader = shared_ptr<VRSceneLoader>(VRSceneLoader::get());

    //string app = options->getOption<string>("application");
    //if (app != "") VRSceneManager::get()->loadScene(app);
    removeFile("setup/.startup"); // remove startup failsafe
    if (runit) run();
}

void PolyVR::startTestScene(OSGObjectPtr n) {
    start(false);
    cout << "start test scene " << n << endl;
    VRSceneManager::get()->newScene("test");
    VRScene::getCurrent()->getRoot()->find("Headlight")->addChild(n);
    VRGuiManager::get()->wakeWindow();
    run();
}

string createTimeStamp() {
    time_t _tm =time(NULL );
    struct tm * curtime = localtime ( &_tm );
    return asctime(curtime);
}

char getch() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
                perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
                perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0)
                perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
                perror ("tcsetattr ~ICANON");
        return (buf);
}

void PolyVR::checkProcessesAndSockets() {
    // check for failed startup
    string timestamp;
    ifstream f1("setup/.startup"); getline(f1,timestamp); f1.close();
    if (timestamp != "") {
        bool handling_bad_startup = true;
        cout << "Warning! a previously failed startup has been detected that occurred at " << timestamp << endl;
        cout << "Hints: " << endl;
        cout << " - In some instances a corruption of the x server can prevent the creation of the GL context.. " << endl;
        cout << "     -> restart your system" << endl;
        do {
            cout << "\n\tChoose on of the following options to proceed:" << endl;
            cout << "\t1) resume startup" << endl;
            cout << "\t2) reset to default hardware setup" << endl;
            char c = getch();
            //char c = cin.get();
            if (c == '1') { cout << "\t\tresuming startup now..\n" << endl; break; }
            if (c == '2') { cout << "\t\tremoving local setup config 'setup/.local'" << endl; removeFile("setup/.local"); }
        } while (handling_bad_startup);
    }

    // store startup tmp file, removed after successful startup
    timestamp = createTimeStamp();
    ofstream f("setup/.startup"); f.write(timestamp.c_str(), timestamp.size()); f.close();

    // TODO!!
    VRSharedMemory sm("PolyVR_System");// check for running PolyVR process
    int i = sm.getObject<int>("identifier");
    if (i) cout << "Error: A PolyVR instance is already running!\n";
}


OSG_END_NAMESPACE;
