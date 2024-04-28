#pragma once

#include <QtWidgets>
#include <QVector>
#include <algorithm>
#include <qelapsedtimer.h>
#include <QPoint>
#include <QString>
#include <iostream>
#include <cmath>
#include <QMap>

//-------------Need to place this in different header---------

class H_edge;

class Vertex {
public:
	double x = 0, y = 0, z = 0;
	H_edge* edge = nullptr;

	Vertex() {}
	Vertex(double x, double y, double z) : x(x), y(y), z(z), edge(nullptr) {};

	QString toString() {
		return "Vertex({" + QString::number(x) + ", " + QString::number(y) + ", " + QString::number(z) + "})";
	}
	QPoint toQPointXY() { return QPoint(static_cast<int> (x), static_cast<int> (y)); }

	bool operator==(const Vertex& ver) const {
		return x == ver.x && y == ver.y && z == ver.z;
	}
	double operator*(const Vertex& ver)const {
		return ver.x * x + ver.y * y + ver.z * z;
	}
	Vertex operator+(const Vertex& ver) const {
		return Vertex(ver.x + x, ver.y + y, ver.z + z);
	}
	Vertex operator-(const Vertex& ver) const {
		return Vertex(ver.x - x, ver.y - y, ver.z - z);
	}
	friend QDebug operator<<(QDebug dbg, const Vertex& vertex) {
		dbg.nospace() << "Vertex(" << vertex.x << ", " << vertex.y << ", " << vertex.z << " )";
		return dbg.space();
	}
};

class Face {
public:
	H_edge* edge;

	Face(H_edge* edge) : edge(edge) {}
};

class H_edge {
public:
	Vertex* vert_origin;
	Face* face;
	H_edge* edge_prev, * edge_next;
	H_edge* pair;

	H_edge() {
		vert_origin = nullptr;
		face = nullptr;
		edge_prev = nullptr;
		edge_next = nullptr;
		pair = nullptr;
	}
	H_edge(Vertex* origin, Face* fc, H_edge* ePrevious, H_edge* eNext, H_edge* pair) : vert_origin(origin), face(fc),
		edge_prev(ePrevious), edge_next(eNext), pair(pair) {}
	void setEdge(Vertex* origin, Face* fc, H_edge* ePrevious, H_edge* eNext, H_edge* pair) {
		vert_origin = origin;
		face = fc;
		edge_prev = ePrevious;
		edge_next = eNext;
		this->pair = pair;
	}

	bool operator==(const H_edge& e) const {
		return e.edge_next == edge_next && e.edge_prev == edge_prev && e.face == face && e.vert_origin == vert_origin && e.pair == pair;
	}
	bool operator!=(const H_edge& e) const {
		return e.edge_next != edge_next || e.edge_prev != edge_prev || e.face != face || e.vert_origin != vert_origin || e.pair != pair;
	}
};

class Object_H_edge {
public:
	QVector<Vertex*> vertices;
	QVector<H_edge*> edges;
	QVector<Face*> faces;
	QHash<Face*, QColor> colors;

	Object_H_edge() {};
	Object_H_edge(QVector<Vertex*> vert, QVector<H_edge*> edg, QVector<Face*> fcs) : vertices(vert), edges(edg), faces(fcs) {};

};

void createCubeVTK(double d, QString filename);

Object_H_edge loadPolygonsVTK(QString filename);

void savePolygonsVTK(QString filename, Object_H_edge object);

void createCubeVTK(QVector<Vertex> vertices, QString filename);

void rotateCubeAnimation(double d, int frames);

void createUvSphereVTK(double r, int longitude, int latitude, QString filename);

//-------------------------------------------------------------

