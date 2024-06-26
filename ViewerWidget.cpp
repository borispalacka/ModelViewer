#include "ViewerWidget.h"
#include <QElapsedTimer>
#include <QFile>
#include <regex>
#include <QHash>
#include <random>

#define VTK_FILE_HEADER "#vtk DataFile Version 3.0\nvtk output\nASCII\nDATASET POLYDATA\n"

ViewerWidget::ViewerWidget(QSize imgSize, QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_StaticContents);
	setMouseTracking(true);
	if (imgSize != QSize(0, 0)) {
		img = new QImage(imgSize, QImage::Format_ARGB32);
		img->fill(Qt::white);
		resizeWidget(img->size());
		setPainter();
		setDataPtr();
	}
	z_buffer_default_color_array = QVector<QVector<QColor>>(img->height(), QVector<QColor>(img->width(), Qt::white));
	z_buffer_default_layer_array = QVector<QVector<double>>(img->height(), QVector<double>(img->width(), -DBL_MAX));
}
ViewerWidget::~ViewerWidget()
{
	delete painter;
	delete img;
}
void ViewerWidget::resizeWidget(QSize size)
{
	this->resize(size);
	this->setMinimumSize(size);
	this->setMaximumSize(size);
}

//Image functions
bool ViewerWidget::setImage(const QImage& inputImg)
{
	if (img != nullptr) {
		delete painter;
		delete img;
	}
	img = new QImage(inputImg);
	if (!img) {
		return false;
	}
	resizeWidget(img->size());
	setPainter();
	setDataPtr();
	update();

	return true;
}
bool ViewerWidget::isEmpty()
{
	if (img == nullptr) {
		return true;
	}

	if (img->size() == QSize(0, 0)) {
		return true;
	}
	return false;
}
bool ViewerWidget::changeSize(int width, int height)
{
	QSize newSize(width, height);

	if (newSize != QSize(0, 0)) {
		if (img != nullptr) {
			delete painter;
			delete img;
		}

		img = new QImage(newSize, QImage::Format_ARGB32);
		if (!img) {
			return false;
		}
		img->fill(Qt::white);
		resizeWidget(img->size());
		setPainter();
		setDataPtr();
		update();
	}

	return true;
}
void ViewerWidget::setPixel(int x, int y, uchar r, uchar g, uchar b, uchar a)
{
	r = r > 255 ? 255 : (r < 0 ? 0 : r);
	g = g > 255 ? 255 : (g < 0 ? 0 : g);
	b = b > 255 ? 255 : (b < 0 ? 0 : b);
	a = a > 255 ? 255 : (a < 0 ? 0 : a);

	size_t startbyte = y * img->bytesPerLine() + x * 4;
	data[startbyte] = b;
	data[startbyte + 1] = g;
	data[startbyte + 2] = r;
	data[startbyte + 3] = a;
}
void ViewerWidget::setPixel(int x, int y, double valR, double valG, double valB, double valA)
{
	valR = valR > 1 ? 1 : (valR < 0 ? 0 : valR);
	valG = valG > 1 ? 1 : (valG < 0 ? 0 : valG);
	valB = valB > 1 ? 1 : (valB < 0 ? 0 : valB);
	valA = valA > 1 ? 1 : (valA < 0 ? 0 : valA);

	size_t startbyte = y * img->bytesPerLine() + x * 4;
	data[startbyte] = static_cast<uchar>(255 * valB);
	data[startbyte + 1] = static_cast<uchar>(255 * valG);
	data[startbyte + 2] = static_cast<uchar>(255 * valR);
	data[startbyte + 3] = static_cast<uchar>(255 * valA);
}
void ViewerWidget::setPixel(int x, int y, const QColor& color)
{
	if (color.isValid()) {
		size_t startbyte = y * img->bytesPerLine() + x * 4;

		data[startbyte] = color.blue();
		data[startbyte + 1] = color.green();
		data[startbyte + 2] = color.red();
		data[startbyte + 3] = color.alpha();
	}
}

