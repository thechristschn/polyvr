#include "VRTextureGenerator.h"
#include "VRPerlin.h"
#include "VRBricks.h"
#include "core/networking/VRSharedMemory.h"

#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGMatrixUtility.h>
#include "core/objects/material/VRTexture.h"
#include "core/math/path.h"
#include "core/math/polygon.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRTextureGenerator::VRTextureGenerator() {}
VRTextureGenerator::~VRTextureGenerator() {}
shared_ptr<VRTextureGenerator> VRTextureGenerator::create() { return shared_ptr<VRTextureGenerator>(new VRTextureGenerator()); }

void VRTextureGenerator::setSize(Vec3i dim, bool a) { width = dim[0]; height = dim[1]; depth = dim[2]; hasAlpha = a; }
void VRTextureGenerator::setSize(int w, int h, int d) { width = w; height = h; depth = d; }
Vec3i VRTextureGenerator::getSize() { return Vec3i(width, height, depth); };

void VRTextureGenerator::add(string type, float amount, Color3f c1, Color3f c2) {
    GEN_TYPE t = PERLIN;
    if (type == "Bricks") t = BRICKS;
    add(t, amount, c1, c2);
}

void VRTextureGenerator::add(GEN_TYPE type, float amount, Color3f c1, Color3f c2) {
    Layer l;
    l.amount = amount;
    l.type = type;
    l.c31 = c1;
    l.c32 = c2;
    l.Nchannels = 3;
    layers.push_back(l);
}

void VRTextureGenerator::add(string type, float amount, Color4f c1, Color4f c2) {
    GEN_TYPE t = PERLIN;
    if (type == "Bricks") t = BRICKS;
    add(t, amount, c1, c2);
}

void VRTextureGenerator::add(GEN_TYPE type, float amount, Color4f c1, Color4f c2) {
    Layer l;
    l.amount = amount;
    l.type = type;
    l.c41 = c1;
    l.c42 = c2;
    l.Nchannels = 4;
    layers.push_back(l);
}

void VRTextureGenerator::drawFill(Color4f c) {
    Layer l;
    l.type = FILL;
    l.c41 = c;
    layers.push_back(l);
}

void VRTextureGenerator::drawPixel(Vec3i p, Color4f c) {
    Layer l;
    l.type = PIXEL;
    l.p1 = p;
    l.c41 = c;
    layers.push_back(l);
}

void VRTextureGenerator::drawLine(Vec3d p1, Vec3d p2, Color4f c, float w) {
    Layer l;
    l.type = LINE;
    l.c31 = Vec3f(p1);
    l.c32 = Vec3f(p2);
    l.c41 = c;
    l.amount = w;
    layers.push_back(l);
}

void VRTextureGenerator::drawPath(PathPtr p, Color4f c, float w) {
    Layer l;
    l.type = PATH;
    l.p = p;
    l.c41 = c;
    l.amount = w;
    layers.push_back(l);
}

void VRTextureGenerator::drawPolygon(VRPolygonPtr p, Color4f c, float h) {
    Layer l;
    l.type = POLYGON;
    l.pgon = p;
    l.c41 = c;
    l.amount = h;
    layers.push_back(l);
}

template<typename T>
void VRTextureGenerator::applyFill(T* data, Color4f c) {
    for (int k=0; k<depth; k++) {
        for (int j=0; j<height; j++) {
            for (int i=0; i<width; i++) {
                applyPixel(data, Vec3i(i,j,k), c);
            }
        }
    }
}

bool VRTextureGenerator::inBox(Pnt3d& p, Vec3d& s) {
    if (abs(p[0]) > s[0]) return false;
    if (abs(p[1]) > s[1]) return false;
    if (abs(p[2]) > s[2]) return false;
    return true;
}

void VRTextureGenerator::applyPixel(Color4f* data, Vec3i p, Color4f c) {
    int d = p[2]*height*width + p[1]*width + p[0];
    int N = depth*height*width;
    if (d >= N || d < 0) { cout << "Warning: applyPixel failed, pixel " << d << " " << p << " " << width << " " << height << " " << depth << " out of range! (buffer size is " << N << ")" << endl; return; }
    data[d] = c;
}

