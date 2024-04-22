#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets>
#include "ui_ModelViewer.h"
#include "ViewerWidget.h"

class ModelViewer : public QMainWindow
{
	Q_OBJECT

public:
	ModelViewer(QWidget* parent = Q_NULLPTR);

private:
	Ui::ModelViewerClass* ui;
	ViewerWidget* vW;

	QColor globalColor;
	QSettings settings;
	QMessageBox msgBox;	

	//Event filters
	bool eventFilter(QObject* obj, QEvent* event);

	//ViewerWidget Events
	bool ViewerWidgetEventFilter(QObject* obj, QEvent* event);
	void ViewerWidgetMouseButtonPress(ViewerWidget* w, QEvent* event);
	void ViewerWidgetMouseButtonRelease(ViewerWidget* w, QEvent* event);
	void ViewerWidgetMouseMove(ViewerWidget* w, QEvent* event);
	void ViewerWidgetLeave(ViewerWidget* w, QEvent* event);
	void ViewerWidgetEnter(ViewerWidget* w, QEvent* event);
	void ViewerWidgetWheel(ViewerWidget* w, QEvent* event);

	//ModelViewer Events
	void closeEvent(QCloseEvent* event);

	//Image functions
	bool openImage(QString filename);
	bool saveImage(QString filename);

private slots:
	//actions
	void on_actionOpen_triggered();
	void on_actionSave_as_triggered();
	void on_actionClear_triggered();
	void on_actionExit_triggered();

	//Tools slots
	void on_pushButtonSetColor_clicked();
	void on_toolButtonDrawLine_clicked();
	void on_toolButtonDrawCircle_clicked();
	void on_toolButtonDrawPolygon_clicked();
	void on_toolButtonDrawCurve_clicked();

	//Edit Tool slots
	// 
	//2D mode 
	void on_toolButtonEditPosition_clicked();

	void on_toolButtonEditRotation_clicked();
	void on_spinBoxRotation_valueChanged(int value);
	void on_spinBoxRotation_editingFinished();

	void on_toolButtonEditScale_clicked();
	void on_spinBoxScaleX_valueChanged(double value);
	void on_spinBoxScaleY_valueChanged(double value);
	void on_spinBoxScaleX_editingFinished();
	void on_spinBoxScaleY_editingFinished();

	void on_toolButtonEditShear_clicked();
	void on_spinBoxShearFactor_valueChanged(double value);
	void on_spinBoxShearFactor_editingFinished();

	void on_toolButtonSymmetry_clicked();

	//3D mode 
	void on_comboBoxTypeCreateVTK_currentIndexChanged(int index);
	void on_pushButtonCreateVTK_clicked();
	void on_horizontalSliderZenit_valueChanged(int value);
	void on_horizontalSliderAzimut_valueChanged(int value);
	void on_comboBoxProjectionType_currentIndexChanged(int index);
	void on_horizontalSliderCameraCoordZ_valueChanged(int value);
};