//Draw functions
//2D draw functions
void ViewerWidget::drawLine(QPoint start, QPoint end, QColor color, int algType)
{
	if (!croppedBySutherlandHodgman) {
		QVector<QPoint> newPoints = cyrusBeck(start, end);
		if (newPoints.isEmpty()) {
			//qDebug() << "drawing line: none";
			return;
		}
		else {
			start = newPoints[0];
			end = newPoints[1];
			//qDebug() << "drawing line: " << start << end;
		}
	}
	if (algType == 0) { //DDA
		drawLineDDA(start, end, color);
	}
	else if (algType == 1) { //Bresenham
		drawLineBresenham(start, end, color);
	}
	//drawCircleBresenham(start, start + QPoint(0, 2), Qt::red);
	//drawCircleBresenham(end, end + QPoint(0, 2), Qt::red);
	update();
}
void ViewerWidget::drawCircleBresenham(QPoint start, QPoint end, QColor color) {
	int r = static_cast<int>(sqrt(pow(end.x() - start.x(), 2) + pow(end.y() - start.y(), 2)));
	int x = 0, twoX = 3;
	int y = r, twoY = 2 * r - 2;
	int pCurrent = 1 - r;
	for (x = 0; x <= y; x++) {
		if (isInside(y + start.x(), x + start.y())) {
			setPixel(y + start.x(), x + start.y(), color);
			if (z_buffer_in_use) {
				if (z_buffer_current_value > z_buffer_layer_array[x + start.y()][y + start.x()]) {
					z_buffer_color_array[x + start.y()][y + start.x()] = color;
					z_buffer_layer_array[x + start.y()][y + start.x()] = z_buffer_current_value;
				}
				else {
					setPixel(y + start.x(), x + start.y(), z_buffer_color_array[x + start.y()][y + start.x()]);
				}
			}
		}
		if (isInside(x + start.x(), y + start.y())) {
			setPixel(x + start.x(), y + start.y(), color);
			if (z_buffer_in_use) {
				if (z_buffer_current_value > z_buffer_layer_array[y + start.y()][x + start.x()]) {
					z_buffer_color_array[y + start.y()][x + start.x()] = color;
					z_buffer_layer_array[y + start.y()][x + start.x()] = z_buffer_current_value;
				}
				else {
					setPixel(x + start.x(), y + start.y(), z_buffer_color_array[y + start.y()][x + start.x()]);
				}
			}
		}
		if (isInside(x + start.x(), -y + start.y())) {
			setPixel(x + start.x(), -y + start.y(), color);
			if (z_buffer_in_use) {
				if (z_buffer_current_value > z_buffer_layer_array[-y + start.y()][x + start.x()]) {
					z_buffer_color_array[-y + start.y()][x + start.x()] = color;
					z_buffer_layer_array[-y + start.y()][x + start.x()] = z_buffer_current_value;
				}
				else {
					setPixel(x + start.x(), -y + start.y(), z_buffer_color_array[-y + start.y()][x + start.x()]);
				}
			}
		}
		if (isInside(-y + start.x(), x + start.y())) {
			setPixel(-y + start.x(), x + start.y(), color);
			if (z_buffer_in_use) {
				if (z_buffer_current_value > z_buffer_layer_array[x + start.y()][-y + start.x()]) {
					z_buffer_color_array[x + start.y()][-y + start.x()] = color;
					z_buffer_layer_array[x + start.y()][-y + start.x()] = z_buffer_current_value;
				}
				else {
					setPixel(-y + start.x(), x + start.y(), z_buffer_color_array[x + start.y()][-y + start.x()]);
				}
			}

		}
		if (isInside(-y + start.x(), -x + start.y())) {
			setPixel(-y + start.x(), -x + start.y(), color);
			if (z_buffer_in_use) {
				if (z_buffer_current_value > z_buffer_layer_array[-x + start.y()][-y + start.x()]) {
					z_buffer_color_array[-x + start.y()][-y + start.x()] = color;
					z_buffer_layer_array[-x + start.y()][-y + start.x()] = z_buffer_current_value;
				}
				else {
					setPixel(-y + start.x(), -x + start.y(), z_buffer_color_array[-x + start.y()][-y + start.x()]);
				}
			}
		}
		if (isInside(-x + start.x(), -y + start.y())) {
			setPixel(-x + start.x(), -y + start.y(), color);
			if (z_buffer_in_use) {
				if (z_buffer_current_value > z_buffer_layer_array[-y + start.y()][-x + start.x()]) {
					z_buffer_color_array[-y + start.y()][-x + start.x()] = color;
					z_buffer_layer_array[-y + start.y()][-x + start.x()] = z_buffer_current_value;
				}
				else {
					setPixel(-x + start.x(), -y + start.y(), z_buffer_color_array[-y + start.y()][-x + start.x()]);
				}
			}
		}
		if (isInside(-x + start.x(), y + start.y())) {
			setPixel(-x + start.x(), y + start.y(), color);
			if (z_buffer_in_use) {
				if (z_buffer_current_value > z_buffer_layer_array[y + start.y()][-x + start.x()]) {
					z_buffer_color_array[y + start.y()][-x + start.x()] = color;
					z_buffer_layer_array[y + start.y()][-x + start.x()] = z_buffer_current_value;
				}
				else {
					setPixel(-x + start.x(), y + start.y(), z_buffer_color_array[y + start.y()][-x + start.x()]);
				}
			}
		}
		if (isInside(y + start.x(), -x + start.y())) {
			setPixel(y + start.x(), -x + start.y(), color);
			if (z_buffer_in_use) {
				if (z_buffer_current_value > z_buffer_layer_array[-x + start.y()][y + start.x()]) {
					z_buffer_color_array[-x + start.y()][y + start.x()] = color;
					z_buffer_layer_array[-x + start.y()][y + start.x()] = z_buffer_current_value;
				}
				else {
					setPixel(y + start.x(), -x + start.y(), z_buffer_color_array[-x + start.y()][y + start.x()]);
				}
			}
		}
		if (pCurrent > 0) {
			pCurrent = pCurrent - twoY;
			y--;
			twoY -= 2;
		}
		pCurrent += twoX;
		twoX += 2;
	}
	update();
}
void ViewerWidget::drawLineDDA(QPoint start, QPoint end, QColor color) {
	if (start.x() != end.x()) {
		double m = (static_cast<double>(end.y()) - static_cast<double>(start.y())) / (static_cast<double>(end.x()) - static_cast<double>(start.x()));
		if (abs(m) <= 1) { //riadiaca os X
			if (start.x() > end.x()) {
				std::swap(start, end);
			}
			double y = start.y();
			for (int x = start.x(); x < end.x(); x++) {
				if (isInside(x, static_cast<int>(y + 0.5))) {
					if (z_buffer_in_use) {
						if (z_buffer_current_value > z_buffer_layer_array[static_cast<int>(y + 0.5)][x]) {
							z_buffer_layer_array[static_cast<int>(y + 0.5)][x] = z_buffer_current_value;
							z_buffer_color_array[static_cast<int>(y + 0.5)][x] = color;
							setPixel(x, static_cast<int>(y + 0.5), color);
						}
						else {
							setPixel(x, static_cast<int>(y + 0.5), z_buffer_color_array[static_cast<int>(y + 0.5)][x]);
						}
					}
					else {
						setPixel(x, static_cast<int>(y + 0.5), color);
					}
				}
				y += m;
			}
		}
		else { //riadiaca os Y
			if (start.y() > end.y()) {
				std::swap(start, end);
			}
			double x = start.x();
			for (int y = start.y(); y < end.y(); y++) {
				if (isInside(static_cast<int>(x + 0.5), y)) {
					setPixel(static_cast<int>(x + 0.5), y, color);
					if (z_buffer_in_use) {
						if (z_buffer_current_value > z_buffer_layer_array[y][static_cast<int>(x + 0.5)]) {
							z_buffer_layer_array[y][static_cast<int>(x + 0.5)] = z_buffer_current_value;
							z_buffer_color_array[y][static_cast<int>(x + 0.5)] = color;
						}
						else {
							setPixel(static_cast<int>(x + 0.5), y, z_buffer_color_array[y][static_cast<int>(x + 0.5)]);
						}
					}
				}
				x += 1 / m;
			}
		}
	}
	else {
		if (start.y() > end.y()) {
			std::swap(start, end);
		}
		for (int y = start.y(); y < end.y(); y++) {
			if (isInside(start.x(), y)) {
				setPixel(start.x(), y, color);
				if (z_buffer_in_use) {
					if (z_buffer_current_value > z_buffer_layer_array[y][start.x()]) {
						z_buffer_layer_array[y][start.x()] = z_buffer_current_value;
						z_buffer_color_array[y][start.x()] = color;
					}
					else {
						setPixel(start.x(), y, z_buffer_color_array[y][start.x()]);
					}
				}
			}
		}
	}
}
void ViewerWidget::drawLineBresenham(QPoint start, QPoint end, QColor color) {
	if (start.x() == end.x()) {
		if (start.y() > end.y()) {
			std::swap(start, end);
		}
		for (int y = start.y(); y <= end.y(); y++) {
			if (isInside(start.x(), y)) {
				setPixel(start.x(), y, color);
				if (z_buffer_in_use) {
					if (z_buffer_current_value > z_buffer_layer_array[y][start.x()]) {
						z_buffer_layer_array[y][start.x()] = z_buffer_current_value;
						z_buffer_color_array[y][start.x()] = color;
					}
					else {
						setPixel(start.x(), y, z_buffer_color_array[y][start.x()]);
					}
				}
			}
		}
		return;
	}
	double m = (static_cast<double>(end.y()) - static_cast<double>(start.y())) / (static_cast<double>(end.x()) - static_cast<double>(start.x()));
	int tmp = 0;
	if (abs(m) <= 1) {									// riadiaca os X
		if (start.x() > end.x()) {
			std::swap(start, end);
		}
		int twoDeltaX = 2 * (end.x() - start.x());
		int twoDeltaY = 2 * (end.y() - start.y());
		if (0 < m  && m <= 1) {
			tmp = -1;
		}
		else {
			tmp = 1;
		}
		int k1 = twoDeltaY;
		int k2 = twoDeltaY + tmp * twoDeltaX;
		int pCurrent = twoDeltaY + tmp * twoDeltaX / 2;
		int y = start.y();
		if (isInside(start.x(), start.y())) {
			setPixel(start.x(), start.y(), color);
		}
		for (int x = start.x(); x <= end.x(); x++) {

			if (tmp == -1) {							// m patri ]0,1]
				if (pCurrent > 0) {
					y++;
					pCurrent += k2;
				}
				else {
					pCurrent += k1;
				}
			}
			else {										// m patri [-1,0]
				if (pCurrent < 0) {
					y--;
					pCurrent += k2;
				}
				else {
					pCurrent += k1;
				}
			}
			if (isInside(x, y)) {
				setPixel(x, y, color);
				if (z_buffer_in_use) {
					if (z_buffer_current_value > z_buffer_layer_array[y][x]) {
						z_buffer_layer_array[y][x] = z_buffer_current_value;
						z_buffer_color_array[y][x] = color;
					}
					else {
						setPixel(x, y, z_buffer_color_array[y][x]);
					}
				}
			}
		}

	}
	else {												// riadiaca os Y
		if (start.y() > end.y()) {
			std::swap(start, end);
		}
		int twoDeltaX = 2 * (end.x() - start.x());
		int twoDeltaY = 2 * (end.y() - start.y());
		if (m > 1) {
			tmp = -1;
		}
		else {
			tmp = 1;
		}
		int x = start.x();
		int pCurrent = twoDeltaX + tmp * twoDeltaY / 2;
		int k1 = twoDeltaX;
		int k2 = twoDeltaX + tmp * twoDeltaY;
		if (isInside(start.x(), start.y())) {
			setPixel(start.x(), start.y(), color);
		}
		for (int y = start.y(); y <= end.y(); y++) {
			if (tmp == -1) {							// m > 1
				if (pCurrent > 0) {
					x++;
					pCurrent += k2;
				}
				else {
					pCurrent += k1;
				}
			}
			else {										// m < -1 
				if (pCurrent < 0) {
					x--;
					pCurrent += k2;
				}
				else {
					pCurrent += k1;
				}
			}
			if (isInside(x, y)) {
				setPixel(x, y, color);
				if (isInside(x, y)) {
					setPixel(x, y, color);
					if (z_buffer_in_use) {
						if (z_buffer_current_value > z_buffer_layer_array[y][x]) {
							z_buffer_layer_array[y][x] = z_buffer_current_value;
							z_buffer_color_array[y][x] = color;
						}
						else {
							setPixel(x, y, z_buffer_color_array[y][x]);
						}
					}
				}
			}
		}
	}
}
void ViewerWidget::drawPolygon(QVector<QPoint> points, QColor color, int algType, int fillingAlgType) {
	QVector<QPoint> tmpPoints = sutherlandHodgman(points);
	if (tmpPoints.isEmpty() || tmpPoints.length() <= 2) {
		//qDebug() << "drawing polygon : none";
		return;
	}
	points = tmpPoints;
	bool isHorizontalLine = std::all_of(points.begin(), points.end(), [&](const QPoint& point) {
		return point.y() == points[0].y();
		});
	bool isVerticalLine = std::all_of(points.begin(), points.end(), [&](const QPoint& point) {
		return point.x() == points[0].x();
		});
	if (isHorizontalLine || isVerticalLine) {
		//qDebug() << "drawing polygon : none";
		return;
	}
	//qDebug() << "drawing polygon : " << points;
	croppedBySutherlandHodgman = true;
	if (fillingAlgType == 0) {
		for (int i = 0; i < points.length(); i++) {
			if (i < points.length() - 1) {
				drawLine(points[i], points[i + 1], color, algType);
			}
			else {
				drawLine(points[i], points[0], color, algType);
			}
		}
	}
	else if (points.length() == 3) {
		fillTriangleSetup(points, color, fillingAlgType);
	}
	else {
		scanLinePolygon(points, color);
	}
	for (const QPoint& point : points) {
		drawCircleBresenham(point, point + QPoint(0, 1), Qt::red);
	}
	croppedBySutherlandHodgman = false;
}
void ViewerWidget::scanLinePolygon(QVector<QPoint> points, QColor color) {
	struct Edge {											// structura obsahuje informacie o krivke
		QPoint start;
		QPoint end;
		int deltaY = 0;
		double m = 0;
	};
	QVector<Edge> e;
	QPoint start = points.last();
	QPoint end = QPoint();
	for (int i = 0; i < points.length(); i++) {
		end = points[i];
		if (start.y() > end.y()) {							// usporiadanie krivky z hora dole
			std::swap(start, end);
		}
		if (end.y() != start.y()) {							// vynechanie horizontalnych hran
			Edge line;
			line.start = start;
			line.end = QPoint(end.x(), end.y() - 1);			// skratenie hrany o 1 px
			line.deltaY = line.end.y() - line.start.y();
			if (line.end.x() == line.start.x()) {
				line.m = -DBL_MAX;							// prevencia pred delenim nulou
			}
			else {
				line.m = line.deltaY / static_cast<double>(line.end.x() - line.start.x());
			}
			if (line.m != 0) {
				e.append(line);
			}
		}
		start = points[i];
	}
	if (e.isEmpty()) {
		return;
		qDebug() << "no polygon to draw";
	}
	bool isVerticalLine = std::all_of(e.begin(), e.end(),[startX = e.first().start.x()](const Edge& edge) {
		return edge.start.x() == startX && edge.end.x() == startX;
		});
	if (isVerticalLine) {
		return;
		qDebug() << "no polygon to draw";
	}
	std::sort(e.begin(), e.end(), [](const Edge &line1, const Edge &line2) {	
		return line1.start.y() < line2.start.y();			// sortovanie hran podla ich zaciatocnej suradnice
		});
	int ymin = e[0].start.y();
	int ymax = ymin;
	int y = ymin;
	for (const Edge& element : e) {
		if (element.end.y() > ymax) {
			ymax = element.end.y();							// stanovenie Maximalnej y suradnice
		}
	}
	QVector<QVector<Edge>> edgeTable = QVector<QVector<Edge>>(ymax - ymin + 1);
	QVector<Edge> eActive;
	for (const Edge& element : e) {
		int tableIndex = element.start.y() - ymin;
		if (tableIndex >= edgeTable.length()) {
			qDebug() << "hooops";
		}
		edgeTable[tableIndex].append(element);
	}
	for (int i = 0; i < edgeTable.length(); i++) {

		if (!edgeTable[i].isEmpty()) {
			eActive.append(edgeTable[i]);
		}
		std::sort(eActive.begin(), eActive.end(), [y](Edge line1, Edge line2) {
			return (y - line1.start.y()) / line1.m + line1.start.x() < (y - line2.start.y()) / line2.m + line2.start.x();
			});
		if (eActive.length() % 2 == 0) {
			for (int j = 0; j < eActive.length(); j += 2) {
				int xIntercept1 = static_cast<int>((y - eActive[j].start.y()) / eActive[j].m) + eActive[j].start.x();
				int xIntercept2 = static_cast<int>((y - eActive[j + 1].start.y()) / eActive[j + 1].m) + eActive[j + 1].start.x();
				if (bool state = xIntercept1 != xIntercept2) {
					for (int x = xIntercept1; x <= xIntercept2; x++) {
						if (isInside(x, y)) {
							setPixel(x, y, color);
							if (isInside(x, y)) {
								setPixel(x, y, color);
								if (z_buffer_in_use) {
									if (z_buffer_current_value > z_buffer_layer_array[y][x]) {
										z_buffer_layer_array[y][x] = z_buffer_current_value;
										z_buffer_color_array[y][x] = color;
									}
									else {
										setPixel(x, y, z_buffer_color_array[y][x]);
									}
								}
							}
						}
					}
				}
			}
		}
		for (int k = 0; k < eActive.length(); k++) {
			if (eActive[k].deltaY == 0) {
				eActive.removeAt(k);
				k--;
			}
			else {
				eActive[k].deltaY --;
			}
		}
		y++;
	}
	update();
}
void ViewerWidget::fillTriangleSetup(QVector<QPoint> points, QColor color,int fillAlgType) {
	QVector<QPoint> T = points;
	std::sort(T.begin(), T.end(), [](QPoint point1, QPoint point2) {
		if (point1.y() < point2.y() || point1.y() == point2.y() && point1.x() < point2.x()) {
			return TRUE;
		}
		else {
			return FALSE;
		}
		});
	if (T[0].y() == T[1].y() || T[1].y() == T[2].y()) {
		fillTriangle(T, points, color, fillAlgType);
		return;
	}
	double m = static_cast<double>(T[2].y() - T[0].y()) / (T[2].x() - T[0].x());
	QPoint P = QPoint((T[1].y() - T[0].y()) / m + T[0].x(), T[1].y());
	if (T[1].x() < P.x()) {
		fillTriangle({ T[0],T[1],P }, points,color,fillAlgType);
		fillTriangle({ T[1], P, T[2] }, points, color,fillAlgType);
	}
	else {
		fillTriangle({ T[0], P, T[1] }, points, color,fillAlgType);
		fillTriangle({ P,T[1],T[2] }, points, color,fillAlgType);
	}
}
void ViewerWidget::fillTriangle(QVector<QPoint> currentPoints, QVector<QPoint> oldPoints,QColor color ,int fillAlgType) {
	struct Edge {
		QPoint start;
		QPoint end;
		double m = 0;
	};
	QVector<QColor> colors = { Qt::red, Qt::blue,Qt::green };
	QVector<Edge> edges;
	QPoint start = currentPoints.last();
	for (int i = 0; i < currentPoints.length(); i++) {
		QPoint end = currentPoints[i];
		if (start.y() > end.y()) {
			std::swap(start, end);
		}
		if (start.y() != end.y()) {
			Edge edge;
			edge.start = start;
			edge.end = end;
			edge.m = static_cast<double>(end.y() - start.y()) / (end.x() - start.x());
			edges.append(edge);
		}

		start = currentPoints[i];
	}
	if (edges[0].end.x() != edges[1].end.x()) {
		std::sort(edges.begin(), edges.end(), [](Edge edge1, Edge edge2) {
			return edge1.end.x() < edge2.end.x();
			});
	}
	int ymin = edges[0].start.y();
	int ymax = edges[0].end.y();
	double x1 = edges[0].start.x();
	double x2 = edges[1].start.x();
	for (int y = ymin; y < ymax; y++) {
		if (x1 != x2) {
			for (int x = static_cast<int>(x1); x <= static_cast<int>(x2); x++) {
				if (fillAlgType == 2) {
					color = fillTriangleNearestNeighbour(oldPoints, QPoint(x, y), colors);
				}
				else if (fillAlgType == 3) {
					color = fillTriangleBaricentric(oldPoints, QPoint(x, y), colors);
				}
				if (isInside(x, y)) {
					setPixel(x, y, color);
					if (isInside(x, y)) {
						setPixel(x, y, color);
						if (z_buffer_in_use) {
							if (z_buffer_current_value > z_buffer_layer_array[y][x]) {
								z_buffer_layer_array[y][x] = z_buffer_current_value;
								z_buffer_color_array[y][x] = color;
							}
							else {
								setPixel(x, y, z_buffer_color_array[y][x]);
							}
						}
					}
				}
			}
		}
		x1 += 1 / edges[0].m;
		x2 += 1 / edges[1].m;
	}
	update();
}
QColor ViewerWidget::fillTriangleNearestNeighbour(QVector<QPoint> points,QPoint currentPoint, QVector<QColor> colors) {
	QVector<double> distance = QVector<double>(3);
	for (int i = 0; i < points.length(); i++) {
		distance[i] = sqrt(pow(currentPoint.x() - points[i].x(), 2) + pow(currentPoint.y() - points[i].y(), 2));
	}
	if (distance[0] <= distance[1] && distance[0] <= distance[2]) {
		return colors[0];
	}
	else if (distance[1] <= distance[2] && distance[1] <= distance[0]) {
		return colors[1];
	}
	else if (distance[2] <= distance[1] && distance[2] <= distance[0]) {
		return colors[2];
	}
	return Qt::white;
}
QColor ViewerWidget::fillTriangleBaricentric(QVector<QPoint> points, QPoint currentPoint, QVector<QColor> colors) {
	QPoint P = currentPoint;
	QVector<QPoint> T = points;
	double lambda[3];
	lambda[0] = abs((T[1].x() - P.x()) * (T[2].y() - P.y()) - (T[1].y() - P.y()) * (T[2].x() - P.x()));
	lambda[0] /= abs(static_cast<double>(T[1].x() - T[0].x()) * (T[2].y() - T[0].y()) - (T[1].y() - T[0].y()) * (T[2].x() - T[0].x()));

	lambda[1] = abs((T[0].x() - P.x()) * (T[2].y() - P.y()) - (T[0].y() - P.y()) * (T[2].x() - P.x()));
	lambda[1] /= abs(static_cast<double>(T[1].x() - T[0].x()) * (T[2].y() - T[0].y()) - (T[1].y() - T[0].y()) * (T[2].x() - T[0].x()));

	lambda[2] = 1 - lambda[0] - lambda[1];

	double red = 0, green = 0, blue = 0;
	for (int i = 0; i < 3; i++) {
		red += lambda[i] * colors[i].red();
		green += lambda[i] * colors[i].green();
		blue += lambda[i] * colors[i].blue();
	}
	return QColor(static_cast<int> (red), static_cast<int> (green), static_cast<int> (blue), 255);
}
void ViewerWidget::drawCurve(QVector<QPair<QPoint, QPoint>> points, QColor color, int algType) {
	if (algType == 0) {
		drawCurveHermint(points, color);
	}
	else {
		QVector<QPoint> vectorOfPoints;
		for (int i = 0; i < points.length(); i++) {
			vectorOfPoints.append(points[i].first);
		}
		if (algType == 1) {
			drawCurveCasteljau(vectorOfPoints, color);
		}
		else {
			drawCurveCoons(vectorOfPoints, color);
		}
	}
}
void ViewerWidget::drawCurveHermint(QVector<QPair<QPoint, QPoint>> points, QColor color) {
	auto cubicPolynoms = [](double t) ->QVector<double> {
		return { 2 * pow(t,3) - 3 * pow(t,2) + 1 ,
				-2 * pow(t,3) + 3 * pow(t,2),
				 pow(t,3) - 2 * pow(t,2) + t,
				 pow(t,3) - pow(t,2) };
		};
	double deltaT = 0.05;
	int n = points.length();
	QVector<QPair<QPoint, QPoint>> P = points;
	QPointF Q0 = QPointF();
	QPointF Q1 = QPointF();
	QVector<double> F = QVector<double>();
	int k = 0;
	for (int i = 1; i < n; i++) {
		Q0 = P[i - 1].first;
		for (double t = deltaT; t < 1; t += deltaT) {
			F = cubicPolynoms(t);
			Q1 = P[i - 1].first * F[0] + P[i].first * F[1] + (P[i - 1].second - P[i - 1].first) * F[2] + (P[i].second - P[i].first) * F[3];
			drawLine(Q0.toPoint(), Q1.toPoint(), color, 1);
			Q0 = Q1;
			k++;
		}
		k = 0;
		drawLine(Q0.toPoint(), P[i].first, color, 1);
	}
	for (int i = 0; i < P.length(); i++) {
		drawLine(P[i].first, P[i].second, Qt::red, 1);
		drawCircleBresenham(P[i].first, P[i].first + QPoint(0, 2), Qt::red);
		drawCircleBresenham(P[i].second, P[i].second + QPoint(0, 2), Qt::red);
	}
	update();
}
void ViewerWidget::drawCurveCasteljau(QVector<QPoint> points, QColor color) {
	int n = points.length();
	QVector<QVector<QPoint>> P(n);
	for (int i = 0; i < n; i++) {
		P[i] = QVector<QPoint>(n - i);
	}
	P[0] = points;
	double deltaT = 0.025;
	QPoint Q0 = P[0][0];
	QPoint Q1 = QPoint();
	for (double t = deltaT; t <= 1; t += deltaT) {
		for (int i = 1; i < n; i++) {
			for (int j = 0; j < n - i; j++) {
				P[i][j] = (1 - t) * P[i - 1][j] + t * P[i - 1][j + 1];
			}
		}
		Q1 = P[n - 1][0];
		drawLine(QPoint(static_cast<int> (Q0.x()), static_cast<int>(Q0.y())), QPoint(static_cast<int> (Q1.x()), static_cast<int>(Q1.y())), color, 1);
		Q0 = Q1;
	}
	drawLine(QPoint(static_cast<int> (Q0.x()), static_cast<int>(Q0.y())), P[0][n - 1], color, 1);
	for (int i = 0; i < n; i++) {
		drawCircleBresenham(points[i], points[i] + QPoint(0, 2), Qt::red);
	}
	update();
}
void ViewerWidget::drawCurveCoons(QVector<QPoint> points, QColor color) {
	auto cubicPolynoms = [](double t)->QVector<double> {						// Lambda funckia ktora vracia vektor hodnotu polynomov v case t
		return { -pow(t,3) / 6 + pow(t,2) / 2 - t / 2 + 1. / 6,
				  pow(t,3) / 2 - pow(t,2) + 2. / 3,
				 -pow(t,3) / 2 + pow(t,2) / 2 + t / 2 + 1. / 6,
				  pow(t,3) / 6 };
		};
	QVector<QPoint> P = points;
	int n = points.length();
	double deltaT = 0.05;
	QPointF Q0 = QPoint();
	QPointF Q1 = QPoint();
	for (int i = 3; i < n; i++) {
		QVector<double> B = cubicPolynoms(0);
		Q0 = P[i - 3] * B[0] + P[i - 2] * B[1] + P[i - 1] * B[2] + P[i] * B[3];
		if (i > 3) {
			drawLine(Q1.toPoint(), Q0.toPoint(), color, 1);
		}
		for (double t = deltaT; t <= 1; t += deltaT) {
			B = cubicPolynoms(t);
			Q1 = P[i - 3] * B[0] + P[i - 2] * B[1] + P[i - 1] * B[2] + P[i] * B[3];
			drawLine(Q0.toPoint(),Q1.toPoint(), color, 1);
			Q0 = Q1;
		}
	}
	for (const QPoint& point : points) {
		drawCircleBresenham(point, point + QPoint(0, 2), Qt::red);
	}
	update();
}

