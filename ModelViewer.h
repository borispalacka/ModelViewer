#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets>
#include "ui_ModelViewer.h"
#include "ViewerWidget.h" 
#include <QTableWidget>
#include <QColorDialog>
#include <QMessageBox>

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
	LightSettings *globalLightSettings = nullptr;
	QMessageBox msgBox;
	int sizex = 700, sizey = 700;

	//2Dobjects
	QMap <QString, Object2D> object_map; 
	Object2D current_object;

	//Table widget
	QMenu* tableWidgetContextMenu = new QMenu();
	int colored_row_index = 0;

	bool isIn3dMode = false;

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
	//Table Widget
	void objectTableWidgetUpdate();

	//save/load state 2D Scene
	void saveSceneState(QString filename);
	void loadSceneState(QString filename);

	//Viewer widget setup
	void createViewerWidget(int width, int height);

private slots:
	//table widget
	void on_tableWidgetObjectList_customContextMenuRequested(const QPoint& pos);
	void on_tableWidgetObjectList_cellDoubleClicked(int row, int column);

	//actions
	void on_actionOpen_triggered();
	void on_actionSave_as_triggered();
	void on_actionClear_triggered();
	void on_actionExit_triggered();
	void on_actionSave_state_triggered();
	void on_actionLoad_state_triggered();

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
	// 
	void on_checkBoxCameraSettings_stateChanged(int state);
	void on_checkBoxLightSettings_stateChanged(int state);
	void on_checkBoxVtkOutput_stateChanged(int state);

	void on_comboBoxTypeCreateVTK_currentIndexChanged(int index);
	void on_horizontalSliderZenit_valueChanged(int value);
	void on_horizontalSliderAzimut_valueChanged(int value);
	void on_comboBoxProjectionType_currentIndexChanged(int index);
	void on_horizontalSliderCameraCoordZ_valueChanged(int value);
	void on_comboBoxRepresentationType_currentIndexChanged(int index);
	//Light settings
	void on_comboBoxShadingAlg_currentIndexChanged(int index);
	//reflexion/difusion/ambient coefficients handlers
	void on_horizontalSliderRdCoefficient_valueChanged(int value);
	void on_horizontalSliderRsCoefficient_valueChanged(int value);
	void on_horizontalSliderRaCoefficient_valueChanged(int value);
	void on_horizontalSliderLightH_valueChanged(int value);

	//light position handlers
	void on_spinBoxLightPosX_valueChanged(int value);
	void on_spinBoxLightPosY_valueChanged(int value);
	void on_spinBoxLightPosZ_valueChanged(int value);

	//Incident light ray handlers
	void on_spinBoxLightIntensityRed_valueChanged(int value);
	void on_spinBoxLightIntensityGreen_valueChanged(int value);
	void on_spinBoxLightIntensityBlue_valueChanged(int value);

	//Ambient light reflexion handlers
	void on_spinBoxLightIntensityAmbientRed_valueChanged(int value);
	void on_spinBoxLightIntensityAmbientGreen_valueChanged(int value);
	void on_spinBoxLightIntensityAmbientBlue_valueChanged(int value);



	void on_action2D_triggered();
	void on_action3D_triggered();
	
	void on_pushButtonCreateVTK_clicked();
};