//--------Need separated header for this classes---------------
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
	//Basis vectors for projection Coordinate System
	Vertex basisVectorN = Vertex(0, 0, 0);
	Vertex basisVectorU = Vertex(0, 0, 0);
	Vertex basisVectorV = Vertex(0, 0, 0);
	ProjectionPlane() {};
	ProjectionPlane(double az, double zen, Vertex vectorNormal) :
		azimut(az), zenit(zen), normalVector(vectorNormal) {
		basisVectorN = Vertex(sin(zenit) * sin(azimut), sin(zenit) * cos(azimut), cos(zenit));
		basisVectorU = Vertex(sin(zenit + M_PI / 2) * sin(azimut), sin(zenit + M_PI / 2) * cos(azimut), cos(zenit + M_PI / 2));
		basisVectorV.x = basisVectorN.y * basisVectorU.z - basisVectorN.z * basisVectorU.y;
		basisVectorV.y = basisVectorN.z * basisVectorU.x - basisVectorN.x * basisVectorU.z;
		basisVectorV.z = basisVectorN.x * basisVectorU.y - basisVectorN.y * basisVectorU.x;
	};
	void setProjectionPlane(double az, double zen) {
		azimut = az;
		zenit = zen;
		basisVectorN = Vertex(sin(zenit) * sin(azimut), sin(zenit) * cos(azimut), cos(zenit));
		basisVectorU = Vertex(sin(zenit + M_PI / 2) * sin(azimut), sin(zenit + M_PI / 2) * cos(azimut), cos(zenit + M_PI / 2));
		basisVectorV.x = basisVectorN.y * basisVectorU.z - basisVectorN.z * basisVectorU.y;
		basisVectorV.y = basisVectorN.z * basisVectorU.x - basisVectorN.x * basisVectorU.z;
		basisVectorV.z = basisVectorN.x * basisVectorU.y - basisVectorN.y * basisVectorU.x;
	}
};
//-------------------------------------------------------------

class ViewerWidget :public QWidget {
	Q_OBJECT
private:
	QSize areaSize = QSize(0, 0);
	QImage* img = nullptr;
	QPainter* painter = nullptr;
	uchar* data = nullptr;

	Camera camera = Camera(Vertex(0,0,0));
	ProjectionPlane projectionPlane = ProjectionPlane(0,0,Vertex(0,0,0));

	//Hash tables for Z-buffer algorithm
	QHash<QPair<int, int>, QColor> mapOfColors;
	QHash<QPair<int, int>, double> mapOfZCoords;

	bool drawLineActivated = false;
	QPoint drawLineBegin = QPoint(0, 0);
	QPoint drawLineEnd = QPoint(0, 0);

	QElapsedTimer timer;

	bool drawPolygonActivated = false;
	QVector<QPoint> drawPolygonPoints = QVector<QPoint>();

	bool drawCurveActivated = false;
	QVector<QPair<QPoint,QPoint>> drawCurveMasterPoints = QVector<QPair<QPoint,QPoint>>();

	bool croppedBySutherlandHodgman = false;

	bool drawObjectActivated = false;
	Object_H_edge currentObject = Object_H_edge();


	//Image Editing variables
	bool dragReady = false;
	QPoint dragStartingPosition = QPoint();
	QPoint dragedPoint = QPoint();

public:
	ViewerWidget(QSize imgSize, QWidget* parent = Q_NULLPTR);
	~ViewerWidget();
	void resizeWidget(QSize size);

	//Get/Set functions
	uchar* getData() { return data; }
	void setDataPtr() { data = img->bits(); }
	void setPainter() { painter = new QPainter(img); }
	int getImgWidth() { return img->width(); };
	int getImgHeight() { return img->height(); };
	//CAMERA & PROJECTION PLANE
	ProjectionPlane& getProjectionPlane() { return projectionPlane; }
	Camera& getCamera() { return camera; }
	//LINE DRAW
	void setDrawLineBegin(QPoint begin) { drawLineBegin = begin; }
	QPoint getDrawLineBegin() { return drawLineBegin; }
	void setDrawLineEnd(QPoint end) { drawLineEnd = end; }
	QPoint getDrawLineEnd() { return drawLineEnd; }
	void setDrawLineActivated(bool state) { drawLineActivated = state; }
	bool getDrawLineActivated() { return drawLineActivated; }
	//POLYGON DRAW
	void setDrawPolygonActivated(bool state) { drawPolygonActivated = state; }
	bool getDrawPolygonActivated() { return drawPolygonActivated; }
	QVector<QPoint>& getDrawPolygonPoints() { return drawPolygonPoints; }
	void setDrawPolygonPoints(QVector<QPoint> points) { drawPolygonPoints = points; }
	//CURVE DRAW
	void setDrawCurveActivated(bool state) { drawCurveActivated = state; }
	bool getDrawCurveActivated() { return drawCurveActivated; }
	void setDrawCurveMasterPoints(QVector<QPair<QPoint, QPoint>> points) { drawCurveMasterPoints = points; }
	QVector<QPair<QPoint, QPoint>>& getDrawCurveMasterPoints() { return drawCurveMasterPoints; }
	//3D OBJECT DRAW
	void setDrawObjectActivated(bool state) { drawObjectActivated = state; }
	bool getDrawObjectActivated() { return drawObjectActivated; }
	void setCurrentObject(Object_H_edge object) { currentObject = object; }
	Object_H_edge getCurrentObject() { return currentObject; }