void ViewerWidget::drawObjects2D(QMap<QString,Object2D> objects) {
	QVector<Object2D> sorted_objects = objects.values();
	/*std::sort(sorted_objects.begin(), sorted_objects.end(), [](const Object2D& object1, const Object2D& object2) {
		return object1.layer_height > object2.layer_height;
		});*/	
	z_buffer_layer_array = z_buffer_default_layer_array;
	z_buffer_color_array = z_buffer_default_color_array;
	z_buffer_in_use = true;
	for (Object2D object : sorted_objects) {
		z_buffer_current_value = object.layer_height;
		if (object.type == "line") {
			drawLine(object.points[0], object.points[1], object.color_outline, 1);
		}
		else if (object.type == "circle") {
			drawCircleBresenham(object.points[0], object.points[1], object.color_outline);
		}
		else if (object.type == "polygon") {
			drawPolygon(object.points, object.color_filling, 1, object.filling_alg);
		}
		else if (object.type == "curve") {
			drawCurve(object.curve_points, object.color_outline, object.curve_type);
		}
	}
	update();
}
//3D draw functions
void ViewerWidget::drawObject(const Object_H_edge& object, Camera camera, ProjectionPlane projectionPlane, int projectionType, int representationType,int fillingAlgType, const LightSettings* ls) {
	//Storing old Vertices in vector , transforming object to projection coordinates
	QVector<Vertex> oldVertices = perspectiveCoordSystemTransformation(object,projectionType );
	//Wireframe-Model
	if (representationType == 0) {
		//hash table to store already drawed lines
		QHash <H_edge*, H_edge*> pairDrawingMap;
		for (H_edge* edge : object.edges) {
			//skipping iteration if pair line was already drawn
			if (edge->pair && pairDrawingMap.contains(edge->pair)) {
				continue;
			}
			pairDrawingMap.insert(edge, edge->pair);
			//transforming vertex to QPoint
			QPoint lineStart = QPoint(static_cast<int>(edge->vert_origin->x), static_cast<int>(edge->vert_origin->y));
			QPoint lineEnd = QPoint(static_cast<int>(edge->edge_next->vert_origin->x), static_cast<int>(edge->edge_next->vert_origin->y));
			drawLine(lineStart, lineEnd, Qt::black, 1);
		}
	}
	//Surface-Representation
	else if (representationType == 1) {
		// filling arrays of depth of image and color for Z-buffer algorithm
		z_buffer_layer_array = QVector<QVector<double>>(img->height(), QVector<double>(img->width(), -DBL_MAX));
		z_buffer_color_array = QVector<QVector<QColor>>(img->height(), QVector<QColor>(img->width(),Qt::white));
		//iterating thru faces of polygon
		for (Face* face : object.faces) {
			QVector<Vertex*> polygonVertices;
			H_edge edge = *face->edge;
			polygonVertices.append(edge.vert_origin);
			while (edge.edge_next->vert_origin != polygonVertices.first()) {
				edge = *edge.edge_next;
				polygonVertices.append(edge.vert_origin);
			}
			fillObjectPolygonSetup(polygonVertices, object.colors.value(face), fillingAlgType, ls );
		}
	}


	//updating old Vertices
	for (int i = 0; i < object.vertices.length(); i++) {
		*object.vertices[i] = oldVertices[i];
	}
	update();
}
QVector<Vertex> ViewerWidget::perspectiveCoordSystemTransformation(const Object_H_edge& object, int projectionType) {
	QVector<Vertex> oldVertices;
	//Defining translation to center where better time complexity
	double correctionX = static_cast<double>(img->width()) / 2;
	double correctionY = static_cast<double>(img->height()) / 2;
	//Transformation to world coordination
	for (Vertex* vertex : object.vertices) {
		Vertex newVertex(0, 0, 0);
		oldVertices.append(*vertex);
		//calculating new projection coordinates
		newVertex.x = (*vertex) * projectionPlane.basisVectorV;
		newVertex.y = (*vertex) * projectionPlane.basisVectorU;
		newVertex.z = *vertex * projectionPlane.basisVectorN;
		if (projectionType == 0) {
			newVertex = Vertex(correctionX, correctionY, 0) + newVertex;
		}
		*vertex = newVertex;
	}
	//Perspective Projection
	if (projectionType == 1) {
		for (Vertex* vertex : object.vertices) {
			Vertex newVertex(0, 0, 0);
			//calculating new projection coordinates
			newVertex.x = camera.position.z * vertex->x / (camera.position.z - vertex->z);
			newVertex.y = camera.position.z * vertex->y / (camera.position.z - vertex->z);
			newVertex.z = vertex->z;
			*vertex = Vertex(correctionX, correctionY, 0) + newVertex;
		}
	}
	return oldVertices;
}
double ViewerWidget::baricentricInterpolation(const QVector<Vertex*> T, Vertex* P) {
	double lambda[3];
	lambda[0] = abs((T[1]->x - P->x) * (T[2]->y - P->y) - (T[1]->y - P->y) * (T[2]->x - P->x));
	lambda[0] /= abs(static_cast<double>(T[1]->x - T[0]->x) * (T[2]->y - T[0]->y) - (T[1]->y - T[0]->y) * (T[2]->x - T[0]->x));

	lambda[1] = abs((T[0]->x - P->x) * (T[2]->y - P->y) - (T[0]->y - P->y) * (T[2]->x - P->x));
	lambda[1] /= abs(static_cast<double>(T[1]->x - T[0]->x) * (T[2]->y - T[0]->y) - (T[1]->y - T[0]->y) * (T[2]->x - T[0]->x));

	lambda[2] = 1 - lambda[0] - lambda[1];

	return T[0]->z * lambda[0] + T[1]->z * lambda[1] + T[2]->z * lambda[2];
}
void ViewerWidget::fillObjectPolygonSetup(const QVector<Vertex*> vertices, QColor color, int fillAlgType, const LightSettings* ls) {
	auto phongLightningModel = [&](Vertex& vertex)->QColor {
		// inicializing vectors N(normal) L(light) V(viewer) R(reflexion)
		QVector3D N = vertex.toQVector3D().normalized();
		QVector3D L = (ls->lightPosition - vertex).toQVector3D().normalized();
		QVector3D V = (camera.position - vertex).toQVector3D().normalized();
		QVector3D R = (2 * (QVector3D::dotProduct(L, N)) * N - L).normalized();
		//-------------------
		double red = 0;
		double green = 0;
		double blue = 0;

		if (QVector3D::dotProduct(L, N) > 0) {
			double coef;
			//Reflexion part of phong model
			if (QVector3D::dotProduct(V, R) > 0) {
				coef = ls->rs * pow(QVector3D::dotProduct(V, R), ls->h);
				red += ls->lightIntesity.red() * coef;
				green += ls->lightIntesity.green() * coef;
				blue += ls->lightIntesity.blue() * coef;
			}
			//Difusion part of phong model
			coef = ls->rd * QVector3D::dotProduct(L, N);
			red += ls->lightIntesity.red() * coef;
			green += ls->lightIntesity.green() * coef;
			blue += ls->lightIntesity.blue() * coef;
		}
		//Ambient part of phong model
		red += ls->lightIntesityAmbient.red() * ls->ra;
		green += ls->lightIntesityAmbient.green() * ls->ra;
		blue += ls->lightIntesityAmbient.blue() * ls->ra;

		//qDebug() << static_cast<int>(red) << static_cast<int>(green) << static_cast<int>(blue);
		return QColor(std::max(std::min(static_cast<int>(red), 255),0), std::max(std::min(static_cast<int>(green), 255) , 0), std::max(std::min(static_cast<int>(blue), 255),0));
	};

	if (vertices.length() != 3) {
		return;
	}
	QVector<Vertex*> T = vertices;
	QVector<QColor> colors;
	if (ls != nullptr) {
		for (int i = 0; i < vertices.length(); i++) {
			colors.append(phongLightningModel(*T[i]));
		}
	}
	else {
		colors.append(color);
	}
	//Sorting all vertices primarly with their y-coordinate and secondary with their x-coordinate
	std::sort(T.begin(), T.end(), [](const Vertex* vertex1, const Vertex* vertex2) {
		if (vertex1->y < vertex2->y || vertex1->y == vertex2->y && vertex1->x < vertex2->x) {
			return TRUE;
		}
		else {
			return FALSE;
		}
		});
	if (T[0]->y == T[1]->y || T[1]->y == T[2]->y) {
		fillObjectPolygon(T, vertices,colors, fillAlgType);
		return;
	}
	double m = static_cast<double>(T[2]->y - T[0]->y) / (T[2]->x - T[0]->x);
	Vertex *P = new Vertex((T[1]->y - T[0]->y) / m + T[0]->x, T[1]->y,0);
	if (T[1]->x < P->x) {
		fillObjectPolygon({ T[0],T[1],P }, vertices, colors, fillAlgType);
		fillObjectPolygon({ T[1], P, T[2] }, vertices, colors, fillAlgType);
	}
	else {
		fillObjectPolygon({ T[0], P, T[1] }, vertices, colors, fillAlgType);
		fillObjectPolygon({ P,T[1],T[2] }, vertices, colors, fillAlgType);
	}
}
void ViewerWidget::fillObjectPolygon(const QVector<Vertex*> vertices,const QVector<Vertex*> oldVertices,QVector<QColor> colors, int fillAlgType) {
	struct Edge {
		Vertex start;
		Vertex end;
		double m = 0;
	};
	//Interpolation that interpolates thru given Point and Vertices of triangle
	//creating cosnt variables for better time complexitya
	const double T0x = oldVertices[0]->x;
	const double T0y = oldVertices[0]->y;
	const double T0z = oldVertices[0]->z;
	const double T1x = oldVertices[1]->x;
	const double T1y = oldVertices[1]->y;
	const double T1z = oldVertices[1]->z;
	const double T2x = oldVertices[2]->x;
	const double T2y = oldVertices[2]->y;
	const double T2z = oldVertices[2]->z;
	//color values
	const int C0R = colors.at(0).red();
	const int C0G = colors.at(0).green();
	const int C0B = colors.at(0).blue();
	const int C1R = colors.at(1).red();
	const int C1G = colors.at(1).green();
	const int C1B = colors.at(1).blue();
	const int C2R = colors.at(2).red();
	const int C2G = colors.at(2).green();
	const int C2B = colors.at(2).blue();

	auto interpolation = [&](const Vertex& P, double& lambda1, double& lambda2, double& lambda3)->void  {
		const double Px = P.x;
		const double Py = P.y;
		const double divider = abs((T1x - T0x) * (T2y - T0y) - (T1y - T0y) * (T2x - T0x));
		lambda1 = abs((T1x - Px) * (T2y - Py) - (T1y - Py) * (T2x - Px)) / divider;
		lambda2 = abs((T0x - Px) * (T2y - Py) - (T0y - Py) * (T2x - Px)) / divider;
		lambda3 = 1 - lambda1 - lambda2;
	};

	auto nearestNeighbour = [&](const Vertex& P) ->QColor {
		double const distance0 = sqrt(pow(P.x - T0x, 2) + pow(P.y - T0y, 2));
		double const distance1 = sqrt(pow(P.x - T1x, 2) + pow(P.y - T1y, 2));
		double const distance2 = sqrt(pow(P.x - T2x, 2) + pow(P.y - T2y, 2));

		if (distance0 <= distance1 && distance0 <= distance2) {
			return colors.at(0);
		}
		else if (distance1 <= distance2 && distance1 <= distance0) {
			return colors.at(1);
		}
		else if (distance2 <= distance1 && distance2 <= distance0) {
			return colors.at(2);
		}
	};



	QVector<Edge> edges;
	QColor color = colors.first();
	bool usingLightSettings = colors.length() == 3;
	Vertex start = *vertices.last();
	for (int i = 0; i < vertices.length(); i++) {
		Vertex end = *vertices[i];
		if (start.y > end.y) {
			std::swap(start, end);
		}
		if (start.y != end.y) {
			Edge edge;
			edge.start = start;
			edge.end = end;
			edge.m = static_cast<double>(end.y - start.y) / (end.x - start.x);
			edges.append(edge);
		}
		start = *vertices[i];
	}
	if (edges.length() != 2) {
		return;
	}
	if (edges[0].end.x > edges[1].end.x) {
		std::swap(edges[0], edges[1]);
	}
	int ymin = edges[0].start.y;
	int ymax = edges[0].end.y;
	double x1 = edges[0].start.x;
	double x2 = edges[1].start.x;
	double red = 0, green = 0, blue = 0;
	double lambda0, lambda1, lambda2;
	double z;
	//current Vertex  indicates itteration position in image
	Vertex currentVertex = Vertex(static_cast<int>(x1), ymin,0);
	for (int y = ymin ; y < ymax; y++) {
		for (int x = x1; x <= static_cast<int>(x2); x++) {
			interpolation(currentVertex, lambda0, lambda1, lambda2);
			z = lambda0 * T0z + lambda1 * T1z + lambda2 * T2z;
			if (z > z_buffer_layer_array.at(y).at(x)) {
				if (usingLightSettings) {
					if (fillAlgType == 1) {
						red = lambda0 * C0R + lambda1 * C1R + lambda2 * C2R;
						green = lambda0 * C0G + lambda1 * C1G + lambda2 * C2G;
						blue = lambda0 * C0B + lambda1 * C1B + lambda2 * C2B;
						color = QColor(static_cast<int>(red), static_cast<int> (green), static_cast<int> (blue), 255);
						red = 0; green = 0; blue = 0;
					}
					else {
						color = nearestNeighbour(currentVertex);
					}
				}
				z_buffer_layer_array[y][x] = z;
				//z_buffer_color_array[y][x] = color;
				setPixel(x, y, color);
			}
			currentVertex.x++;
		}
		x1 += 1 / edges[0].m;
		x2 += 1 / edges[1].m;
		currentVertex.x = static_cast<int>(x1);
		currentVertex.y++;
	}
}

