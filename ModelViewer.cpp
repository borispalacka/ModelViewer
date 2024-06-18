#include "ModelViewer.h"
#include <cmath>
#include <QDockWidget>

ModelViewer::ModelViewer(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::ModelViewerClass)
{
	ui->setupUi(this);
	vW = new ViewerWidget(QSize(700, 700));
	ui->scrollArea->setWidget(vW);
	ui->scrollArea->setBackgroundRole(QPalette::Dark);
	ui->scrollArea->setWidgetResizable(true);
	ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	ui->tableWidgetObjectList->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->tableWidgetObjectList->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidgetObjectList->setSelectionMode(QAbstractItemView::NoSelection);
	ui->tableWidgetObjectList->setColumnWidth(0, 50);
	ui->tableWidgetObjectList->setColumnWidth(1, 75);
	ui->tableWidgetObjectList->setColumnWidth(2, 75);
	ui->tableWidgetObjectList->setColumnWidth(3, 50);


	vW->setObjectName("ViewerWidget");
	vW->installEventFilter(this);

	globalColor = Qt::blue;
	QString style_sheet = QString("background-color: #%1;").arg(globalColor.rgba(), 0, 16);
	ui->pushButtonSetColor->setStyleSheet(style_sheet);

	ui->dockWidget_4->setHidden(true);
}

// Event filters
bool ModelViewer::eventFilter(QObject* obj, QEvent* event)
{
	if (obj->objectName() == "ViewerWidget") {
		return ViewerWidgetEventFilter(obj, event);
	}
	return false;
}

