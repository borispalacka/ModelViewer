#pragma once
#include "ModelViewer.h"

class Camera {
public:
	Vertex position;
	Camera() {};
	Camera(Vertex pos) :position(pos) {};
};

class ProjectionPlane {
public:
	double azimut = 0;
	double zenit = 0;
	QVector<Vertex> normalVector = QVector<Vertex>(3,Vertex(0,0,0));
	ProjectionPlane() {};
	ProjectionPlane(double az, double zen, Vertex n[3]) :azimut(az), zenit(az) {
		normalVector = QVector<Vertex>(3);
		for (int i = 0; i < normalVector.length(); i++) {
			normalVector[i] = n[i];
		}
	}
};