void VRTextureGenerator::applyPixel(Color3f* data, Vec3i p, Color4f c) {
    int d = p[2]*height*width + p[1]*width + p[0];
    int N = depth*height*width;
    if (d >= N|| d < 0) { cout << "Warning: applyPixel failed, pixel " << d << " " << p << " " << width << " " << height << " " << depth << " out of range! (buffer size is " << N << ")" << endl; return; }
    data[d] = Color3f(c[0], c[1], c[2])*c[3] + data[d]*(1.0-c[3]);
}

Vec3i VRTextureGenerator::clamp(Vec3i p) {
    if (p[0] < 0) p[0] = 0;
    if (p[1] < 0) p[1] = 0;
    if (p[2] < 0) p[2] = 0;
    if (p[0] >= width)  p[0] = width-1;
    if (p[1] >= height) p[1] = height-1;
    if (p[2] >= depth)  p[2] = depth-1;
    return p;
}

Vec3d VRTextureGenerator::upscale(Vec3d& p) {
    p = Vec3d(p[0]*width, p[1]*height, p[2]*depth);
    return p;
}

template<typename T>
void VRTextureGenerator::applyLine(T* data, Vec3d p1, Vec3d p2, Color4f c, float w) { // Bresenham's
    auto upscale = [&](Vec3d& p) {
        p = Vec3d(p[0]*width, p[1]*height, p[2]*depth);
    };

    auto getLeadDim = [](Vec3d d) {
        Vec3i iDs = Vec3i(0,1,2);
        if (abs(d[1]) > abs(d[0]) && abs(d[1]) > abs(d[2])) iDs = Vec3i(1,0,2);
        if (abs(d[2]) > abs(d[0]) && abs(d[2]) > abs(d[1])) iDs = Vec3i(2,0,1);
        return iDs;
    };

    auto BresenhamPixels = [&](Vec3d p1, Vec3d p2) {
        vector<Vec3i> pixels;
        Vec3d d = p2-p1;
        Vec3i iDs = getLeadDim(d);
        int I = iDs[0];

        Vec2d derr; // slopes
        if (I == 0) derr = Vec2d(abs(d[1]/d[0]), abs(d[2]/d[0]));
        if (I == 1) derr = Vec2d(abs(d[0]/d[1]), abs(d[2]/d[1]));
        if (I == 2) derr = Vec2d(abs(d[0]/d[2]), abs(d[1]/d[2]));
        Vec2d err = derr - Vec2d(0.5,0.5);

        if (p1[I] > p2[I]) { swap(p1, p2); d *= -1; }
        Vec3i pi1 = Vec3i(p1);
        Vec3i pi2 = Vec3i(p2);
        if (pi1[I] == pi2[I]) pi2[I]++;
        Vec3i pi = pi1;

        int ky = 1; if (d[iDs[1]] < 0) ky = -1;
        int kz = 1; if (d[iDs[2]] < 0) kz = -1;

        for (; pi[I] < pi2[I]; pi[I]++) {
            pixels.push_back(pi);
            err += derr;
            if (err[0] >= 0.5) { pi[iDs[1]] += ky; err[0] -= 1.0; pixels.push_back(pi); }
            if (err[1] >= 0.5) { pi[iDs[2]] += kz; err[1] -= 1.0; pixels.push_back(pi); }
        }
        return pixels;
    };

    upscale(p1);
    upscale(p2);
    Vec3d d = p2-p1;
    Vec3i iDs = getLeadDim(d);
    Vec3d u = Vec3d(0,0,0); u[iDs[2]] = 1;
    Vec3d p3 = d.cross(u);
    Vec3d p4 = d.cross(p3);
    p3.normalize();
    p4.normalize();

    float W = w*0.5*width; // line width in pixel
    p3 *= W;
    p4 *= W;

    //cout << "BresenhamPixels p1 " << p1 << " p2 " << p2 << " p3 " << p3 << " p4 " << p4 << " d " << d << " u " << u << endl;

    auto pixels1 = BresenhamPixels(p1,p2);
    auto pixels2 = BresenhamPixels(-p3,p3);
    auto pixels3 = BresenhamPixels(-p4,p4);

    for (auto pi : pixels1) {
        for (auto pj : pixels2) {
            for (auto pk : pixels3) {
                applyPixel(data, clamp(pi+pj+pk), c);
            }
        }
    }
}