//ViewerWidget Events
bool ModelViewer::ViewerWidgetEventFilter(QObject* obj, QEvent* event)
{
	ViewerWidget* w = static_cast<ViewerWidget*>(obj);

	if (!w) {
		return false;
	}

	if (event->type() == QEvent::MouseButtonPress) {
		ViewerWidgetMouseButtonPress(w, event);
	}
	else if (event->type() == QEvent::MouseButtonRelease) {
		ViewerWidgetMouseButtonRelease(w, event);
	}
	else if (event->type() == QEvent::MouseMove) {
		ViewerWidgetMouseMove(w, event);
	}
	else if (event->type() == QEvent::Leave) {
		ViewerWidgetLeave(w, event);
	}
	else if (event->type() == QEvent::Enter) {
		ViewerWidgetEnter(w, event);
	}
	else if (event->type() == QEvent::Wheel) {
		ViewerWidgetWheel(w, event);
	}

	return QObject::eventFilter(obj, event);
}
void ModelViewer::ViewerWidgetMouseButtonPress(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
	qDebug() << e->pos().x() << e->pos().y();
	//Draw Line
	if (e->button() == Qt::LeftButton && ui->toolButtonDrawLine->isChecked() && ui->toolButtonDrawLine->isEnabled())
	{
		if (w->getDrawLineActivated()) {
			w->setDrawLineEnd(e->pos());
			int line_index = 0;
			QString object_name = "";
			while (true) {
				if (object_map.contains("line (" + QString::number(line_index) + ")")) {
					line_index++;
				}
				else {
					object_name = "line (" + QString::number(line_index) + ")";
					break;
				}
			}
			current_object = Object2D("line", object_name, {w->getDrawLineBegin(),w->getDrawLineEnd()}, globalColor, object_map.size());
			object_map.insert(current_object.name, current_object);
			//w->drawLine(w->getDrawLineBegin(),w->getDrawLineEnd(), globalColor, ui->comboBoxLineAlg->currentIndex());
			w->drawObjects2D(object_map);
			w->setDrawLineActivated(false);
			ui->toolButtonDrawLine->setChecked(false);
			colored_row_index = object_map.size() - 1;
			objectTableWidgetUpdate();
		}
		else {
			w->setDrawLineBegin(e->pos());
			w->setDrawLineActivated(true);
			w->setPixel(e->pos().x(), e->pos().y(), globalColor);
			w->update();
		}
	}
	//Draw Circle
	else if (e->button() == Qt::LeftButton && ui->toolButtonDrawCircle->isChecked() && ui->toolButtonDrawCircle->isEnabled()) {
		if (w->getDrawLineActivated()) {
			w->setDrawLineEnd(e->pos());
			int circle_index = 0;
			QString object_name = "";
			while (true) {
				if (object_map.contains("circle (" + QString::number(circle_index) + ")")) {
					circle_index++;
				}
				else {
					object_name = "circle (" + QString::number(circle_index) + ")";
					break;
				}
			}
			current_object = Object2D("circle", object_name, { w->getDrawLineBegin(),w->getDrawLineEnd() }, globalColor, object_map.size());
			object_map.insert(current_object.name, current_object);
			w->drawObjects2D(object_map);
			w->setDrawLineActivated(false);
			ui->toolButtonDrawCircle->setChecked(false);
			objectTableWidgetUpdate();
		}
		else {
			w->setDrawLineBegin(e->pos());
			w->setDrawLineActivated(true);
			w->setPixel(e->pos().x(), e->pos().y(), globalColor);
			w->update();
		}
	}
	//Add Polygon Point
	else if (e->button() == Qt::LeftButton && ui->toolButtonDrawPolygon->isChecked() && ui->toolButtonDrawPolygon->isEnabled()) {
		w->getDrawPolygonPoints().append(e->pos());
		w->drawCircleBresenham(e->pos(), e->pos() + QPoint(0, 2), Qt::black);
		if (w->getDrawPolygonPoints().length() >= 2) {
			w->setDrawPolygonActivated(true);
		}
	}
	//Draw Polygon
	else if (e->button() == Qt::RightButton && ui->toolButtonDrawPolygon->isChecked() && ui->toolButtonDrawPolygon->isEnabled()) {
		if (w->getDrawPolygonActivated()) {
			if (w->getDrawPolygonPoints().length() == 2) {
				QPoint point1 = w->getDrawPolygonPoints()[0];
				QPoint point2 = w->getDrawPolygonPoints()[1];
				int xmin = std::min({ point1.x(), point2.x() });
				int xmax = std::max({ point1.x(), point2.x() });
				int ymin = std::min({ point1.y(), point2.y() });
				int ymax = std::max({ point1.y(), point2.y() });
				w->getDrawPolygonPoints().clear();
				w->getDrawPolygonPoints().append({ QPoint(xmin,ymin),QPoint(xmin,ymax), QPoint(xmax,ymax),QPoint(xmax,ymin) });
			}
			int polygon_index = 0;
			QString object_name = "";
			while (true) {
				if (object_map.contains("polygon (" + QString::number(polygon_index) + ")")) {
					polygon_index++;
				}
				else {
					object_name = "polygon (" + QString::number(polygon_index) + ")";
					break;
				}
			}
			current_object = Object2D("polygon",object_name, w->getDrawPolygonPoints(), Qt::black, globalColor,ui->comboBoxFillingAlg->currentIndex(), object_map.size());
			object_map.insert(current_object.name, current_object);
			w->drawObjects2D(object_map);
			w->setDrawPolygonActivated(false);
			ui->toolButtonDrawPolygon->setChecked(false);
			objectTableWidgetUpdate();
		}
	}
	//Add Curve Point
	else if (e->button() == Qt::LeftButton && ui->toolButtonDrawCurve->isChecked() && ui->toolButtonDrawCurve->isEnabled()){
		QPair<QPoint, QPoint> vector = { e->pos(),e->pos() - QPoint(0, 100) };
		w->getDrawCurveMasterPoints().append(vector);
		ui->comboBoxCurveAlg->setEnabled(false);
		if (w->getDrawCurveMasterPoints().length() >= 2 && ui->comboBoxCurveAlg->currentIndex() != 2) {
			w->setDrawCurveActivated(true);
		}
		else if (w->getDrawCurveMasterPoints().length() >= 4 && ui->comboBoxCurveAlg->currentIndex() == 2) {
			w->setDrawCurveActivated(true);
		}
	}
	//Draw Curve
	else if (e->button() == Qt::RightButton && ui->toolButtonDrawCurve->isChecked() && ui->toolButtonDrawCurve->isEnabled()) {
		if (w->getDrawCurveActivated()) {
			int curve_index = 0;
			QString object_name = "";
			while (true) {
				if (object_map.contains("curve (" + QString::number(curve_index) + ")")) {
					curve_index++;
				}
				else {
					object_name = "curve (" + QString::number(curve_index) + ")";
					break;
				}
			}
			current_object = Object2D("curve", object_name, w->getDrawCurveMasterPoints(), globalColor, ui->comboBoxCurveAlg->currentIndex(), object_map.size());
			object_map.insert(current_object.name, current_object);
			w->drawObjects2D(object_map);
			w->setDrawCurveActivated(false);
			w->getDrawCurveMasterPoints().clear();
			ui->toolButtonDrawCurve->setChecked(false);
			objectTableWidgetUpdate();
		}
	}

	//Edit Position
	else if (ui->toolButtonEditPosition->isChecked() && e->button() == Qt::LeftButton) {
		w->setDragReady(true);
		w->setDragStartingPosition(e->pos());
		if(current_object.type == "curve") {
			QVector<QPoint> points;
			for (const QPair<QPoint, QPoint>& pair : current_object.curve_points) {
				points.append({pair.first, pair.second});
			}
			QPoint P = e->pos();
			std::sort(points.begin(), points.end(), [P](const QPoint point1, const QPoint point2) {
				return sqrt(pow(point1.x() - P.x(), 2) + pow(point1.y() - P.y(), 2)) < sqrt(pow(point2.x() - P.x(), 2) + pow(point2.y() - P.y(), 2));
				});
			w->setDragedPoint(points.first());
		}
	}
}
void ModelViewer::ViewerWidgetMouseButtonRelease(ViewerWidget* w, QEvent* event)
{
	w->setDragReady(false);
	w->setDragStartingPosition(QPoint());
}
void ModelViewer::ViewerWidgetMouseMove(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
	if (ui->toolButtonEditPosition->isChecked() && w->getDragReady()) {
		QPoint delta = w->getDragStartingPosition() - e->pos();
		if (current_object.type == "line" || current_object.type == "circle") {
			current_object.points = { current_object.points[0] - delta, current_object.points[1] - delta };
			object_map[current_object.name] = current_object;
		}
		else if (current_object.type == "polygon") {
			QVector<QPoint> resultPolygon;
			for (const QPoint& point : current_object.points) {
				resultPolygon.append(point-delta);
			}
			current_object.points = resultPolygon;
			object_map[current_object.name] = current_object;
		}
		else if (current_object.type == "curve") {
			QVector<QPair<QPoint, QPoint>> pointTmp = current_object.curve_points;
			for (QPair<QPoint, QPoint>& pair :pointTmp) {
				if (w->getDragedPoint() == pair.first) {
					pair.first -= delta;
					w->setDragedPoint(pair.first);
					break;
				}
				else if (w->getDragedPoint() == pair.second) {
					pair.second -= delta;
					w->setDragedPoint(pair.second);
					break;
				}
			}
			current_object.curve_points = pointTmp;
			object_map[current_object.name] = current_object;
		}
		w->setDragStartingPosition(e->pos());
		vW->clear();
		w->drawObjects2D(object_map);
		w->update();
	}
}
void ModelViewer::ViewerWidgetLeave(ViewerWidget* w, QEvent* event)
{
}
void ModelViewer::ViewerWidgetEnter(ViewerWidget* w, QEvent* event)
{
}
void ModelViewer::ViewerWidgetWheel(ViewerWidget* w, QEvent* event)
{
	qDebug() << event->type();
	QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
	if (ui->toolButtonEditScale->isChecked()) {
		double value = 1;
		if (wheelEvent->angleDelta().y() > 0) { //scroll up
			value = 1.25;
		}
		else if(wheelEvent->angleDelta().y() < 0){
			value = 0.75;
		}
		qDebug() << wheelEvent->angleDelta().y();
		if (ui->toolButtonDrawLine->isChecked()) {
			int x = static_cast<int>((vW->getDrawLineEnd().x() - vW->getDrawLineBegin().x()) * value + vW->getDrawLineBegin().x() + 0.5);
			int y = static_cast<int>((vW->getDrawLineEnd().y() - vW->getDrawLineBegin().y()) * value + vW->getDrawLineBegin().y() + 0.5);
			qDebug() << x << y;
			vW->setDrawLineEnd(QPoint(x, y));
			vW->clear();
			vW->drawLine(vW->getDrawLineBegin(), vW->getDrawLineEnd(), globalColor, ui->comboBoxLineAlg->currentIndex());
		}
		else if (ui->toolButtonDrawPolygon->isChecked()) {
			QVector <QPoint> scaledPoints;
			scaledPoints.append(vW->getDrawPolygonPoints()[0]);
			for (int i = 1; i < vW->getDrawPolygonPoints().length(); i++) {
				int x = static_cast<int>((vW->getDrawPolygonPoints()[i].x() - vW->getDrawPolygonPoints()[0].x()) * value + vW->getDrawPolygonPoints()[0].x() + 0.5);
				int y = static_cast<int>((vW->getDrawPolygonPoints()[i].y() - vW->getDrawPolygonPoints()[0].y()) * value + vW->getDrawPolygonPoints()[0].y() + 0.5);
				QPoint point(x,y);
				scaledPoints.append(point);
			}
			qDebug() << vW->getDrawPolygonPoints().length();
			vW->setDrawPolygonPoints(scaledPoints);
			vW->clear();
			vW->drawPolygon(vW->getDrawPolygonPoints(), globalColor, ui->comboBoxLineAlg->currentIndex(), ui->comboBoxFillingAlg->currentIndex());
		}
		vW->update();
	}
}

//ModelViewer Events
void ModelViewer::closeEvent(QCloseEvent* event)
{
	if (QMessageBox::Yes == QMessageBox::question(this, "Close Confirmation", "Are you sure you want to exit?", QMessageBox::Yes | QMessageBox::No))
	{
		event->accept();
	}
	else {
		event->ignore();
	}
}

//Image functions
bool ModelViewer::openImage(QString filename)
{
	QImage loadedImg(filename);
	if (!loadedImg.isNull()) {
		return vW->setImage(loadedImg);
	}
	return false;
}
bool ModelViewer::saveImage(QString filename)
{
	QFileInfo fi(filename);
	QString extension = fi.completeSuffix();

	QImage* img = vW->getImage();
	return img->save(filename, extension.toStdString().c_str());
}

