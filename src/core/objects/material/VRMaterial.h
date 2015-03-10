#ifndef VRMATERIAL_H_INCLUDED
#define VRMATERIAL_H_INCLUDED

#include <OpenSG/OSGFieldContainerFields.h>
#include <OpenSG/OSGColor.h>

#include "core/objects/object/VRObject.h"

class VRVideo;

OSG_BEGIN_NAMESPACE;
using namespace std;

class Material; OSG_GEN_CONTAINERPTR(Material);
class Image; OSG_GEN_CONTAINERPTR(Image);
/*class ChunkMaterial; OSG_GEN_CONTAINERPTR(ChunkMaterial);
class MaterialChunk; OSG_GEN_CONTAINERPTR(MaterialChunk);
class BlendChunk; OSG_GEN_CONTAINERPTR(BlendChunk);
class TextureEnvChunk; OSG_GEN_CONTAINERPTR(TextureEnvChunk);
class TextureObjChunk; OSG_GEN_CONTAINERPTR(TextureObjChunk);
class LineChunk; OSG_GEN_CONTAINERPTR(LineChunk);
class PointChunk; OSG_GEN_CONTAINERPTR(PointChunk);
class PolygonChunk; OSG_GEN_CONTAINERPTR(PolygonChunk);
class TwoSidedLightingChunk; OSG_GEN_CONTAINERPTR(TwoSidedLightingChunk);
class ShaderProgramChunk; OSG_GEN_CONTAINERPTR(ShaderProgramChunk);*/
class ShaderProgram; OSG_GEN_CONTAINERPTR(ShaderProgram);
class MultiPassMaterial; OSG_GEN_CONTAINERPTR(MultiPassMaterial);

struct VRMatData;

Color4f toColor4f(Color3f c, float t = 1);
Color3f toColor3f(Color4f c);

class VRMaterial : public VRObject {
    public:
        static map<string, VRMaterial*> materials;
        static map<MaterialRecPtr, VRMaterial*> materialsByPtr;

    protected:
        MultiPassMaterialRecPtr passes;
        vector<VRMatData*> mats;
        int activePass = 0;

        VRObject* copy(vector<VRObject*> children);

        bool isCMat(MaterialUnrecPtr matPtr);
        bool isSMat(MaterialUnrecPtr matPtr);
        bool isSTMat(MaterialUnrecPtr matPtr);

        void saveContent(xmlpp::Element* e);
        void loadContent(xmlpp::Element* e);

    public:
        VRMaterial(string name);
        virtual ~VRMaterial();

        void setActivePass(int i);
        int getActivePass();
        int getNPasses();
        int addPass();
        void remPass(int i);

        static VRMaterial* getDefault();
        static VRMaterial* get(MaterialRecPtr mat);
        static VRMaterial* get(string s);
        static void clearAll();

        //** Set the material of the mesh **/
        void setMaterial(MaterialRecPtr mat);
        void resetDefault();

        /** Load a texture && apply it to the mesh as new material **/
        void setTexture(string img_path, bool alpha = true);
        void setTexture(ImageRecPtr img, bool alpha = true);
        void setTexture(char* data, int N, int format, Vec3i dims, bool isfloat);
        void setQRCode(string s, Vec3f fg, Vec3f bg, int offset);
        void setVideo(string vid_path);
        VRVideo* getVideo();

        void toggleMaterial(string mat1, string mat2, bool b);

        void setDiffuse(Color3f c);
        void setSpecular(Color3f c);
        void setAmbient(Color3f c);
        void setShininess(float s);
        void setEmission(Color3f c);
        void setTransparency(float t);
        void setLineWidth(int w);
        void setPointSize(int s);
        void setWireFrame(bool b);
        void setZOffset(float factor, float bias);
        void setFronBackDraw(int front, int back);

        Color3f getDiffuse();
        Color3f getSpecular();
        Color3f getAmbient();
        float getShininess();
        Color3f getEmission();
        float getTransparency();

        void initShaderChunk();
        void setVertexShader(string s);
        void setFragmentShader(string s);
        void setGeometryShader(string s);
        void readVertexShader(string s);
        void readFragmentShader(string s);
        void readGeometryShader(string s);
        void setVertexScript(string script);
        void setFragmentScript(string script);
        void setGeometryScript(string script);
        string getVertexShader();
        string getFragmentShader();
        string getGeometryShader();
        string getVertexScript();
        string getFragmentScript();
        string getGeometryScript();
        ShaderProgramRecPtr getShaderProgram();

        template<class T> void setShaderParameter(string name, const T &value);

        void setMagMinFilter(string mag, string min);

        void setLit(bool b);
        bool isLit();

        /** Returns the mesh material **/
        MaterialRecPtr getMaterial();

        /** Returns the texture || 0 **/
        ImageRecPtr getTexture();

        /** Deprecated  **/
        void printMaterialColors();

        //void setLit(bool b) { getMesh()->getMaterial()->setLit(b); }//setLit only for simplematerials :/
};

OSG_END_NAMESPACE;

#endif // VRMATERIAL_H_INCLUDED