template<typename T>
void VRTextureGenerator::applyPath(T* data, PathPtr p, Color4f c, float w) {
    auto poses = p->getPoses();
    for (uint i=1; i<poses.size(); i++) {
        Pose& p1 = poses[i-1];
        Pose& p2 = poses[i];

        Vec2d A1 = Vec2d(p1.pos()-p1.x()*w*0.5);
        Vec2d B1 = Vec2d(p1.pos()+p1.x()*w*0.5);
        Vec2d A2 = Vec2d(p2.pos()-p2.x()*w*0.5);
        Vec2d B2 = Vec2d(p2.pos()+p2.x()*w*0.5);

        auto poly = VRPolygon::create();
        poly->addPoint(A1);
        poly->addPoint(B1);
        poly->addPoint(B2);
        poly->addPoint(A2);
        applyPolygon(data,poly,c,0);
    }
}

template<typename T>
void VRTextureGenerator::applyPolygon(T* data, VRPolygonPtr p, Color4f c, float h) {
    auto bb = p->getBoundingBox();
    Vec3d a = bb.min(); swap(a[1], a[2]);
    Vec3d b = bb.max(); swap(b[1], b[2]);
    Vec3i A = Vec3i( upscale( a ) ) - Vec3i(1,1,1);
    Vec3i B = Vec3i( upscale( b ) ) + Vec3i(1,1,1);
    //float texelSize = 1.0/width; // TODO: non square textures?
    for (int j=A[1]; j<B[1]; j++) {
        for (int i=A[0]; i<B[0]; i++) {
            Vec2d pos = Vec2d(float(i)/width, float(j)/height);
            double d;
            if (p->isInside(pos, d)) {
                for (int k=0; k<depth; k++) applyPixel(data, clamp(Vec3i(i,j,k)), c);
            } /*else if (d < texelSize) { // TODO: finish antialiasing feature
                float a = 1.0 - d*width;
                auto cc = c; cc[3] = a;
                cout << "VRTextureGenerator::applyPolygon a " << a << endl;
                //for (int k=0; k<depth; k++) applyPixel(data, clamp(Vec3i(i,j,k)), cc);
                for (int k=0; k<depth; k++) applyPixel(data, clamp(Vec3i(i,j,k)), Color4f(a,0,1,1));
                //for (int k=0; k<depth; k++) applyPixel(data, clamp(Vec3i(i,j,k)), Color4f(0,0,1,a));
            }*/
        }
    }
}

void VRTextureGenerator::clearStage() { layers.clear(); }