//Table widget
void ModelViewer::objectTableWidgetUpdate() {
	ui->tableWidgetObjectList->clearContents();
	ui->tableWidgetObjectList->setRowCount(0);
	QList<Object2D> objects = object_map.values();
	std::sort(objects.begin(), objects.end(), [](const Object2D& object1, const Object2D& object2) {
		return object1.layer_height < object2.layer_height;
		});
	for (const Object2D& object : objects) {
		if (object.type.isEmpty()) {
			object_map.remove(object.name);
			continue;
		}
		int current_row = ui->tableWidgetObjectList->rowCount();
		ui->tableWidgetObjectList->insertRow(current_row);
		QTableWidgetItem* layer = new QTableWidgetItem(QString::number(object.layer_height));
		layer->setFlags(layer->flags() & ~Qt::ItemIsEditable);
		layer->setTextAlignment(Qt::AlignCenter);
		ui->tableWidgetObjectList->setItem(current_row, 0, layer);
		
		QTableWidgetItem* name = new QTableWidgetItem(object.name);
		name->setFlags(layer->flags() & ~Qt::ItemIsEditable);
		name->setTextAlignment(Qt::AlignCenter);
		ui->tableWidgetObjectList->setItem(current_row, 1, name);
		QTableWidgetItem* type = new QTableWidgetItem(object.type);
		type->setFlags(type->flags() & ~Qt::ItemIsEditable);
		type->setTextAlignment(Qt::AlignCenter);
		ui->tableWidgetObjectList->setItem(current_row, 2, type);
		QTableWidgetItem* color = new QTableWidgetItem();
		color->setFlags(color->flags() & ~Qt::ItemIsEditable);
		ui->tableWidgetObjectList->setItem(current_row, 3, color);
		if (object.color_filling != Qt::white) {
			ui->tableWidgetObjectList->item(current_row, 3)->setBackground(object.color_filling);
		}
		else {
			ui->tableWidgetObjectList->item(current_row, 3)->setBackground(object.color_outline);
		}
		if (current_object.name == object.name) {
			for (int i = 0; i < 3; i++) {
				if (ui->tableWidgetObjectList->item(current_row, i)) {
					ui->tableWidgetObjectList->item(current_row, i)->setBackground(Qt::gray);
				}
			}
		}
	}
}
void ModelViewer::on_tableWidgetObjectList_customContextMenuRequested(const QPoint& pos) {
	QTableWidgetItem* rightClickedItem = ui->tableWidgetObjectList->itemAt(pos);
	tableWidgetContextMenu = new QMenu(this);
	QAction* select_action = new QAction("select");
	QAction* delele_action = new QAction("delete");
	QAction* move_up_action = new QAction("move up");
	QAction* move_down_action = new QAction("move down");
	QAction* change_color_action = new QAction("change color");

	tableWidgetContextMenu->addAction(select_action);
	tableWidgetContextMenu->addAction(delele_action);
	tableWidgetContextMenu->addAction(move_up_action);
	tableWidgetContextMenu->addAction(move_down_action);
	tableWidgetContextMenu->addAction(change_color_action);
	//Select item
	connect(select_action, &QAction::triggered, this, [this, rightClickedItem]() {
		QString object_name = ui->tableWidgetObjectList->item(rightClickedItem->row(), 1)->text();
		current_object = object_map[object_name];
		objectTableWidgetUpdate();
		});
	//Delete item
	connect(delele_action, &QAction::triggered, this, [this, rightClickedItem]() {
		QString object_name = ui->tableWidgetObjectList->item(rightClickedItem->row(), 1)->text();
		object_map.remove(object_name);
		for (int i = rightClickedItem->row() + 1; i < ui->tableWidgetObjectList->rowCount(); i++) {
			object_name = ui->tableWidgetObjectList->item(i, 1)->text();
			object_map[object_name].layer_height--;
		}
		if (current_object.name == ui->tableWidgetObjectList->item(rightClickedItem->row(), 1)->text()) {
			if (rightClickedItem->row() == 0 ) {
				if (ui->tableWidgetObjectList->item(rightClickedItem->row() + 1, 1)) {
					current_object = object_map[ui->tableWidgetObjectList->item(rightClickedItem->row() + 1, 1)->text()];
				}
				else {
					current_object = Object2D();
				}
			}
			else{
				current_object = object_map[ui->tableWidgetObjectList->item(rightClickedItem->row() - 1, 1)->text()];
			}
		}
		ui->tableWidgetObjectList->removeRow(rightClickedItem->row());
		objectTableWidgetUpdate();
		vW->clear();
		vW->drawObjects2D(object_map);
		});
	//Move up
	connect(move_up_action, &QAction::triggered, this, [this, rightClickedItem]() {
		if (rightClickedItem->row() != 0) {
			QString swapped_object_name = ui->tableWidgetObjectList->item(rightClickedItem->row() - 1, 1)->text();
			QString object_name = ui->tableWidgetObjectList->item(rightClickedItem->row(), 1)->text();
			object_map[object_name].layer_height--;
			object_map[swapped_object_name].layer_height++;
			objectTableWidgetUpdate();
			vW->clear();
			vW->drawObjects2D(object_map);
		}
		});
	//Move down
	connect(move_down_action, &QAction::triggered, this, [this, rightClickedItem]() {
		if (rightClickedItem->row() != ui->tableWidgetObjectList->rowCount() - 1) {
			if (ui->tableWidgetObjectList->item(rightClickedItem->row() + 1, 1)) {
				QString swapped_object_name = ui->tableWidgetObjectList->item(rightClickedItem->row() + 1, 1)->text();
				object_map[swapped_object_name].layer_height--;
				QString object_name = ui->tableWidgetObjectList->item(rightClickedItem->row(), 1)->text();
				object_map[object_name].layer_height++;
			}
			objectTableWidgetUpdate();
			vW->clear();
			vW->drawObjects2D(object_map);
		}
		});
	//Change color
	connect(change_color_action, &QAction::triggered, this, [this, rightClickedItem]() {
		QColor color = QColorDialog::getColor(Qt::white, this, "Change Color");
		if (color.isValid()) {
			QString object_name = ui->tableWidgetObjectList->item(rightClickedItem->row(), 1)->text();
			object_map[object_name].color_outline = color;
			object_map[object_name].color_filling = color;
			if (object_map[object_name].name == current_object.name) {
				current_object.color_filling = color;
				current_object.color_outline = color;
			}
			vW->drawObjects2D(object_map);
			vW->clear();
			objectTableWidgetUpdate();
		}
		});
	tableWidgetContextMenu->popup(ui->tableWidgetObjectList->viewport()->mapToGlobal(pos));
}
void ModelViewer::on_tableWidgetObjectList_cellDoubleClicked(int row, int column) {
	QString object_name = ui->tableWidgetObjectList->item(row, 1)->text();
	current_object = object_map[object_name];
	objectTableWidgetUpdate();
}

