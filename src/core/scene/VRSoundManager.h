#ifndef VRSOUNDMANAGER_H_INCLUDED
#define VRSOUNDMANAGER_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <map>

namespace boost{ class thread; }

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VRSound;
struct VRSoundContext;

class VRSoundManager {
    private:
        VRSoundContext* context = 0;
        VRSound* bgMusic = 0;
        map<string, VRSound*> sounds;
        vector<string> sound_jobs;

        VRSoundManager();
        VRSound* getSound(string filename);
        void clearSoundMap(void);

        bool t_running = true;
        boost::thread* s_thread = 0;
        void soundThread();

        void play(VRSound* sound);

    public:
        static VRSoundManager& get();
        ~VRSoundManager();

        void playMusic(string filename);
        void stopMusic(void);

        void playSound(string filename);
        void playPositionalSound(string filename, Vec3f vec);

        void stopAllSounds(void);

        void setMusicVolume(float volume);
        void setSoundVolume(float volume);

        void updatePlayerPosition(Vec3f position, Vec3f forward);
};

OSG_END_NAMESPACE;

#endif // VRSOUNDMANAGER_H_INCLUDED