//Crop functions
QVector<QPoint> ViewerWidget::cyrusBeck(QPoint P1, QPoint P2) {
	if (P1.x() < 0 && P2.x() < 0 || P1.x() > img->width() && P2.x() > img->width() ||
		P1.y() < 0 && P2.y() < 0 || P1.y() > img->height() && P2.y() > img->height()) {
		return QVector<QPoint>();
	}
	if (!isInside(P1.x(), P1.y()) || !isInside(P2.x(), P2.y())) {
		double tMin = 0;
		double tMax = 1;
		QPoint vectorD = P2 - P1;
		QPoint E[4] = { QPoint(0,0),
			QPoint(0,img->height()),
			QPoint(img->width(),img->height()),
			QPoint(img->width(),0) };
		for (int i = 0; i < 4; i++) {
			QPoint lineVector;
			if (i < 3) {
				lineVector = E[i + 1] - E[i];
			}
			else {
				lineVector = E[0] - E[i];
			}
			QPoint normalVector = QPoint(lineVector.y(), -lineVector.x());
			QPoint vectorW = P1 - E[i];
			int dotProductDN = vectorD.x() * normalVector.x() + vectorD.y() * normalVector.y();
			int dotProductWN = vectorW.x() * normalVector.x() + vectorW.y() * normalVector.y();
			if (dotProductDN != 0) {
				double t = -static_cast<double>(dotProductWN) / dotProductDN;
				if (dotProductDN > 0 && t <= 1) {
					tMin = std::max(t, tMin);
				}
				else if (dotProductDN < 0 && t >= 0) {
					tMax = std::min(t, tMax);
				}
			}
		}
		if (tMin < tMax) {
			QPoint newP1 = P1 + (P2 - P1) * tMin;
			QPoint newP2 = P1 + (P2 - P1) * tMax;
			return { newP1, newP2 };
		}
		else {
			return QVector<QPoint>();
		}
	}
	else {
		return { P1, P2 };
	}
}
QVector<QPoint> ViewerWidget::sutherlandHodgman(QVector<QPoint> V) {
	QVector<QPoint> W;
	QPoint S = V.last();
	int xMin = 0;
	int x[4] = {0, 0, -img->width(), -img->height()};				// inicializacia pola hodnot xmin pre jednotlive hrany orezavania
	for (int j = 0; j < 4; j++) {
		xMin = x[j];
		for (int i = 0; i < V.length(); i++) {
			if (V[i].x() >= xMin) {
				if (S.x() >= xMin) {
					if (!W.contains(V[i])) {
						W.append(V[i]);
					}
				}
				else {
					QPoint Intersect = QPoint(xMin,static_cast<int>( S.y() + (xMin - S.x()) * static_cast<double>(V[i].y() - S.y()) / (V[i].x() - S.x())));
					if (!W.contains(Intersect)) {
						W.append(Intersect);
					}
					if (!W.contains(V[i])) {
						W.append(V[i]);
					}
				}
			}
			else {
				if (S.x() >= xMin) {
					QPoint Intersect = QPoint(xMin, static_cast<int>(S.y() + (xMin - S.x()) * static_cast<double>(V[i].y() - S.y()) / (V[i].x() - S.x())));
					if (!W.contains(Intersect)) {
						W.append(Intersect);
					}
				}
			}
			S = V[i];
		}
		if (W.isEmpty()) {
			return QVector<QPoint>();
		}
		V = W;
		for (QPoint& point : V) {
			point = QPoint(point.y(), -point.x());
		}
		S = V.last();
		W.clear();
	}
	W = V;
	return W;
}