//Save/Load 2d scene state
void ModelViewer::saveSceneState(QString filename) {
	QFile file(filename);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qDebug() << "Could not open file for writing:" << file.errorString();
		return;
	}
	QTextStream out(&file);
	out << "MODELVIEWER 2D SCENE FORMAT\n";
	for (const Object2D& object : object_map.values()) {
		out << "TYPE:" << object.type << "\n";
		out << "NAME:" << object.name << "\n";
		out << "OUTLINE_COLOR:" << object.color_outline.red() << ":" << object.color_outline.green() << ":" << object.color_outline.blue() << "\n";
		out << "FILLING_COLOR:" << object.color_filling.red() << ":" << object.color_filling.green() << ":" << object.color_filling.blue() << "\n";
		out << "FILLING_ALG:" << object.filling_alg << "\n";
		out << "CURVE_TYPE:" << object.curve_type << "\n";
		out << "LAYER:" << object.layer_height << "\n";
		out << "POINTS:" << object.points.length() << "\n";
		if (object.type == "curve") {
			for (const QPair<QPoint, QPoint>& points : object.curve_points) {
				out << points.first.x() << ":" << points.first.y() << ":" << points.second.x() << ":" << points.second.y() << "\n";
			}
		}
		else {
			for (int i = 0; i < object.points.length(); i++) {
				out << object.points[i].x() << ":" << object.points[i].y() << "\n";
			}
		}
		out << "\n";
	}
}
void ModelViewer::loadSceneState(QString filename) {
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Could not open file for writing:" << file.errorString();
		return;
	}
	QMap<QString, Object2D> object_map_temporary = QMap<QString, Object2D>();
	bool correct_format = true;
	QTextStream in(&file);
	if (in.readLine().trimmed() != "MODELVIEWER 2D SCENE FORMAT") {
		correct_format = false;
	}
	while (!in.atEnd() && correct_format == true) {
		//TYPE
		QString file_line = in.readLine();
		QList<QString> file_data = file_line.split(":");
		if (file_data.length() != 2 || file_data[0] != "TYPE") {
			correct_format = false;
		}
		QString object_type = file_data[1].trimmed().toLower();
		//NAME
		file_line = in.readLine();
		file_data = file_line.split(":");
		if (file_data.length() != 2 || file_data[0] != "NAME") {
			correct_format = false;
		}
		QString object_name = file_data[1].trimmed();
		//OUTLINE_COLOR
		file_line = in.readLine();
		file_data = file_line.split(":");
		if (file_data.length() != 4 || file_data[0] != "OUTLINE_COLOR") {
			correct_format = false;
		}
		correct_format = !std::all_of(file_data.begin() + 1, file_data.end(), [](QString string) {
			bool ok;
			string.toInt(&ok);
			return ok;
			});
		QColor object_outline_color = QColor(file_data[1].toInt(), file_data[2].toInt(), file_data[3].toInt());
		//FILLING_COLOR
		file_line = in.readLine();
		file_data = file_line.split(":");
		if (file_data.length() != 4 || file_data[0] != "FILLING_COLOR") {
			correct_format = false;
		}
		correct_format = !std::all_of(file_data.begin() + 1, file_data.end(), [](QString string) {
			bool ok;
			string.toInt(&ok);
			return ok;
			});
		QColor object_filling_color = QColor(file_data[1].toInt(), file_data[2].toInt(), file_data[3].toInt());
		//FILLING_ALG
		file_line = in.readLine();
		file_data = file_line.split(":");
		if (file_data.length() != 2 || file_data[0] != "FILLING_ALG") {
			correct_format = false;
		}
		int object_filling_alg = file_data[1].toInt(&correct_format);
		//CURVE_TYPE
		file_line = in.readLine();
		file_data = file_line.split(":");
		if (file_data.length() != 2 || file_data[0] != "CURVE_TYPE") {
			correct_format = false;
		}
		int object_curve_type = file_data[1].toInt(&correct_format);
		//LAYER
		file_line = in.readLine();
		file_data = file_line.split(":");
		if (file_data.length() != 2 || file_data[0] != "LAYER") {
			correct_format = false;
		}
		int object_layer = file_data[1].toInt(&correct_format);
		//POINTS
		file_line = in.readLine();
		file_data = file_line.split(":");
		if (file_data.length() != 2 || file_data[0] != "POINTS") {
			correct_format = false;
		}
		int object_points_count = file_data[1].toInt(&correct_format);
		if ((object_type == "line" || object_type == "circle") && object_points_count != 2) {
			correct_format = false;
		}
		QVector<QPair<QPoint, QPoint>> object_curve_points = QVector<QPair<QPoint, QPoint>>();
		QVector<QPoint> object_points = QVector<QPoint>();
		for (int i = 0; i < object_points_count; i++) {
			file_data = in.readLine().split(":");
			if (object_type == "curve" && file_data.length() == 4) {
				QPoint start(file_data[0].toInt(&correct_format), file_data[1].toInt(&correct_format));
				if (!file_data[2].isEmpty() && !file_data[3].isEmpty()) {
					QPoint end(file_data[2].toInt(&correct_format), file_data[3].toInt(&correct_format));
					object_curve_points.append({ start,end });
				}
				else {
					object_curve_points.append({ start,QPoint() });
				}
			}
			else if (file_data.length() == 2) {
				QPoint point(file_data[0].toInt(&correct_format), file_data[1].toInt(&correct_format));
				object_points.append(point);
			}
		}
		file_line = in.readLine();
		if (!file_line.trimmed().isEmpty()) {
			correct_format = false;
			break;
		}
		Object2D object = Object2D();
		object.type = object_type;
		object.name = object_name;
		object.color_outline = object_outline_color;
		object.color_filling = object_filling_color;
		object.filling_alg = object_filling_alg;
		object.curve_type = object_curve_type;
		object.layer_height = object_layer;
		object.curve_points = object_curve_points;
		object.points = object_points;

		object_map_temporary.insert(object.name, object);

	}
	if (!correct_format) {
		QMessageBox::warning(nullptr, "Warning", "Wrong format file!", QMessageBox::Ok);
	}
	else {
		object_map = object_map_temporary;
		vW->drawObjects2D(object_map);
		current_object = object_map.values().first();
		objectTableWidgetUpdate();
	}
	qDebug() << "file loaded";
}

//Slots
//2D Slots
//Draw
void ModelViewer::on_toolButtonDrawLine_clicked() {
	vW->setDrawLineBegin(QPoint());
	vW->setDrawLineEnd(QPoint());

	ui->toolButtonDrawCircle->setChecked(false);
	ui->toolButtonDrawPolygon->setChecked(false);
	ui->toolButtonEditPosition->setChecked(false);
	ui->toolButtonDrawCurve->setChecked(false);
	ui->spinBoxRotation->setEnabled(false);
}
void ModelViewer::on_toolButtonDrawCircle_clicked() {
	vW->setDrawLineBegin(QPoint());
	vW->setDrawLineEnd(QPoint());

	ui->toolButtonDrawLine->setChecked(false);
	ui->toolButtonDrawPolygon->setChecked(false);
	ui->toolButtonEditPosition->setChecked(false);
	ui->toolButtonDrawCurve->setChecked(false);
	ui->spinBoxRotation->setEnabled(false);
}
void ModelViewer::on_toolButtonDrawPolygon_clicked() {
	vW->getDrawPolygonPoints().clear();

	ui->toolButtonDrawCircle->setChecked(false);
	ui->toolButtonDrawLine->setChecked(false);
	ui->spinBoxRotation->setEnabled(false);
	ui->toolButtonDrawCurve->setChecked(false);
	ui->toolButtonEditPosition->setChecked(false);
}
void ModelViewer::on_toolButtonDrawCurve_clicked() {
	ui->toolButtonDrawLine->setChecked(false);
	ui->toolButtonDrawPolygon->setChecked(false);
	ui->toolButtonEditPosition->setChecked(false);
	ui->toolButtonDrawCircle->setChecked(false);
	ui->spinBoxRotation->setEnabled(false);
}
//Edit
void ModelViewer::on_toolButtonEditPosition_clicked() {
	ui->toolButtonEditRotation->setChecked(false);
	ui->toolButtonEditShear->setChecked(false);
	ui->spinBoxRotation->setEnabled(false);
	ui->toolButtonEditScale->setChecked(false);
	ui->spinBoxScaleX->setEnabled(false);
	ui->spinBoxScaleY->setEnabled(false);
	ui->spinBoxShearFactor->setEnabled(false);

	ui->toolButtonDrawCircle->setChecked(false);
	ui->toolButtonDrawLine->setChecked(false);
	ui->toolButtonDrawPolygon->setChecked(false);
	ui->toolButtonDrawCurve->setChecked(false);
}

