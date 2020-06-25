#include "PointCloudManager.h"

void PointCloudManager::drawPointCloud()
{
	ofNoFill();
	pointCloud.draw();
}

void PointCloudManager::drawRGB(const ofRectangle& viewRect) {
	rgbTex.draw(viewRect);
}

void PointCloudManager::drawDepth(const ofRectangle& viewRect) {
	depthTex.draw(viewRect);
}

void PointCloudManager::updateRGB(RGBFrame::Ptr data)
{
	rgbFrameSize.x = data->getCols();
	rgbFrameSize.y = data->getRows();

	const Color3* rgbData = data->getData();
	uint8_t* colorDataPtr = (uint8_t*)rgbData;
	ofPixels rgbPix;
	// Assuming the data comes from the live feed (hence BGR)
	rgbPix.setFromPixels(colorDataPtr, rgbFrameSize.x, rgbFrameSize.y, OF_PIXELS_BGR);
	rgbTex.loadData(rgbPix);

	createPointCloudIfNotExist(depthFrameSize);

	for (int y = 0; y < depthFrameSize.y; y += skip) {
		for (int x = 0; x < depthFrameSize.x; x += skip) {
			const int index = y * depthFrameSize.x + x;
			const int skippedIndex = (y / skip) * (depthFrameSize.x / skip) + (x / skip);

			pointCloud.setColor(skippedIndex, rgbPix.getColor(x, y));
		}
	}
}

void PointCloudManager::updateDepth(DepthFrame::Ptr data)
{
	depthFrameSize.x = data->getCols();
	depthFrameSize.y = data->getRows();

	const unsigned short* depthData = data->getData();
	ofShortPixels pix;
	pix.setFromPixels(depthData, depthFrameSize.x, depthFrameSize.y, OF_PIXELS_GRAY);
	depthTex.loadData(pix);

	createPointCloudIfNotExist(depthFrameSize);
	
	for (int y = 0; y < depthFrameSize.y; y += skip) {
		for (int x = 0; x < depthFrameSize.x; x += skip) {
			const int index = y * depthFrameSize.x + x;
			const int skippedIndex = (y / skip) * (depthFrameSize.x / skip) + (x / skip);

			const unsigned short d = depthData[index];
			Vector3 v = depthSensor->convertProjToRealCoords(x, y, depthData[index]);
			pointCloud.setVertex(skippedIndex, ofxnui::Tracker::fromVector3(v));
		}
	}
}

void PointCloudManager::createPointCloudIfNotExist(glm::vec2 dim)
{
	if (pointCloud.getVertices().size() == 0) {
		pointCloud.setMode(OF_PRIMITIVE_POINTS);
		pointCloud.enableColors();

		// Allocate
		pointCloud.clear();
		int size = ceil(((float)dim.x / skip) * ((float)dim.y / skip));

		vector<glm::vec3> p;
		p.assign(size, glm::vec3(0, 0, 0));
		pointCloud.addVertices(p);

		vector<ofFloatColor> c;
		c.assign(size, ofFloatColor(0, 0, 0, 1));
		pointCloud.addColors(c);
	}
}

void PointCloudManager::setDepthSensor(DepthSensor::Ptr sensor) {
	depthSensor = sensor;
}