void ViewerWidget::clear()
{
	img->fill(Qt::white);
	update();
}

//Slots
void ViewerWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	QRect area = event->rect();
	painter.drawImage(area, *img, area);
}

//---------------------VTK file functions------------------------------

void createCubeVTK(double d,QString filename) {
	QVector<Vertex*> vertices = {
		new Vertex(0, 0, 0), new Vertex(0, d, 0), new Vertex(d, d, 0), new Vertex(d, 0, 0),
		new Vertex(0,0,d), new Vertex(0,d,d), new Vertex(d,d,d), new Vertex(d,0,d) };

	QFile file(filename + ".vtk");

	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out << "#vtk DataFile Version 3.0\n";
		out << "vtk output\nASCII\nDATASET POLYDATA\n";
		out << "POINTS " << vertices.size() << " float\n";
		for (int i = 0; i < vertices.size(); i++) {
			out << vertices[i]->x  - d/2<< " " << vertices[i]->y  - d/2<< " " << vertices[i]->z - d/2<< "\n";
		}
		out << "POLYGONS 12 48\n";
		out << "3 0 1 3\n";
		out << "3 1 2 3\n";
		out << "3 0 1 5\n";
		out << "3 0 4 5\n";
		out << "3 0 3 4\n";
		out << "3 3 7 4\n";
		out << "3 3 2 7\n";
		out << "3 2 6 7\n";
		out << "3 1 5 6\n";
		out << "3 2 1 6\n";
		out << "3 4 7 5\n";
		out << "3 5 7 6\n";
		file.close();
		qDebug() << "file saved sucssefully";
	}
	else {
		qDebug() << "file wasnt open";
	}
}

