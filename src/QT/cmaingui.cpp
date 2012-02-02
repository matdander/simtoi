#include "cmaingui.h"

#include <QWidgetList>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QVariant>
#include <QString>
#include <QMessageBox>
#include <QTreeView>
#include <QStringList>
#include <QFileDialog>
#include <vector>
#include <utility>

#include "CGLWidget.h"
#include "enumerations.h"
#include "CGLShaderList.h"
#include "CModel.h"
#include "CModelList.h"
#include "CTreeModel.h"
#include "CParameterItem.h"

Q_DECLARE_METATYPE(eModels);
Q_DECLARE_METATYPE(eGLShaders);

cmaingui::cmaingui(QWidget *parent_widget)
    : QMainWindow(parent_widget)
{
	ui.setupUi(this);

	// Set initial values for the spinboxes:
	ui.spinModelScale->setRange(0.01, 1.0);
	ui.spinModelScale->setSingleStep(0.05);
	ui.spinModelScale->setValue(0.05);
	ui.spinModelSize->setRange(64, 1024);
	ui.spinModelSize->setSingleStep(64);
	ui.spinModelSize->setValue(128);
	ui.spinTimestep->setValue(0.10);
	ui.spinTimestep->setSingleStep(0.1);
	ui.spinTimestep->setRange(0, 10000);

	mAnimating = false;

	// Now setup some signals and slots
	connect(ui.btnModelArea, SIGNAL(clicked(void)), this, SLOT(addGLArea(void)));
	connect(ui.mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(subwindowSelected(QMdiSubWindow*)));
	connect(ui.btnStartStop, SIGNAL(clicked(void)), this, SLOT(Animation_StartStop(void)));
	connect(ui.btnReset, SIGNAL(clicked(void)), this, SLOT(Animation_Reset(void)));
	connect(ui.btnMinimizer, SIGNAL(clicked(void)), this, SLOT(RunMinimizer(void)));
	connect(ui.btnLoadData, SIGNAL(clicked(void)), this, SLOT(LoadData(void)));

	// TODO: Remove this, shouldn't be hard-coded!
	mShaderSourceDir = "/home/bkloppenborg/workspace/simtoi/src/shaders/";
	mKernelSourceDir = "/home/bkloppenborg/workspace/simtoi/lib/liboi/src/kernels/";
	mDataDir = "./";
}

cmaingui::~cmaingui()
{

}

void cmaingui::Animation_StartStop()
{
	CGLWidget *widget = (CGLWidget*) ui.mdiArea->activeSubWindow()->widget();
	if(mAnimating)
	{
		widget->EnqueueOperation(GLT_StopAnimate);
		mAnimating = false;
		ui.btnStartStop->setText("Start");
	}
	else
	{
		widget->SetTimestep(ui.spinTimestep->value());
		widget->EnqueueOperation(GLT_Animate);
		mAnimating = true;
		ui.btnStartStop->setText("Stop");
	}
}

void cmaingui::Animation_Reset()
{
	CGLWidget *widget = (CGLWidget*) ui.mdiArea->activeSubWindow()->widget();
	widget->SetTime(0);
	widget->EnqueueOperation(GLT_RenderModels);

}


void cmaingui::closeEvent(QCloseEvent *evt)
{
	QList<QMdiSubWindow *> windows = ui.mdiArea->subWindowList();
    for (int i = int(windows.count()) - 1; i > 0; i--)
    {
    	CGLWidget * tmp = (CGLWidget *)windows.at(i)->widget();
    	tmp->stopRendering();
    }
    QMainWindow::closeEvent(evt);
}


