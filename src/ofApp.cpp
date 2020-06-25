#include "ofApp.h"

#define RECONNECT_TIME 400

#define DEPTH_X_RES 640
#define DEPTH_Y_RES 480

ofMatrix4x4 ofApp::makeNuitrackToRealSenseTransform() {
	// Thankfully this matrix is symmetric, so we need not worry about the row-major-ness
	// of the matrix object
	float mat[16] = {
		1e-3,  0,    0,    0,
		0,    -1e-3, 0,    0,
		0,     0,    1e-3, 0,
		0,     0,    0,    1
	};
	// TODO: add scaling
	return glm::make_mat4x4(mat);
}

const ofMatrix4x4 ofApp::nuitrackViewportToRealSenseViewportTransform = makeNuitrackToRealSenseTransform();

//--------------------------------------------------------------
void ofApp::initNuitrack() {
	tracker = ofxnui::Tracker::create();
	tracker->init("");

	// depth feed settings
	tracker->setConfigValue("Realsense2Module.Depth.FPS", "30");
	tracker->setConfigValue("Realsense2Module.Depth.RawWidth", "1280");
	tracker->setConfigValue("Realsense2Module.Depth.RawHeight", "720");
	tracker->setConfigValue("Realsense2Module.Depth.ProcessWidth", "1280");
	tracker->setConfigValue("Realsense2Module.Depth.ProcessHeight", "720");
	tracker->setConfigValue("Realsense2Module.Depth.ProcessMaxDepth", "7000");

	// rgb feed settings
	tracker->setConfigValue("Realsense2Module.RGB.FPS", "30");
	tracker->setConfigValue("Realsense2Module.RGB.RawWidth", "1280");
	tracker->setConfigValue("Realsense2Module.RGB.RawHeight", "720");
	tracker->setConfigValue("Realsense2Module.RGB.ProcessWidth", "1280");
	tracker->setConfigValue("Realsense2Module.RGB.ProcessHeight", "720");

	// feeds alignement
	tracker->setConfigValue("DepthProvider.Depth2ColorRegistration", "true");

	// post processing settings
	tracker->setConfigValue("Realsense2Module.Depth.PostProcessing.SpatialFilter.spatial_alpha", "0.1");
	tracker->setConfigValue("Realsense2Module.Depth.PostProcessing.SpatialFilter.spatial_delta", "50");

	// distance settings
	tracker->setConfigValue("Segmentation.MAX_DISTANCE", "7000");
	tracker->setConfigValue("Skeletonization.MaxDistance", "7000");

	tracker->setIssuesCallback([this](nuitrack::IssuesData::Ptr data) {
		auto issue = data->getIssue<nuitrack::Issue>();
		if (issue) {
			ofLogNotice() << "Issue detected! " << issue->getName() << " [" << issue->getId() << "]";
		}
		});
	tracker->setRGBCallback([this](nuitrack::RGBFrame::Ptr data) {
		pointCloudManager.updateRGB(data);
		});
	tracker->setDepthCallback([this](nuitrack::DepthFrame::Ptr data) {
		pointCloudManager.updateDepth(data);
		});
	tracker->setSkeletonCallback([this](nuitrack::SkeletonData::Ptr data) {
		skeletonFinder.update(data);
		});

	pointCloudManager.setDepthSensor(tracker->depthTracker);
}

void ofApp::initViewports() {
	float xOffset = VIEWGRID_WIDTH; //ofGetWidth() / 3;
	float yOffset = VIEWPORT_HEIGHT / N_CAMERAS;

	viewMain.x = xOffset;
	viewMain.y = 0;
	viewMain.width = ofGetWidth() - xOffset - MENU_WIDTH; //xOffset * 2;
	viewMain.height = VIEWPORT_HEIGHT;

	for (int i = 0; i < N_CAMERAS; i++) {

		viewGrid[i].x = 0;
		viewGrid[i].y = yOffset * i;
		viewGrid[i].width = xOffset;
		viewGrid[i].height = yOffset;
	}

	iMainCamera = 0;

	previewCam.setUpAxis(glm::vec3(0, 0, 1));
	previewCam.setTranslationSensitivity(2., 2., 2.);
	previewCam.setNearClip(0.001f);
}

void ofApp::setupTransformGui() {
	guitransform = gui.addPanel();
	guitransform->loadTheme("theme/theme_light.json");
	guitransform->setName("Transformation");

	transformationGuiGroup.setName("Matrix");
	transformationGuiGroup.add(transformation.set("Transform", ofMatrix4x4()));
	guitransform->addGroup(transformationGuiGroup);

	// The GUI apparently cannot display matrices
	// Also the method apparently requires a reference for some reason
	bool visible = false;
	guitransform->setVisible(visible);

	reloadTransformMatrix();
}

void ofApp::reloadTransformMatrix() {
	guitransform->loadFromFile("transformation.xml");

	// ofMatrices multiplication works in reverse
	worldToDeviceTransform = nuitrackViewportToRealSenseViewportTransform * transformation.get();
	deviceToWorldTransform = nuitrackViewportToRealSenseViewportTransform * transformation.get();
}

