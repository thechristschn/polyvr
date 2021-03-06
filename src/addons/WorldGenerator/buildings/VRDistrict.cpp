#include "VRDistrict.h"

#include "VRBuilding.h"
#include "addons/WorldGenerator/GIS/OSMMap.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "core/objects/material/VRShader.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/toString.h"
#include "core/math/triangulator.h"
#include "core/scene/VRSceneManager.h"

#define GLSL(shader) #shader

using namespace OSG;

VRDistrict::VRDistrict() : VRObject("District") {}
VRDistrict::~VRDistrict() {}

VRDistrictPtr VRDistrict::create() {
    auto d = VRDistrictPtr( new VRDistrict() );
    d->init();
    return d;
}

void VRDistrict::init() {
    if (!b_mat) {
        b_mat = VRMaterial::create("Buildings");
        b_mat->setTexture("world/textures/Buildings.png", false);
        b_mat->setAmbient(Color3f(0.7, 0.7, 0.7)); //light reflection in all directions
        b_mat->setDiffuse(Color3f(1.0, 1.0, 1.0)); //light from ambient (without lightsource)
        b_mat->setSpecular(Color3f(0.2, 0.2, 0.2)); //light reflection in camera direction
        b_mat->setVertexShader(matVShdr, "buildingVS");
        b_mat->setFragmentShader(matFShdr, "buildingFS");
        b_mat->setMagMinFilter(GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, 0);
    }

    if (facades) facades->destroy();
    if (roofs) roofs->destroy();
    facades = VRGeometry::create("facades");
    roofs = VRGeometry::create("roofs");
    addChild(facades);
    addChild(roofs);
    facades->setMaterial(b_mat);
    roofs->setMaterial(b_mat);
}

void VRDistrict::addBuilding( VRPolygon p, int stories, string housenumber, string street ) {
    if (p.size() < 3) return;
    if (p.isCCW()) p.reverseOrder();
    auto b = VRBuilding::create();
    string ID = street+housenumber;
    if (ID == "" || buildings.count(ID)) {
        static int i = 0; i++;
        ID = "__placeholder__"+toString(i);
    }
    buildings[ID] = b;
    b->setWorld(world);

    b->addFoundation(p, 4);
    for (auto i=0; i<stories; i++) b->addFloor(p, 4);
    b->addRoof(p);
    b->computeGeometry(facades, roofs);

    auto bEnt = ontology->addEntity("building", "Building");
    bEnt->set("streetName", street);
    bEnt->set("houseNumber", housenumber);

    auto toStringVector = [](const Vec3d& v) {
        vector<string> res;
        res.push_back( toString(v[0]) );
        res.push_back( toString(v[1]) );
        res.push_back( toString(v[2]) );
        return res;
    };

    auto area = ontology->addEntity("area", "Area");
    auto perimeter = ontology->addEntity("perimeter", "Path");
    for (auto pnt : p.get()) {
        auto node = ontology->addEntity("node", "Node");
        node->setVector("position", toStringVector(Vec3d(pnt[0],0,pnt[1])), "Position");
		auto nodeEntry = ontology->addEntity(name+"Entry", "NodeEntry");
		nodeEntry->set("path", perimeter->getName());
		nodeEntry->set("node", node->getName());
		nodeEntry->set("sign", "0");
		nodeEntry->setVector("direction", toStringVector(Vec3d()), "Direction");
		node->add("paths", nodeEntry->getName());
		perimeter->add("nodes", nodeEntry->getName());
    }
    area->set("borders", perimeter->getName());
    bEnt->set("area", area->getName());
}

void VRDistrict::computeGeometry() {
    init();
    for (auto b : buildings) b.second->computeGeometry(facades, roofs);
}

void VRDistrict::remBuilding( string street, string housenumber ) {
    string ID = street+housenumber;
    if (!buildings.count(ID)) cout << "VRDistrict::remBuilding, Warning: building unknown with address '" << street << " " << housenumber << "'\n";
    buildings.erase(ID);
    computeGeometry();
}

void VRDistrict::clear() {
    buildings.clear();
    init();
}

string VRDistrict::matVShdr = GLSL(
varying vec3 vnrm;
varying vec2 vtc1;
varying vec2 vtc2;
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec2 osg_MultiTexCoord0;
attribute vec2 osg_MultiTexCoord1;

void main( void ) {
    vnrm = normalize( gl_NormalMatrix * osg_Normal );
    vtc1 = vec2(osg_MultiTexCoord0);
    vtc2 = vec2(osg_MultiTexCoord1);
    gl_Position = gl_ModelViewProjectionMatrix * osg_Vertex;
}
);


string VRDistrict::matFShdr = GLSL(
varying vec3 vnrm;
varying vec2 vtc1;
varying vec2 vtc2;
uniform sampler2D tex;

vec4 color;
vec3 normal;

void applyLightning() {
	vec3 n = normal;
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(n, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular;
}

void main( void ) {
	normal = vnrm;
	vec4 tex1 = texture2D(tex, vtc1);
	vec4 tex2 = texture2D(tex, vtc2);
	color = mix(tex1, tex2, tex2[3]);
	applyLightning();
}
);