void cmaingui::addGLArea()
{
	// Create a new subwindow with a title and close button:
    CGLWidget *widget = new CGLWidget(ui.mdiArea, mShaderSourceDir, mKernelSourceDir);
    QMdiSubWindow *sw = ui.mdiArea->addSubWindow(widget, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    sw->setWindowTitle("Model Area");

    // TODO: This is approximately right for my machine, probably not ok on other OSes.
    int frame_width = 8;
    int frame_height = 28;
    int model_width = ui.spinModelSize->value();
    int model_height = ui.spinModelSize->value();
    sw->setFixedSize(model_width + frame_width, model_height + frame_height);
    //sw->resize(model_width + frame_width, model_height + frame_height);
    sw->show();

    widget->resize(model_width, model_height);
    widget->SetScale(ui.spinModelScale->value());
    widget->startRendering();

    // TODO: Remove later.  Add a sphere:
    widget->AddModel(MDL_SPHERE);
    widget->SetShader(0, SHDR_LD_HESTEROFFER1997);
//    widget->AddModel(MDL_CYLINDER);
//    widget->SetPositionType(1, POSITION_ORBIT);

    // Just messing around...
    CTreeModel * TreeModel = new CTreeModel();
	QStringList labels = QStringList();
	labels << "Name" << "Free" << "Value";
	TreeModel->setColumnCount(3);
	TreeModel->setHorizontalHeaderLabels(labels);
	CModelList * model_list = widget->GetModelList();

	QList<QStandardItem *> items;
	QStandardItem * item;
	QStandardItem * parent;
	CModel * model;
	CPosition * position;
	CGLShaderWrapper * shader;

	for(int i = 0; i < model_list->size(); i++)
	{
		// First pull out the model parameters
		model = model_list->GetModel(i);
		items = LoadParametersHeader(QString("Model"), model);
		parent = items[0];
		TreeModel->appendRow(items);
		LoadParameters(parent, model);

		// Now for the Position Parameters
		position = model->GetPosition();
		items = LoadParametersHeader(QString("Position"), position);
		item = items[0];
		parent->appendRow(items);
		LoadParameters(item, position);

		// Lastly for the shader:
		shader = model->GetShader();
		items = LoadParametersHeader(QString("Shader"), shader);
		item = items[0];
		parent->appendRow(items);
		LoadParameters(item, shader);

	}

	// Set the model
	ui.treeModels->setModel(TreeModel);
	ui.treeModels->setHeaderHidden(false);
	ui.treeModels->resizeColumnToContents(1);

	// Now connect the slot
	connect(TreeModel, SIGNAL(parameterUpdated(void)), this, SLOT(render(void)));
}

void cmaingui::addModel(void)
{
//	QMdiSubWindow * window = ui.mdiArea->activeSubWindow();
//
//	if(window != NULL)
//	{
//		CGLWidget * widget = (CGLWidget*) window->widget();
//		QVariant tmp = ui.cboModels->itemData(ui.cboModels->currentIndex());
//		eModels model = tmp.value<eModels>();
////		tmp = ui.cboShaders->itemData(ui.cboShaders->currentIndex());
////		eGLShaders shader = tmp.value<eGLShaders>();
//
//		widget->AddModel(model);
//	}
//	else
//	{
//		QMessageBox msgBox;
//		msgBox.setText("Please select a window to which the model may be added.");
//		msgBox.exec();
//	}
}

void cmaingui::delGLArea()
{
    CGLWidget *widget = (CGLWidget*) ui.mdiArea->activeSubWindow()->widget();
    if (widget)
    {
        widget->stopRendering();
        delete widget;
    }
}

void cmaingui::RunMinimizer()
{
    CGLWidget *widget = (CGLWidget*) ui.mdiArea->activeSubWindow()->widget();

    if(!widget->OpenCLInitialized())
    	widget->EnqueueOperation(CLT_Init);

    widget->LoadMinimizer();
    widget->RunMinimizer();
}

void cmaingui::LoadData()
{
	string tmp;
	int size;
	// Ensure there is a selected widget, if not immediately return.
    QMdiSubWindow * sw = ui.mdiArea->activeSubWindow();
    if(!sw)
    	return;

    CGLWidget *widget = (CGLWidget*) sw->widget();

    QFileDialog dialog(this);
    dialog.setDirectory(QString::fromStdString(mDataDir));
    dialog.setNameFilter(tr("Data Files (*.fit *.fits *.oifits)"));
    dialog.setFileMode(QFileDialog::ExistingFiles);


    QStringList filenames;
    QString dir = "";
	if (dialog.exec())
	{
		filenames = dialog.selectedFiles();
	}

	if(filenames.size() > 0)
		mDataDir = QFileInfo(filenames[0]).absolutePath().toStdString();

	for(int i = 0; i < filenames.size(); i++)
	{
		tmp = filenames[i].toStdString();
		size = tmp.size() - mDataDir.size();
		widget->LoadData(tmp);
		ui.listOpenFiles->addItem(QString::fromStdString( tmp.substr(mDataDir.size() + 1, size) ));
	}
}

void cmaingui::LoadParameters(QStandardItem * parent, CParameters * parameters)
{
	for(int j = 0; j < parameters->GetNParams(); j++)
	{
		QList<QStandardItem *> items;
		QStandardItem * item;

		// First the name
		item = new QStandardItem(QString::fromStdString(parameters->GetParamName(j)));
		items << item;

		// Now the checkbox
		item = new CParameterItem(parameters, j);
		item->setEditable(true);
		item->setCheckable(true);
		if(parameters->IsFree(j))
			item->setCheckState(Qt::Checked);
		else
			item->setCheckState(Qt::Unchecked);
		items << item;

		// Lastly the value:
		item = new CParameterItem(parameters, j);
		item->setEditable(true);
		item->setData(QVariant((double)parameters->GetParam(j)), Qt::DisplayRole);
		items << item;

		parent->appendRow(items);
	}
}

QList<QStandardItem *> cmaingui::LoadParametersHeader(QString name, CParameters * param_base)
{
	QList<QStandardItem *> items;
	QStandardItem * item;
	item = new QStandardItem(name);
	items << item;
	item = new QStandardItem(QString(""));
	items << item;
	item = new QStandardItem(QString::fromStdString(param_base->GetName()));
	items << item;

	return items;
}

void cmaingui::render()
{
    CGLWidget * widget = (CGLWidget*) ui.mdiArea->activeSubWindow()->widget();
    if(widget)
    {
    	widget->EnqueueOperation(GLT_RenderModels);
    }
}

void cmaingui::subwindowSelected(QMdiSubWindow * window)
{
	//ui.treeModels
}
