#pragma once

#include "ViewerWidget.h"

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
	Vertex normalVector = Vertex(0, 0, 0);
	//Basis vectors for perspective Coordinate System
	Vertex basisVectorN = Vertex(0, 0, 0);
	Vertex basisVectorU = Vertex(0, 0, 0);
	Vertex basisVectorV = Vertex(0, 0, 0);
	ProjectionPlane() {};
	ProjectionPlane(double az, double zen, Vertex vN, Vertex vU, Vertex vV, Vertex vectorNormal) :
		azimut(az), zenit(az), basisVectorN(vN), basisVectorU(vU), basisVectorV(vV), normalVector(vectorNormal) {};
};