VRTexturePtr VRTextureGenerator::compose(int seed) { // TODO: optimise!
    srand(seed);
    Vec3i dims(width, height, depth);

    Color3f* data3 = new Color3f[width*height*depth];
    Color4f* data4 = new Color4f[width*height*depth];
    for (int i=0; i<width*height*depth; i++) data3[i] = Color3f(1,1,1);
    for (int i=0; i<width*height*depth; i++) data4[i] = Color4f(1,1,1,1);
    for (auto l : layers) {
        if (!hasAlpha) {
            if (l.type == BRICKS) VRBricks::apply(data3, dims, l.amount, l.c31, l.c32);
            if (l.type == PERLIN) VRPerlin::apply(data3, dims, l.amount, l.c31, l.c32);
            if (l.type == LINE) applyLine(data3, Vec3d(l.c31), Vec3d(l.c32), l.c41, l.amount);
            if (l.type == FILL) applyFill(data3, l.c41);
            if (l.type == PIXEL) applyPixel(data3, l.p1, l.c41);
            if (l.type == PATH) applyPath(data3, l.p, l.c41, l.amount);
            if (l.type == POLYGON) applyPolygon(data3, l.pgon, l.c41, l.amount);
        }
        if (hasAlpha) {
            if (l.type == BRICKS) VRBricks::apply(data4, dims, l.amount, l.c41, l.c42);
            if (l.type == PERLIN) VRPerlin::apply(data4, dims, l.amount, l.c41, l.c42);
            if (l.type == LINE) applyLine(data4, Vec3d(l.c31), Vec3d(l.c32), l.c41, l.amount);
            if (l.type == FILL) applyFill(data4, l.c41);
            if (l.type == PIXEL) applyPixel(data4, l.p1, l.c41);
            if (l.type == PATH) applyPath(data4, l.p, l.c41, l.amount);
            if (l.type == POLYGON) applyPolygon(data4, l.pgon, l.c41, l.amount);
        }
    }

    img = VRTexture::create();
    auto format = hasAlpha ? OSG::Image::OSG_RGBA_PF : OSG::Image::OSG_RGB_PF;
    if (hasAlpha) img->getImage()->set(format, width, height, depth, 0, 1, 0.0, (const uint8_t*)data4, OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    else       img->getImage()->set(format, width, height, depth, 0, 1, 0.0, (const uint8_t*)data3, OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    delete[] data3;
    delete[] data4;
    return img;
}

struct tex_params {
    Vec3i dims;
    int pixel_format = OSG::Image::OSG_RGB_PF;
    int data_type = OSG::Image::OSG_FLOAT32_IMAGEDATA;
    int internal_pixel_format = -1;
};

VRTexturePtr VRTextureGenerator::readSharedMemory(string segment, string object) {
    VRSharedMemory sm(segment, false);

    // add texture example
    /*auto s = sm.addObject<Vec3i>(object+"_size");
    *s = Vec3i(2,2,1);
    auto vec = sm.addVector<Vec3d>(object);
    vec->push_back(Vec3d(1,0,1));
    vec->push_back(Vec3d(1,1,0));
    vec->push_back(Vec3d(0,0,1));
    vec->push_back(Vec3d(1,0,0));*/

    // read texture
    auto tparams = sm.getObject<tex_params>(object+"_size");
    auto vs = tparams.dims;
    auto vdata = sm.getVector<float>(object);

    //cout << "read shared texture " << object << " " << vs << "   " << tparams.pixel_format << "  " << tparams.data_type << "  " << vdata.size() << endl;

    img = VRTexture::create();
    img->getImage()->set(tparams.pixel_format, vs[0], vs[1], vs[2], 0, 1, 0.0, (const uint8_t*)&vdata[0], tparams.data_type, true, 1);
    img->setInternalFormat( tparams.internal_pixel_format );
    return img;
}

void VRTextureGenerator::addSimpleNoise(Vec3i dim, bool doAlpha, Color4f fg, Color4f bg, float amount) {
    setSize(dim, doAlpha);
    if (doAlpha) {
        add(PERLIN, amount*1./2, bg, fg);
        add(PERLIN, amount*1./4, bg, fg);
        add(PERLIN, amount*1./8, bg, fg);
        add(PERLIN, amount*1./16, bg, fg);
        add(PERLIN, amount*1./32, bg, fg);
    } else {
        Color3f fg3(fg[0], fg[1], fg[2]);
        Color3f bg3(bg[0], bg[1], bg[2]);
        add(PERLIN, amount*1./2, bg3, fg3);
        add(PERLIN, amount*1./4, bg3, fg3);
        add(PERLIN, amount*1./8, bg3, fg3);
        add(PERLIN, amount*1./16, bg3, fg3);
        add(PERLIN, amount*1./32, bg3, fg3);
    }
}

OSG_END_NAMESPACE;