void createCubeVTK(QVector<Vertex> vertices, QString filename) {
	QFile file(filename + ".vtk");

	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out << "#vtk DataFile Version 3.0\n";
		out << "vtk output\nASCII\nDATASET POLYDATA\n";
		out << "POINTS " << vertices.size() << "int\n";
		for (int i = 0; i < vertices.size(); i++) {
			out << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << "\n";
		}
		out << "POLYGONS 12 48" << "\n";
		out << "3 0 1 3\n";
		out << "3 1 2 3\n";
		out << "3 0 1 5\n";
		out << "3 0 4 5\n";
		out << "3 0 3 4\n";
		out << "3 3 7 4\n";
		out << "3 3 2 7\n";
		out << "3 2 6 7\n";
		out << "3 1 5 6\n";
		out << "3 2 1 6\n";
		out << "3 4 7 5\n";
		out << "3 5 7 6\n";
		file.close();
		qDebug() << "file saved sucssefully";
	}
	else {
		qDebug() << "file wasnt open";
	}
}

Object_H_edge loadPolygonsVTK(QString filename) {
	QVector<Vertex*> vertices;
	QVector<Face*> faces;
	QHash<Face*, QColor> colors;
	QHash <QPair<int, int>, H_edge*> edgeMap;
	QHash <Vertex*, int> vertexIndexMap;
	QVector<H_edge*> edges;
	// Inicialization of random generator for generating colors of faces
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(0, 255);

	QFile file(filename);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QString fileHeader = "";
		QTextStream input(&file);
		for (int i = 0; i < 4; i++) {
			fileHeader += input.readLine() + "\n";
		}
		qDebug() << fileHeader;
		if (fileHeader != VTK_FILE_HEADER) {
			qDebug() << "File format is incorrect";
			return Object_H_edge();
		}
		QVector<QString> headerData = input.readLine().split(' ');
		bool isInt;
		int pointCount = headerData[1].toInt(&isInt);
		qDebug() << headerData;
		if (headerData[0] != "POINTS" || !isInt || (headerData[2] != "int" && headerData[2] != "float")) {
			qDebug() << "Content of file has wrong format";
			return Object_H_edge();
		}
		for (int i = 0; i < pointCount; i++) {
			QVector<QString> points = input.readLine().split(' ');
			if (points.length() != 3) {
				qDebug() << "Invalid point count";
				return Object_H_edge();
			}
			Vertex *vertex = new Vertex(points[0].toDouble(), points[1].toDouble(), points[2].toDouble());
			vertices.append(vertex);
			vertexIndexMap.insert(vertices.last(), i);
		}
		QString objectType = "";
		int polygonCount = 0;
		int valueCount = 0;
		input >> objectType >> polygonCount >> valueCount;
		input.readLine();
		for (int i = 0; i < polygonCount; i++) {
			QString str = input.readLine();
			QVector<QString> data = str.split(' ');
			if (data.length() < 4) {
				return Object_H_edge();
			}
			QVector<int> vertexIndex;
			for (int j = 1; j < data.length(); j++) {
				vertexIndex.append(data[j].toInt());
			}
			int edgesInPolygonCount = data[0].toInt();
			QVector<H_edge*> edgesInPolygon(edgesInPolygonCount);
			for (int j = 0; j < edgesInPolygonCount; j++) {
				edgesInPolygon[j] = new H_edge(vertices[vertexIndex[j]], nullptr, nullptr, nullptr, nullptr);
			}
			Face* currentFace = new Face(edgesInPolygon[0]);
			faces.append(currentFace);
			QColor currentColor(dis(gen), dis(gen), dis(gen), 255);
			colors.insert(currentFace, currentColor);
			for (int j = 0; j < edgesInPolygonCount; j++) {
				if (j != edgesInPolygonCount - 1) {
					edgesInPolygon[j]->edge_next = edgesInPolygon[j + 1];
				}
				else {
					edgesInPolygon[j]->edge_next = edgesInPolygon.first();
				}
				if (j != 0) {
					edgesInPolygon[j]->edge_prev = edgesInPolygon[j - 1];
				}
				else {
					edgesInPolygon[j]->edge_prev = edgesInPolygon.last();
				}
				edgesInPolygon[j]->face = currentFace;
				int indexOfEdgeStart = vertexIndexMap.value(edgesInPolygon[j]->vert_origin);
				int indexOfEdgeEnd = vertexIndexMap.value(edgesInPolygon[j]->edge_next->vert_origin);
				if (edgeMap.contains({ indexOfEdgeEnd, indexOfEdgeStart })) {
					edgesInPolygon[j]->pair = edgeMap[{indexOfEdgeEnd, indexOfEdgeStart}];
					edgesInPolygon[j]->pair->pair = edgesInPolygon[j];
					edgeMap.remove({indexOfEdgeEnd, indexOfEdgeStart});
				}
				else {
					edgeMap.insert({ indexOfEdgeStart, indexOfEdgeEnd }, edgesInPolygon[j]);
				}
			}
			edges.append(edgesInPolygon);
		}
		qDebug() << filename << " : file has been loaded";
		file.close();
		Object_H_edge object(vertices, edges, faces);
		object.colors = colors;
		return object;
	}
	else {
		qDebug() << filename << " : file failed to open";
		return Object_H_edge();
	}
}