//Rotation
void ModelViewer::on_toolButtonEditRotation_clicked() {
	ui->toolButtonEditPosition->setChecked(false);
	ui->toolButtonEditScale->setChecked(false);
	ui->toolButtonEditShear->setChecked(false);
	ui->spinBoxScaleX->setEnabled(false);
	ui->spinBoxScaleY->setEnabled(false);
	ui->spinBoxShearFactor->setEnabled(false);
	if (ui->toolButtonEditRotation->isChecked()) {
		ui->spinBoxRotation->setEnabled(true);
	}
	else {
		ui->spinBoxRotation->setEnabled(false);
	}
	ui->toolButtonDrawCircle->setChecked(false);
	ui->toolButtonDrawLine->setChecked(false);
	ui->toolButtonDrawPolygon->setChecked(false);
	ui->toolButtonDrawCurve->setChecked(false);
}
void ModelViewer::on_spinBoxRotation_valueChanged(int value) {
	double valueRadian = value * M_PI / 180;
	if (current_object.type == "line") {

		QPoint delta = current_object.points[1] - current_object.points[0];
		int x = static_cast<int>(delta.x() * cos(valueRadian) + delta.y() * sin(valueRadian)) + current_object.points[0].x();
		int y = static_cast<int>(-delta.x() * sin(valueRadian) + delta.y() * cos(valueRadian)) + current_object.points[0].y();
		object_map[current_object.name].points[1] = QPoint(x, y);
		vW->clear();
		vW->drawObjects2D(object_map);
	}
	else if (current_object.type == "polygon") {
		QVector<QPoint> rotatedPoints;
		rotatedPoints.append(current_object.points[0]);
		for (int i = 1; i < current_object.points.length(); i++) {
			QPoint delta = current_object.points[i] - current_object.points[0];
			int x = static_cast<int>(delta.x() * cos(valueRadian) + delta.y() * sin(valueRadian)) + current_object.points[0].x();
			int y = static_cast<int>(-delta.x() * sin(valueRadian) + delta.y() * cos(valueRadian)) + current_object.points[0].y();
			rotatedPoints.append(QPoint(x, y));
		}
		object_map[current_object.name].points = rotatedPoints;
		vW->clear();
		vW->drawObjects2D(object_map);
	}
	vW->update();
}
void ModelViewer::on_spinBoxRotation_editingFinished() {
	int value = ui->spinBoxRotation->value();
	double valueRadian = value * M_PI / 180;
	if (current_object.type == "line") {
		QPoint delta = current_object.points[1] - current_object.points[0];
		int x = static_cast<int>(delta.x() * cos(valueRadian) + delta.y() * sin(valueRadian)) + current_object.points[0].x();
		int y = static_cast<int>(-delta.x() * sin(valueRadian) + delta.y() * cos(valueRadian)) + current_object.points[0].y();
		current_object.points[1] = QPoint(x, y);
		object_map[current_object.name] = current_object;
	}
	else if (current_object.type == "polygon") {
		QVector<QPoint> rotatedPoints;
		rotatedPoints.append(current_object.points[0]);
		for (int i = 1; i < current_object.points.length(); i++) {
			QPoint delta = current_object.points[i] - current_object.points[0];
			int x = static_cast<int>(delta.x() * cos(valueRadian) + delta.y() * sin(valueRadian)) + current_object.points[0].x();
			int y = static_cast<int>(-delta.x() * sin(valueRadian) + delta.y() * cos(valueRadian)) + current_object.points[0].y();
			rotatedPoints.append(QPoint(x, y));
		}
		current_object.points = rotatedPoints;
		object_map[current_object.name] = current_object;
	}
	ui->spinBoxRotation->setValue(0);
}

