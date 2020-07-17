//
//  SkeletonFinder.cpp
//
//  Created by Pierre B�rki on 19.05.20.
//  Adapted from BlobFinder.cpp by maybites (14.02.14).
//

#include "SkeletonFinder.h"


void SkeletonFinder::initGUI(ofxGui& gui) {
	panel = gui.addPanel();

	panel->loadTheme("theme/theme_light.json");
	panel->setName("Tracking");

	sensorBoxLeft.addListener(this, &SkeletonFinder::updateSensorBox);
	sensorBoxRight.addListener(this, &SkeletonFinder::updateSensorBox);
	sensorBoxFront.addListener(this, &SkeletonFinder::updateSensorBox);
	sensorBoxBack.addListener(this, &SkeletonFinder::updateSensorBox);
	sensorBoxTop.addListener(this, &SkeletonFinder::updateSensorBox);
	sensorBoxBottom.addListener(this, &SkeletonFinder::updateSensorBox);

	sensorBoxGuiGroup = panel->addGroup("SensorBox");
	sensorBoxGuiGroup->add(filtering.set("Filtering", false));
	sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxLeft.set("left", 1000));
	sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxRight.set("right", -1000));
	sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxFront.set("front", 1000));
	sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxBack.set("back", -1000));
	sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxTop.set("top", 2000));
	sensorBoxGuiGroup->add<ofxGuiIntInputField>(sensorBoxBottom.set("bottom", 1000));

	panel->loadFromFile("tracking.xml");

	bool visible = false;
	panel->setVisible(visible);
}

void SkeletonFinder::setTransformMatrix(ofMatrix4x4* mat) {
	transformMatrix = mat;
}

void SkeletonFinder::update(nuitrack::SkeletonData::Ptr data) {
	skeletons.clear();
	// TODO: filter using the capture bounds
	for (nuitrack::Skeleton skel : data->getSkeletons()) {
		vector<Joint> joints;
		for (nuitrack::Joint joint : skel.joints) {
			glm::vec3 pos = ofxnui::Tracker::fromVector3(joint.real);

			// ofMatrix multiplication works in reverse
			pos = (ofVec3f)pos * *transformMatrix;

			joints.push_back(Joint(joint.type, joint.confidence, pos));
		}

		Skeleton skeleton(skel.id, joints);
		if (!filtering.get() || isSkeletonInBounds(skeleton)) {
			skeletons.push_back(skeleton);
		}

	}
}

vector<Skeleton> SkeletonFinder::getSkeletons() const {
	return skeletons;
}

void SkeletonFinder::drawSensorBox()
{
	sensorBox.draw();
}