void savePolygonsVTK(QString filename, Object_H_edge object) {
	QFile file(filename + ".vtk");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QHash <Vertex*, int> vertexIndexMap;
		QTextStream output(&file);
		output << VTK_FILE_HEADER;
		std::cout << VTK_FILE_HEADER;
		output << "POINTS " << object.vertices.length() << " float\n";
		std::cout << "POINTS " << object.vertices.length() << " float\n";
		for (int i = 0; i < object.vertices.length(); i++) {
			output << object.vertices[i]->x << " " << object.vertices[i]->y << " " << object.vertices[i]->z << "\n";
			qDebug() << object.vertices[i]->x << object.vertices[i]->y << object.vertices[i]->z;
			vertexIndexMap.insert(object.vertices[i], i);
		}
		QString polygonData = "";
		int polygonDataLength = 0;
		for (const Face *face : object.faces) {
			int dataLenght = 0;
			QString polygonDataLine = "";
			H_edge *firstEdge = face->edge;
			polygonDataLine.append(" " + QString::number(vertexIndexMap.value(firstEdge->vert_origin)));
			H_edge *currentEdge = firstEdge->edge_next;
			polygonDataLine.append(" " + QString::number(vertexIndexMap.value(currentEdge->vert_origin)));
			H_edge *nextEdge = currentEdge;
			dataLenght = 2;
			while(*nextEdge != *firstEdge->edge_prev) {
				nextEdge = currentEdge->edge_next;
				polygonDataLine.append(" " + QString::number(vertexIndexMap.value(nextEdge->vert_origin)));
				dataLenght++;
				currentEdge = nextEdge;
			}
			polygonDataLine.push_front(QString::number(dataLenght));
			polygonDataLength += 1 + dataLenght;
			polygonData += polygonDataLine + "\n";
		}
		polygonData.push_front("POLYGONS " + QString::number(object.faces.length()) + " " + QString::number(polygonDataLength) + "\n");
		output << polygonData;
		std::cout << polygonData.toStdString() << std::endl;

		file.close();
		qDebug() << filename << " : writing to file succsesfull";
	}
	else {
		qDebug() << filename << " : file failed to open";
	}
}