//Scale
void ModelViewer::on_toolButtonEditScale_clicked() {
	if (ui->toolButtonEditScale->isChecked()) {
		ui->spinBoxScaleX->setEnabled(true);
		ui->spinBoxScaleY->setEnabled(true);
	}
	else {
		ui->spinBoxScaleX->setEnabled(false);
		ui->spinBoxScaleY->setEnabled(false);
	}

	ui->spinBoxRotation->setEnabled(false);
	ui->spinBoxShearFactor->setEnabled(false);
	ui->toolButtonEditPosition->setChecked(false);
	ui->toolButtonEditRotation->setChecked(false);
	ui->toolButtonEditShear->setChecked(false);

	ui->toolButtonDrawCircle->setChecked(false);
	ui->toolButtonDrawLine->setChecked(false);
	ui->toolButtonDrawPolygon->setChecked(false);
	ui->toolButtonDrawCurve->setChecked(false);
}
void ModelViewer::on_spinBoxScaleX_valueChanged(double value) {
	if (current_object.type == "line") {
		int x = static_cast<int>((current_object.points[1].x() - current_object.points[0].x()) * value + current_object.points[0].x());
		object_map[current_object.name].points[1] = QPoint(x, current_object.points[1].y());
		vW->clear();
		vW->drawObjects2D(object_map);
	}
	else if (current_object.type == "polygon") {
		QVector <QPoint> scaledPoints;
		scaledPoints.append(current_object.points[0]);
		for (int i = 1; i < current_object.points.length(); i++) {
			int x = static_cast<int>((current_object.points[i].x() - current_object.points[0].x()) * value + current_object.points[0].x());
			QPoint point(x, current_object.points[i].y());
			scaledPoints.append(point);
		}
		vW->clear();
		object_map[current_object.name].points = scaledPoints;
		vW->drawObjects2D(object_map);
	}
	vW->update();
}
void ModelViewer::on_spinBoxScaleY_valueChanged(double value) {
	if (current_object.type == "line") {
		int y = static_cast<int>((current_object.points[1].y() - current_object.points[0].y()) * value + current_object.points[0].y());
		object_map[current_object.name].points[1] = QPoint(current_object.points[1].x(), y);
		vW->clear();
		vW->drawObjects2D(object_map);
	}
	else if (current_object.type == "polygon") {
		QVector <QPoint> scaledPoints;
		scaledPoints.append(current_object.points[0]);
		for (int i = 0; i < current_object.points.length(); i++) {
			int y = static_cast<int>((current_object.points[i].y() - current_object.points[0].y()) * value + current_object.points[0].y());
			QPoint point( current_object.points[i].x(),y);
			scaledPoints.append(point);
		}
		object_map[current_object.name].points = scaledPoints;
		vW->clear();
		vW->drawObjects2D(object_map);
	}
	vW->update();
}
void ModelViewer::on_spinBoxScaleX_editingFinished() {
	double value = ui->spinBoxScaleX->value();
	if (current_object.type == "line") {
		int x = static_cast<int>((current_object.points[1].x() - current_object.points[0].x()) * value + current_object.points[0].x());
		current_object.points[1] = QPoint(x, current_object.points[1].y());
		object_map[current_object.name] = current_object;
	}
	else if (current_object.type == "polygon") {
		QVector <QPoint> scaledPoints;
		scaledPoints.append(current_object.points[0]);
		for (int i = 1; i < current_object.points.length(); i++) {
			int x = static_cast<int>((current_object.points[i].x() - current_object.points[0].x()) * value + current_object.points[0].x());
			QPoint point(x, current_object.points[i].y());
			scaledPoints.append(point);
		}
		current_object.points = scaledPoints;
		object_map[current_object.name].points = scaledPoints;
	}
	ui->spinBoxScaleX->setValue(1);
}
void ModelViewer::on_spinBoxScaleY_editingFinished() {
	double value = ui->spinBoxScaleY->value();
	if (current_object.type == "line") {
		int y = static_cast<int>((current_object.points[1].y() - current_object.points[0].y()) * value + current_object.points[0].y());
		current_object.points[1] = QPoint(current_object.points[1].x(), y);
		object_map[current_object.name] = current_object;
	}
	else if (current_object.type == "polygon") {
		QVector <QPoint> scaledPoints;
		scaledPoints.append(current_object.points[0]);
		for (int i = 0; i < current_object.points.length(); i++) {
			int y = static_cast<int>((current_object.points[i].y() - current_object.points[0].y()) * value + current_object.points[0].y());
			QPoint point(current_object.points[i].x(), y);
			scaledPoints.append(point);
		}
		current_object.points = scaledPoints;
		object_map[current_object.name] = current_object;
	}
	ui->spinBoxScaleY->setValue(1);
}
//Shear
void ModelViewer::on_toolButtonEditShear_clicked() {
	if (ui->toolButtonEditShear->isChecked()) {
		ui->spinBoxShearFactor->setEnabled(true);
	}
	else {
		ui->spinBoxShearFactor->setEnabled(false);
	}
	ui->toolButtonEditRotation->setChecked(false);
	ui->toolButtonEditPosition->setChecked(false);
	ui->toolButtonEditScale->setChecked(false);
	ui->spinBoxScaleX->setEnabled(false);
	ui->spinBoxScaleY->setEnabled(false);
	ui->spinBoxRotation->setEnabled(false);

	ui->toolButtonDrawCircle->setChecked(false);
	ui->toolButtonDrawLine->setChecked(false);
	ui->toolButtonDrawPolygon->setChecked(false);
	ui->toolButtonDrawCurve->setChecked(false);
}
void ModelViewer::on_spinBoxShearFactor_valueChanged(double value) {
	if (ui->toolButtonDrawLine->isChecked()) {
		int x = vW->getDrawLineEnd().x() + vW->getDrawLineEnd().y() * value;
		vW->clear();
		vW->drawLine(vW->getDrawLineBegin(), QPoint(x, vW->getDrawLineEnd().y()), globalColor, ui->comboBoxLineAlg->currentIndex());
	}
	else if (ui->toolButtonDrawPolygon->isChecked()) {
		QVector<QPoint> shearedPoints;
		shearedPoints.append(vW->getDrawPolygonPoints().first());
		for (int i = 1; i < vW->getDrawPolygonPoints().length(); i++) {
			int x = vW->getDrawPolygonPoints()[i].x() + vW->getDrawPolygonPoints()[i].y() * value;
			shearedPoints.append(QPoint(x, vW->getDrawPolygonPoints()[i].y()));
		}
		vW->clear();
		vW->drawPolygon(shearedPoints, globalColor, ui->comboBoxLineAlg->currentIndex(), ui->comboBoxFillingAlg->currentIndex());
	}
	vW->update();
}
void ModelViewer::on_spinBoxShearFactor_editingFinished() {
	double value = ui->spinBoxShearFactor->value();
	if (ui->toolButtonDrawLine->isChecked()) {
		int x = vW->getDrawLineEnd().x() + vW->getDrawLineEnd().y() * value;
		vW->setDrawLineEnd(QPoint(x, vW->getDrawLineEnd().y()));
	}
	else if (ui->toolButtonDrawPolygon->isChecked()) {
		QVector<QPoint> shearedPoints;
		shearedPoints.append(vW->getDrawPolygonPoints()[0]);
		for (int i = 1; i < vW->getDrawPolygonPoints().length(); i++) {
			int x = vW->getDrawPolygonPoints()[i].x() + vW->getDrawPolygonPoints()[i].y() * value;
			shearedPoints.append(QPoint(x, vW->getDrawPolygonPoints()[i].y()));
		}
		vW->setDrawPolygonPoints(shearedPoints);
	}
	ui->spinBoxShearFactor->setValue(0);
}
//Symmetry
void ModelViewer::on_toolButtonSymmetry_clicked() {
	if (ui->toolButtonDrawLine->isChecked()) {
		QPoint A = QPoint(vW->getDrawLineBegin().x(), 0);
		QPoint B = QPoint(vW->getDrawLineBegin().x(), vW->getImgHeight());
		QPoint vectorAB = B - A;
		QPoint normAB = QPoint(vectorAB.y(), -vectorAB.x());
		int a = normAB.x();
		int b = normAB.y();		
		int c = -a * A.x() - b * A.y();
		int newX = static_cast<int>(vW->getDrawLineEnd().x() - 2 * a * (static_cast<double>(a * vW->getDrawLineEnd().x() + b * vW->getDrawLineEnd().y() + c) / (a * a + b * b)));
		int newY = static_cast<int>(vW->getDrawLineEnd().y() - 2 * b * (static_cast<double>(a * vW->getDrawLineEnd().x() + b * vW->getDrawLineEnd().y() + c) / (a * a + b * b)));
		vW->setDrawLineEnd(QPoint(newX, newY));
		vW->clear();
		vW->drawLine(vW->getDrawLineBegin(), vW->getDrawLineEnd(), globalColor, ui->comboBoxLineAlg->currentIndex());
		vW->update();
	}
	else if (ui->toolButtonDrawPolygon->isChecked()) {
		QPoint A = vW->getDrawPolygonPoints()[vW->getDrawPolygonPoints().length() - 1];
		QPoint B = vW->getDrawPolygonPoints()[0];
		QPoint vectorAB = B - A;
		QPoint normAB = QPoint(vectorAB.y(), -vectorAB.x());
		int a = normAB.x();
		int b = normAB.y();
		int c = -a * A.x() - b * A.y();
		QVector<QPoint> transformedPoints;
		transformedPoints.append(B);
		for (int i = 1; i < vW->getDrawPolygonPoints().length() - 1; i++) {
			int newX = static_cast<int>(vW->getDrawPolygonPoints()[i].x() - 2 * a * (static_cast<double>(a * vW->getDrawPolygonPoints()[i].x() + b * vW->getDrawPolygonPoints()[i].y() + c) / (a * a + b * b)));
			int newY = static_cast<int>(vW->getDrawPolygonPoints()[i].y() - 2 * b * (static_cast<double>(a * vW->getDrawPolygonPoints()[i].x() + b * vW->getDrawPolygonPoints()[i].y() + c) / (a * a + b * b)));
			transformedPoints.append(QPoint(newX, newY));
		}
		transformedPoints.append(A);
		vW->setDrawPolygonPoints(transformedPoints);
		vW->clear();
		vW->drawPolygon(transformedPoints, globalColor, ui->comboBoxLineAlg->currentIndex(), ui->comboBoxFillingAlg->currentIndex());
		vW->update();
	}
}
//Actions
void ModelViewer::on_actionOpen_triggered()
{	
	QString folder = settings.value("folder_img_load_path", "").toString();
	QString fileFilter;
	if (isIn3dMode) {
		fileFilter = "VTK data (*.vtk);;All files (*)";
	}
	else {
		fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm .*xbm .* xpm);;All files (*)";
	}
	QString fileName = QFileDialog::getOpenFileName(this, "Load image", folder, fileFilter);
	if (fileName.isEmpty()) { return; }

	QFileInfo fi(fileName);
	settings.setValue("folder_img_load_path", fi.absoluteDir().absolutePath());
	if (isIn3dMode) {
		Object_H_edge object = loadPolygonsVTK(fileName);
		if (object == Object_H_edge()) {
			msgBox.setText("Unable to open object.");
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.exec();
		}
		else {
			vW->setCurrentObject(object);
			on_action3D_triggered();
			vW->setDrawObjectActivated(true);
			vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), nullptr);
		}
	}
	else if (!openImage(fileName)) {
		msgBox.setText("Unable to open image.");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.exec();
	}
}
void ModelViewer::on_actionSave_as_triggered()
{
	QString folder = settings.value("folder_img_save_path", "").toString();

	QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm .*xbm .* xpm);;All files (*)";
	QString fileName = QFileDialog::getSaveFileName(this, "Save image", folder, fileFilter);
	if (!fileName.isEmpty()) {
		QFileInfo fi(fileName);
		settings.setValue("folder_img_save_path", fi.absoluteDir().absolutePath());

		if (!saveImage(fileName)) {
			msgBox.setText("Unable to save image.");
			msgBox.setIcon(QMessageBox::Warning);
		}
		else {
			msgBox.setText(QString("File %1 saved.").arg(fileName));
			msgBox.setIcon(QMessageBox::Information);
		}
		msgBox.exec();
	}
}
void ModelViewer::on_actionClear_triggered()
{
	ui->toolButtonDrawCircle->setEnabled(true);
	ui->toolButtonDrawLine->setEnabled(true);
	ui->toolButtonDrawPolygon->setEnabled(true);
	ui->toolButtonDrawCurve->setEnabled(true);
	ui->comboBoxCurveAlg->setEnabled(true);
	ui->toolButtonDrawCircle->setChecked(false);
	ui->toolButtonDrawLine->setChecked(false);
	ui->toolButtonDrawPolygon->setChecked(false);
	ui->toolButtonDrawCurve->setChecked(false);
	ui->toolButtonEditPosition->setChecked(false);
	ui->toolButtonEditRotation->setChecked(false);
	ui->spinBoxRotation->setValue(0);
	ui->spinBoxRotation->setEnabled(false);
	ui->spinBoxScaleX->setValue(1);
	ui->spinBoxScaleY->setValue(1);
	ui->spinBoxScaleX->setEnabled(false);
	ui->spinBoxScaleY->setEnabled(false);
	vW->clear();
	vW->getDrawPolygonPoints().clear();
	vW->setDrawLineBegin(QPoint());
	vW->setDrawLineEnd(QPoint());
	vW->getDrawCurveMasterPoints().clear();
	object_map.clear();
	current_object = Object2D();
}
void ModelViewer::on_actionExit_triggered()
{
	this->close();
}
void ModelViewer::on_actionSave_state_triggered() {
	QString filename = QFileDialog::getSaveFileName(this, "save state", "/save/modelviewer_project", "Text files(*.txt)");
	if (!filename.isEmpty()) {
		saveSceneState(filename);
	}
}
void ModelViewer::on_actionLoad_state_triggered(){
	QString filename = QFileDialog::getOpenFileName(this, "laod state",QDir::currentPath() + "/save/", "Text files(*.txt)");
	if (!filename.isEmpty()) {
		loadSceneState(filename);
	}
}