// adapted from ofxNuitrack example
void SkeletonFinder::drawSkeletons() {
	static vector<Bone> bones = {
		Bone(nuitrack::JOINT_WAIST, nuitrack::JOINT_TORSO, glm::vec3(0, 1, 0)),
		Bone(nuitrack::JOINT_TORSO, nuitrack::JOINT_NECK, glm::vec3(0, 1, 0)),
		Bone(nuitrack::JOINT_NECK, nuitrack::JOINT_HEAD, glm::vec3(0, 1, 0)),

		Bone(nuitrack::JOINT_LEFT_COLLAR, nuitrack::JOINT_LEFT_SHOULDER, glm::vec3(1, 0, 0)),
		Bone(nuitrack::JOINT_LEFT_SHOULDER, nuitrack::JOINT_LEFT_ELBOW, glm::vec3(1, 0, 0)),
		Bone(nuitrack::JOINT_LEFT_ELBOW, nuitrack::JOINT_LEFT_WRIST, glm::vec3(1, 0, 0)),
		Bone(nuitrack::JOINT_LEFT_WRIST, nuitrack::JOINT_LEFT_HAND, glm::vec3(1, 0, 0)),

		Bone(nuitrack::JOINT_WAIST, nuitrack::JOINT_LEFT_HIP, glm::vec3(1, 0, 0)),
		Bone(nuitrack::JOINT_LEFT_HIP, nuitrack::JOINT_LEFT_KNEE, glm::vec3(0, -1, 0)),
		Bone(nuitrack::JOINT_LEFT_KNEE, nuitrack::JOINT_LEFT_ANKLE, glm::vec3(0, -1, 0)),

		Bone(nuitrack::JOINT_RIGHT_COLLAR, nuitrack::JOINT_RIGHT_SHOULDER, glm::vec3(-1, 0, 0)),
		Bone(nuitrack::JOINT_RIGHT_SHOULDER, nuitrack::JOINT_RIGHT_ELBOW, glm::vec3(-1, 0, 0)),
		Bone(nuitrack::JOINT_RIGHT_ELBOW, nuitrack::JOINT_RIGHT_WRIST, glm::vec3(-1, 0, 0)),
		Bone(nuitrack::JOINT_RIGHT_WRIST, nuitrack::JOINT_RIGHT_HAND, glm::vec3(-1, 0, 0)),

		Bone(nuitrack::JOINT_WAIST, nuitrack::JOINT_RIGHT_HIP, glm::vec3(-1, 0, 0)),
		Bone(nuitrack::JOINT_RIGHT_HIP, nuitrack::JOINT_RIGHT_KNEE, glm::vec3(0, -1, 0)),
		Bone(nuitrack::JOINT_RIGHT_KNEE, nuitrack::JOINT_RIGHT_ANKLE, glm::vec3(0, -1, 0)),
	};

	ofSetColor(0, 255, 0);
	for (Skeleton skel : skeletons) {
		for (Bone bone : bones) {
			auto j1 = skel.joints[bone.from];
			auto j2 = skel.joints[bone.to];

			if (j1.confidence < 0.15 || j2.confidence < 0.15) {
				continue;
			}

			ofDrawLine(j1.pos, j2.pos);
		}
	}
}

string SkeletonFinder::getShortDesc()
{
	if (skeletons.size() == 0) {
		return "No skeleton found";
	} else {
		ostringstream s;
		Skeleton skel = skeletons[0];
		auto pos = skel.joints[nuitrack::JOINT_HEAD].pos;
		s << "Head position : (" <<
			pos.x << ", " <<
			pos.y << ", " <<
			pos.z << ")";
		return s.str();
	}
}

void SkeletonFinder::updateSensorBox(int& value) {
	sensorBox.clear();
	sensorBox.setMode(OF_PRIMITIVE_LINES);

	sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxBottom.get() * SCALE));
	sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxBottom.get() * SCALE));

	sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxBottom.get() * SCALE));
	sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxBottom.get() * SCALE));

	sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxBottom.get() * SCALE));
	sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxBottom.get() * SCALE));

	sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxBottom.get() * SCALE));
	sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxBottom.get() * SCALE));

	sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxTop.get() * SCALE));
	sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxTop.get() * SCALE));

	sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxTop.get() * SCALE));
	sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxTop.get() * SCALE));

	sensorBox.addVertex(ofPoint(sensorBoxRight.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxTop.get() * SCALE));
	sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxTop.get() * SCALE));

	sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxBack.get() * SCALE, sensorBoxTop.get() * SCALE));
	sensorBox.addVertex(ofPoint(sensorBoxLeft.get() * SCALE, sensorBoxFront.get() * SCALE, sensorBoxTop.get() * SCALE));

	//captureCam.setPosition((sensorBoxLeft.get() * SCALE + sensorBoxRight.get() * SCALE)/2, (sensorBoxBack.get() * SCALE + sensorBoxBack.get() * SCALE)/2, sensorBoxTop.get() * SCALE);
	//captureCam.setPosition(5, 5, 0);
	//captureCam.
}

bool SkeletonFinder::isSkeletonInBounds(const Skeleton& skel) {
	glm::vec3 headPos = skel.joints[nuitrack::JOINT_HEAD].pos;
	return headPos.x < sensorBoxLeft.get() * SCALE
		&& headPos.x > sensorBoxRight.get() * SCALE
		&& headPos.y < sensorBoxFront.get()* SCALE
		&& headPos.y > sensorBoxBack.get() * SCALE
		&& headPos.z < sensorBoxTop.get()* SCALE
		&& headPos.z > sensorBoxBottom.get() * SCALE;
}