void rotateCubeAnimation(double d, int frames) {
	QVector<Vertex> vertices = {
		Vertex(0, 0, 0),Vertex(0, d, 0),Vertex(d, d, 0),Vertex(d, 0, 0),
		Vertex(0,0,d), Vertex(0,d,d),Vertex(d,d,d), Vertex(d,0,d) };
	double alphaIncrement = 2 * M_PI / frames;
	double alpha = 0;
	auto rotatePoints = [](double alpha, QVector<Vertex> vertices)->QVector<Vertex> {
		for (Vertex& vertex : vertices) {
			vertex.y *= (cos(alpha) + sin(alpha));
			vertex.z *= (cos(alpha) - sin(alpha));
		}
		return vertices;
		};
	for (int frame = 0; frame < frames; frame++) {
		createCubeVTK(rotatePoints(alpha, vertices), "cubeAnimationFrame" + QString::number(frame));
	}
}

void createUvSphereVTK(double r, int longitude, int latitude, QString filename, int mode) {
	QVector<QVector<Vertex>> vertices;
	double thetaAngle = -M_PI/2;
	double phiAngle = 0;
	double rho = r;

	double thetaAngleDivision = M_PI / latitude;
	double phiAngleDivision = 2 * M_PI / longitude;

	int verticesCount = 0;
	QFile file(filename + ".vtk");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		for (int i = 0; i <= latitude; i++) {
			QVector<Vertex> verticesOneLatitude;
			for (int j = 0; j <= longitude; j++) {
				Vertex vertex;
				if (mode == 0) {
					vertex.x = rho * cos(thetaAngle) * cos(phiAngle);
					vertex.y = rho * cos(thetaAngle) * sin(phiAngle);
					vertex.z = rho * sin(thetaAngle);
				}
				else if (mode == 1) {
					vertex.x = 200 * cos(thetaAngle) * cos(phiAngle);
					vertex.y = 100 * cos(thetaAngle) * sin(phiAngle);
					vertex.z = 80 * sin(thetaAngle);
				}
				verticesOneLatitude.append(vertex);
				phiAngle += phiAngleDivision;
				verticesCount++;
				if (i == 0 || i == latitude) {
					break;
				}
			}
			vertices.append(verticesOneLatitude);
			phiAngle = 0;
			thetaAngle += thetaAngleDivision;
		}
		out << VTK_FILE_HEADER;
		out << "POINTS " <<  verticesCount << " float\n";
		for (int i = 0; i < vertices.length(); i++) {
			for (const Vertex& vertex : vertices[i]) {
				out << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
			}
		}
		out << "POLYGONS " << 2 * (longitude * latitude) - 2  << " " << 4 * (2 * (longitude * latitude) - 2) << "\n";
		int polygonCount = 0;
		for (int i = 0; i < vertices[1].length(); i++) {
			if (i < vertices[1].length() - 1) {
				out << "3 0 " << i + 1 << " " << i + 2 << "\n";
			}
			else {
				out << "3 0 " << i + 1 << " 1\n";
			}
			polygonCount++;
		}
		int itteratedVertices = 0;
		int itteratedVerticesNext = 0;
		for (int i = 1; i < vertices.length() - 2; i++) {
			itteratedVertices += vertices[i - 1].length();
			itteratedVerticesNext = itteratedVertices + vertices[i].length();
			for (int j = 0; j < vertices[i].length(); j++) {
				if (j < vertices[i].length() - 1) {
					out << "3 " << itteratedVertices + j << " " << itteratedVerticesNext + j << " " << itteratedVerticesNext + j + 1 << "\n";
					out << "3 " << itteratedVertices + j << " " << itteratedVerticesNext + j + 1 << " " << itteratedVertices + j + 1 << "\n";
					polygonCount += 2;
				}
				else {
					out << "3 " << itteratedVertices + j << " " << itteratedVerticesNext + j << " " << itteratedVerticesNext << "\n";
					out << "3 " << itteratedVertices + j << " " << itteratedVerticesNext << " " << itteratedVertices << "\n";
					polygonCount += 2;
				}
			}
		}
		itteratedVertices += vertices[1].length();
		itteratedVerticesNext = verticesCount - 1;
		for (int j = 0; j < vertices[1].length(); j++) {
			if (j < vertices[1].length() - 1) {
				out << "3 " << itteratedVertices + j << " " << itteratedVerticesNext << " " << itteratedVertices + j + 1 << "\n";
			}
			else {
				out << "3 " << itteratedVertices + j << " " << itteratedVerticesNext << " " << itteratedVertices + j << "\n";
			}
			polygonCount++;
		}
		file.close();
		qDebug() << "file was saved succsesfully";
	}
	else {
		qDebug() << "file wasnt opened succsesfully";
	}
}

//---------------------------------------------------------------------