void ModelViewer::on_pushButtonSetColor_clicked()
{
	QColor newColor = QColorDialog::getColor(globalColor, this);
	if (newColor.isValid()) {
		QString style_sheet = QString("background-color: #%1;").arg(newColor.rgba(), 0, 16);
		ui->pushButtonSetColor->setStyleSheet(style_sheet);
		globalColor = newColor;
	}
}
void ModelViewer::on_action2D_triggered() {
	ui->dockWidget_4->setHidden(true);
	ui->dockWidget->setHidden(false);
	isIn3dMode = false;
	vW->clear();
}
void ModelViewer::on_action3D_triggered() {
	ui->dockWidget_4->setHidden(false);
	ui->dockWidget->setHidden(true);
	isIn3dMode = true;

	ui->checkBoxLightSettings->setChecked(false);
	ui->groupBoxLightSettings->setEnabled(false);
	ui->groupBoxLightSettings->setHidden(true);

	vW->clear();
}

//3D slots
void ModelViewer::on_checkBoxLightSettings_stateChanged(int state) {
	if (ui->checkBoxLightSettings->isChecked()) {
		ui->groupBoxLightSettings->setEnabled(true);
		ui->groupBoxLightSettings->setHidden(false);

		globalLightSettings = new LightSettings();
		//Setting reflexion/diffusion/ambient coefficients default 0,0,0
		globalLightSettings->rd = 0.5;
		globalLightSettings->rs = 0.5;
		globalLightSettings->ra = 0.5;

		ui->horizontalSliderRsCoefficient->setValue(50);
		ui->horizontalSliderRdCoefficient->setValue(50);
		ui->horizontalSliderRaCoefficient->setValue(50);


		globalLightSettings->h = ui->horizontalSliderLightH->value();
		//Setting light position default (500,500,500)
		globalLightSettings->lightPosition = Vertex(500, 500, 100);
		ui->spinBoxLightPosX->setValue(500);
		ui->spinBoxLightPosY->setValue(500);
		ui->spinBoxLightPosZ->setValue(100);
		//Setting light intensity default (white)
		globalLightSettings->lightIntesity = QColor(255, 255, 255);
		ui->spinBoxLightIntensityRed->setValue(255);
		ui->spinBoxLightIntensityGreen->setValue(255);
		ui->spinBoxLightIntensityBlue->setValue(255);
		//Setting light ambient intensity default (white)
		globalLightSettings->lightIntesityAmbient = QColor(0, 0, 255);
		ui->spinBoxLightIntensityAmbientRed->setValue(0);
		ui->spinBoxLightIntensityAmbientGreen->setValue(0);
		ui->spinBoxLightIntensityAmbientBlue->setValue(255);
	}
	else {
		ui->groupBoxLightSettings->setEnabled(false);
		ui->groupBoxLightSettings->setHidden(true);

		globalLightSettings = nullptr;
	}
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_checkBoxCameraSettings_stateChanged(int state) {
	if (ui->checkBoxCameraSettings->isChecked()) {
		ui->groupBoxCameraSettings->setEnabled(true);
		ui->groupBoxCameraSettings->setHidden(false);
	}
	else {
		ui->groupBoxCameraSettings->setEnabled(false);
		ui->groupBoxCameraSettings->setHidden(true);
	}
}
void ModelViewer::on_checkBoxVtkOutput_stateChanged(int state) {
	if (ui->checkBoxVtkOutput->isChecked()) {
		ui->groupBoxVtkOutput->setEnabled(true);
		ui->groupBoxVtkOutput->setHidden(false);
	}
	else {
		ui->groupBoxVtkOutput->setEnabled(false);
		ui->groupBoxVtkOutput->setHidden(true);
	}
}

//Create VTK
void ModelViewer::on_pushButtonCreateVTK_clicked() {
	QString filename = QFileDialog::getSaveFileName(this, "Save VTK file", QDir::currentPath(), "VTK Files (*.vtk)");
	// Create Cube VTK
	if (ui->comboBoxTypeCreateVTK->currentIndex() == 0 && !filename.isEmpty()) {
		createCubeVTK(ui->spinBoxEdgeLength->value(), filename);
	}
	else if (ui->comboBoxTypeCreateVTK->currentIndex() == 1 && !filename.isEmpty()) {
		createUvSphereVTK(ui->doubleSpinBoxSphereRadius->value(), ui->spinBoxLongLatCount->value(), ui->spinBoxLongLatCount->value(), filename, 0);
	}
}
void ModelViewer::on_comboBoxTypeCreateVTK_currentIndexChanged(int index) {
	if (index == 0) {
		ui->spinBoxLongLatCount->setEnabled(false);
		ui->doubleSpinBoxSphereRadius->setEnabled(false);
		ui->spinBoxEdgeLength->setEnabled(true);
	}
	else if (index == 1) {
		ui->spinBoxLongLatCount->setEnabled(true);
		ui->doubleSpinBoxSphereRadius->setEnabled(true);
		ui->spinBoxEdgeLength->setEnabled(false);
	}
}

void ModelViewer::on_horizontalSliderZenit_valueChanged(int value) {
	double radValue = value * M_PI / 180;
	vW->getProjectionPlane().setProjectionPlane(vW->getProjectionPlane().azimut, radValue);
	vW->clear();
	if (vW->getDrawObjectActivated()) {
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_horizontalSliderAzimut_valueChanged(int value) {
	double radValue = value * M_PI / 180;
	vW->getProjectionPlane().setProjectionPlane(radValue, vW->getProjectionPlane().zenit);
	vW->clear();
	if (vW->getDrawObjectActivated()) {
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}

}

void ModelViewer::on_comboBoxProjectionType_currentIndexChanged(int index) {
	if (index == 1) {
		ui->horizontalSliderCameraCoordZ->setEnabled(true);
	}
	else {
		ui->horizontalSliderCameraCoordZ->setEnabled(false);
	}
	if (vW->getDrawObjectActivated()) {
		vW->getCamera().position.z = ui->horizontalSliderCameraCoordZ->value();
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_horizontalSliderCameraCoordZ_valueChanged(int value) {
	vW->getCamera().position.z = value;
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}

//Light settings
void ModelViewer::on_comboBoxRepresentationType_currentIndexChanged(int index) {
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_comboBoxShadingAlg_currentIndexChanged(int index) {
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(),ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}

void ModelViewer::on_horizontalSliderRdCoefficient_valueChanged(int value) {
	// value corrected to interval [0,1]
	globalLightSettings->rd = value / 100.;
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
	qDebug() << "rd coef : " << globalLightSettings->rd;
}
void ModelViewer::on_horizontalSliderRsCoefficient_valueChanged(int value) {
	// value corrected to interval [0,1]
	globalLightSettings->rs = value / 100.;
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
	qDebug() << "rs coef : " << globalLightSettings->rs;
}
void ModelViewer::on_horizontalSliderRaCoefficient_valueChanged(int value) {
	// value corrected to interval [0,1]
	globalLightSettings->ra = value / 100.;
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
	qDebug() << "ra coef : " << globalLightSettings->ra;
}
void ModelViewer::on_horizontalSliderLightH_valueChanged(int value) {
	globalLightSettings->h = value;
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
	qDebug() << "H : " << globalLightSettings->h;
}

void ModelViewer::on_spinBoxLightPosX_valueChanged(int value){
	globalLightSettings->lightPosition.x = value;
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_spinBoxLightPosY_valueChanged(int value) {
	globalLightSettings->lightPosition.y = value;
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_spinBoxLightPosZ_valueChanged(int value) {
	globalLightSettings->lightPosition.z = value;
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}


void ModelViewer::on_spinBoxLightIntensityRed_valueChanged(int value) {
	globalLightSettings->lightIntesity = QColor(ui->spinBoxLightIntensityRed->value(), ui->spinBoxLightIntensityGreen->value(), ui->spinBoxLightIntensityBlue->value());
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_spinBoxLightIntensityGreen_valueChanged(int value) {
	globalLightSettings->lightIntesity = QColor(ui->spinBoxLightIntensityRed->value(), ui->spinBoxLightIntensityGreen->value(), ui->spinBoxLightIntensityBlue->value());
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_spinBoxLightIntensityBlue_valueChanged(int value) {
	globalLightSettings->lightIntesity = QColor(ui->spinBoxLightIntensityRed->value(), ui->spinBoxLightIntensityGreen->value(), ui->spinBoxLightIntensityBlue->value());
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}

void ModelViewer::on_spinBoxLightIntensityAmbientRed_valueChanged(int value) {
	globalLightSettings->lightIntesityAmbient = QColor(ui->spinBoxLightIntensityAmbientRed->value(), ui->spinBoxLightIntensityAmbientGreen->value(), ui->spinBoxLightIntensityAmbientBlue->value());
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_spinBoxLightIntensityAmbientGreen_valueChanged(int value) {
	globalLightSettings->lightIntesityAmbient = QColor(ui->spinBoxLightIntensityAmbientRed->value(), ui->spinBoxLightIntensityAmbientGreen->value(), ui->spinBoxLightIntensityAmbientBlue->value());
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}
void ModelViewer::on_spinBoxLightIntensityAmbientBlue_valueChanged(int value) {
	globalLightSettings->lightIntesityAmbient = QColor(ui->spinBoxLightIntensityAmbientRed->value(), ui->spinBoxLightIntensityAmbientGreen->value(), ui->spinBoxLightIntensityAmbientBlue->value());
	if (vW->getDrawObjectActivated()) {
		vW->clear();
		vW->drawObject(vW->getCurrentObject(), vW->getCamera(), vW->getProjectionPlane(), ui->comboBoxProjectionType->currentIndex(), ui->comboBoxRepresentationType->currentIndex(), ui->comboBoxShadingAlg->currentIndex(), globalLightSettings);
	}
}