	//Image functions
	bool setImage(const QImage& inputImg);
	QImage* getImage() { return img; };
	bool isEmpty();
	bool changeSize(int width, int height);

	void setPixel(int x, int y, uchar r, uchar g, uchar b, uchar a = 255);
	void setPixel(int x, int y, double valR, double valG, double valB, double valA = 1.);
	void setPixel(int x, int y, const QColor& color);
	bool isInside(int x, int y) { return (x >= 0 && y >= 0 && x < img->width() && y < img->height()) ? true : false; }

	//Draw functions
	//2D draw functions
	void drawLine(QPoint start, QPoint end, QColor color, int algType = 0);
	void drawLineDDA(QPoint start, QPoint end, QColor color);
	void drawLineBresenham(QPoint start, QPoint end, QColor color);
	void drawCircleBresenham(QPoint start, QPoint end, QColor color);
	void drawPolygon(QVector<QPoint> points, QColor color, int algType = 0, int fillingAlgType = 0);
	void scanLinePolygon(QVector<QPoint> points, QColor color);
	void fillTriangleSetup(QVector<QPoint> points, QColor color, int fillAlgType);
	void fillTriangle(QVector<QPoint> currentPoints,QVector<QPoint> oldPoints ,QColor color, int fillAlgType);
	QColor fillTriangleNearestNeighbour(QVector<QPoint> points,QPoint currentPoint, QVector<QColor> colors);
	QColor fillTriangleBaricentric(QVector<QPoint> points, QPoint currentPoint, QVector<QColor> colors);
	void drawCurve(QVector<QPair<QPoint, QPoint>> points, QColor color, int algType);
	void drawCurveHermint(QVector<QPair<QPoint, QPoint>> points, QColor color);
	void drawCurveCasteljau(QVector<QPoint> points, QColor color);
	void drawCurveCoons(QVector<QPoint> points, QColor color);

	//3D draw functions
	void drawObject(const Object_H_edge& object, Camera camera, ProjectionPlane projectionPlane, int projectionType, int representationType);
	QVector<Vertex> perspectiveCoordSystemTransformation(const Object_H_edge& object, int projectionType);
	double baricentricInterpolation(const QVector<Vertex*> vertices, Vertex* currentVertex);
	void fillObjectPolygonSetup(const QVector<Vertex*> vertices,QColor color, int fillingAlg);
	void fillObjectPolygon(const QVector<Vertex*> vertices, QColor color, int fillingAlg);



	QVector <QPoint> cyrusBeck(QPoint P1, QPoint P2);
	QVector <QPoint> sutherlandHodgman(QVector<QPoint> V);

	//Image Editing variables
	bool getDragReady() { return dragReady; }
	void setDragReady(bool state) { dragReady = state; }
	void setDragStartingPosition(QPoint start) { dragStartingPosition = start; }
	QPoint getDragStartingPosition() { return dragStartingPosition; }
	void setDragedPoint(QPoint point) { dragedPoint = point; }
	QPoint getDragedPoint() { return dragedPoint; }

	void clear();

public slots:
	void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;
};