void ofApp::setup(){
	ofLog(OF_LOG_NOTICE) << "Nuitrack setup started";
	initNuitrack();
	skeletonFinder.setTransformMatrix(&deviceToWorldTransform);

	ofLog(OF_LOG_NOTICE) << "Viewport setup started";
	initViewports();

	ofLog(OF_LOG_NOTICE) << "Loading GUI";
	setupTransformGui();
	skeletonFinder.initGUI(gui);

	ofLog(OF_LOG_NOTICE) << "MainAPP: starting attached Device...";
	tracker->run();

	bPreviewPointCloud = false;
    
	ofLog(OF_LOG_NOTICE) << "MainAPP: setting up networking...";
	networkMng.setup(gui, "TODO: remove useless arg");

    setupViewports();
    createHelp();

	tracker->run();
    
    capMesh.reSize(4);
    
	ofLog(OF_LOG_NOTICE) << "Setup over";
}

//--------------------------------------------------------------
void ofApp::setupViewports(){
	//call here whenever we resize the window
	// TODO: update UI
	skeletonFinder.panel->setWidth(MENU_WIDTH / 2);
	networkMng.panel->setWidth(MENU_WIDTH / 2);

	skeletonFinder.panel->setPosition(ofGetWidth() - MENU_WIDTH, 20);
	networkMng.panel->setPosition(ofGetWidth() - MENU_WIDTH / 2, 20);
}

//--------------------------------------------------------------
void ofApp::update(){
	tracker->poll();
	ofBackground(100, 100, 100);

	networkMng.update(skeletonFinder);
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofSetColor(255, 255, 255);

    if(bShowVisuals){
        // Draw viewport previews
		pointCloudManager.drawRGB(viewGrid[0]);
		pointCloudManager.drawDepth(viewGrid[1]);
		if (iMainCamera != 2) { // make sure the camera is drawn only once (so the interaction with the mouse works)
			previewCam.begin(viewGrid[2]);
			mainGrid.drawPlane(5., 5, false);
			drawPreview();
			previewCam.end();
		}
        
        switch (iMainCamera) {
            case 0:
				pointCloudManager.drawRGB(viewMain);
                break;
            case 1:
				pointCloudManager.drawDepth(viewMain);
                break;
            case 2:
                previewCam.begin(viewMain);
                mainGrid.drawPlane(5., 5, false);
                drawPreview();
                previewCam.end();
                break;
            default:
                break;
        }

        glDisable(GL_DEPTH_TEST);
        ofPushStyle();

        // Highlight background of selected camera
        ofSetColor(255, 0, 255, 255);
        ofNoFill();
        ofSetLineWidth(3);
        ofDrawRectangle(viewGrid[iMainCamera]);
    }    

	// draw instructions
	ofSetColor(255, 255, 255);
    
    if(bShowHelp) {
        ofDrawBitmapString(help, 20 ,VIEWPORT_HEIGHT + 50);
    }

	if (bShowSkeletonData) {
		ofDrawBitmapString(skeletonFinder.getShortDesc(),
			20,
			VIEWPORT_HEIGHT + 20);
	}

    ofDrawBitmapString("fps: " + ofToString(ofGetFrameRate()), ofGetWidth() - 200, 10);

    ofPopStyle();
}

void ofApp::drawPreview() {
	glPointSize(4);
	glEnable(GL_DEPTH_TEST);

    //This moves the crossingpoint of the kinect center line and the plane to the center of the stage
    //ofTranslate(-planeCenterPoint.x, -planeCenterPoint.y, 0);
	if (bPreviewPointCloud) {
		ofPushMatrix();
		ofMultMatrix(worldToDeviceTransform);
		pointCloudManager.drawPointCloud();
		ofPopMatrix();
	}

	// TODO: draw base
	//ofFill();
	//ofSetColor(255, 0, 0);
	//sphere_X.draw();
	//sphere_Y.draw();
	//sphere_Z.draw();
	
	ofPushStyle();
    ofSetColor(255, 255, 0);
    skeletonFinder.drawSensorBox();
	
	glLineWidth(5);
    ofSetColor(255, 100, 255);
	skeletonFinder.drawSkeletons();
	ofPopStyle();

	glDisable(GL_DEPTH_TEST);  
}

//--------------------------------------------------------------
void ofApp::exit() {
    ofLog(OF_LOG_NOTICE) << "exiting application...";
	
	// Nuitrack auto-releases on destroy ...
}

void ofApp::createHelp(){
    help = string("press v -> to show visualizations\n");
	help += "press 1 - 3 -> to change the viewport\n";
	help += "press p -> to show pointcloud\n";
    help += "press h -> to show help \n";
    help += "press s -> to save current settings.\n";
	help += "press l -> to load last saved settings\n";
	help += "press r -> to show calculation results \n";
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
		case ' ':
			break;
			
		case'p':
			bPreviewPointCloud = !bPreviewPointCloud;
            break;
            
		case'v':
			bShowVisuals = !bShowVisuals;
            break;
            
        case 'r':
            bShowSkeletonData = !bShowSkeletonData;
            break;
 
        case 's':
            skeletonFinder.panel->saveToFile("trackings.xml");
			networkMng.panel->saveToFile("broadcast.xml");
			guitransform->saveToFile("transformation.xml");
			break;

        case 'l':
            skeletonFinder.panel->loadFromFile("trackings.xml");
            networkMng.panel->loadFromFile("broadcast.xml");
			guitransform->loadFromFile("transformation.xml");
			break;
           
		case 'h':
			bShowHelp = !bShowHelp;
            if (bShowHelp) {
                createHelp();
            }
			break;
            
		case '1':
            iMainCamera = 0;
			break;
			
		case '2':
            iMainCamera = 1;
			break;
			
		case '3':
            iMainCamera = 2;
			break;						            
	}

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){


}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	stroke.push_back(ofPoint(x,y));
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
	setupViewports();
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

