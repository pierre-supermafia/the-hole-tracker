//
//  BlobTracker.h
//  kinectTCPServer
//
//  Created by maybites on 14.02.14.
//
//
#pragma once

#include "ofMain.h"
#include "ofVec3f.h"
#include "ofxGuiExtended.h"
#include "ofConstants.h"
#include "Planef.h"
#include "Linef.h"
#include "OrthoCamera.h"

#include "ofxNuitrack.h"
#include "nuitrack/Nuitrack.h"

#include <cmath>

#define N_MAX_BLOBS 30
#define SCALE 0.001

using namespace tdv;

struct Joint {
    nuitrack::JointType type;
    float confidence;
    glm::vec3 pos;

    Joint(nuitrack::JointType type, float confidence, glm::vec3 pos) :
        type(type), confidence(confidence), pos(pos) {}
};

struct Skeleton {
    int id;
    vector<Joint> joints;

    bool isValid() const;

    Skeleton(int id, vector<Joint> joints) :
        id(id), joints(joints) {}
};

struct Bone {
    Bone(nuitrack::JointType from, nuitrack::JointType to, glm::vec3 _direction) :
        from(from),
        to(to),
        direction(direction) {}

    nuitrack::JointType from;
    nuitrack::JointType to;
    glm::vec3 direction;
};


class SkeletonFinder {
    
public:
    SkeletonFinder()
        : skeleton(Skeleton(-1, vector<Joint>()))
        {}

    void initGUI(ofxGui& gui);
    void setTransformMatrix(ofMatrix4x4* mat);
    void update(nuitrack::SkeletonData::Ptr data);
    
    void drawSensorBox();
    void drawSkeletons();

    bool getSkeletonHead(glm::vec3& position) const;
    
private:
    void updateSensorBox(int & value);
    bool isSkeletonInBounds(const Skeleton& skel);

    float currentDistanceFactor = 1.0f;
    float maxDistanceFactor = 5e4;
    float distanceGrowth = 1.01f;

    ofxnui::TrackerRef tracker;
    Skeleton skeleton;

    ofMatrix4x4* transformMatrix;

public:
    ofxGuiPanel *panel;
	ofxGuiGroup *sensorBoxGuiGroup;

    ofParameter<bool> filtering;
    ofParameter<int> sensorBoxLeft;
    ofParameter<int> sensorBoxRight;
    ofParameter<int> sensorBoxTop;
    ofParameter<int> sensorBoxBottom;
    ofParameter<int> sensorBoxFront;
    ofParameter<int> sensorBoxBack;

    ofVboMesh sensorBox;
};


