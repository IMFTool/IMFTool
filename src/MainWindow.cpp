/* Copyright(C) 2016 Björn Stresing, Denis Manthey, Wolfgang Ruppel, Krispin Weiss
 *
 * This program is free software : you can redistribute it and / or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 */
#include "MainWindow.h"
#include "global.h"
#include "WidgetAbout.h"
#include "WidgetImpBrowser.h"
#include "WizardWorkspaceLauncher.h"
#include "WizardPartialImpGenerator.h"
#include "MetadataExtractor.h"
#include "WidgetCentral.h"
#include <QMenuBar>
#include <QUndoGroup>
#include <QToolBar>
#include <QCoreApplication>
#include <QGridLayout>
#include <QDesktopWidget>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QDockWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QStatusBar>
#include <QDir>

#include <QShortcut> //(k)
//WR
#include <QProcess>
#include "JobQueue.h"
#include "Jobs.h"
#include <QProgressDialog>
#include <QTextEdit>
#include <QClipboard>
//WR




MainWindow::MainWindow(QWidget *pParent /*= NULL*/) :
QMainWindow(pParent) {

	InitLayout();
	InitMenuAndToolbar();
	// Prepare mpJobQueue for Photon calls
	mpJobQueue = new JobQueue(this);
	mpJobQueue->SetInterruptIfError(true);
	connect(mpJobQueue, SIGNAL(finished()), this, SLOT(rJobQueueFinished()));
	mpProgressDialog = new QProgressDialog();
	mpProgressDialog->setWindowModality(Qt::WindowModal);
	mpProgressDialog->setMinimumSize(500, 150);
	mpProgressDialog->setMinimum(0);
	mpProgressDialog->setMaximum(100);
	mpProgressDialog->setValue(100);
	mpProgressDialog->setMinimumDuration(0);
	connect(mpJobQueue, SIGNAL(Progress(int)), mpProgressDialog, SLOT(setValue(int)));
	connect(mpJobQueue, SIGNAL(NextJobStarted(const QString&)), mpProgressDialog, SLOT(setLabelText(const QString&)));

}

void MainWindow::InitLayout() {

	mpMsgBox = new QMessageBox(this);
	mpMsgBox->setIcon(QMessageBox::Warning);

	mpCentralWidget = new WidgetCentral(this);
	mpStatusBar = new QStatusBar(this); // (k)
	setCentralWidget(mpCentralWidget);
	setStatusBar(mpStatusBar);

	new QShortcut(QKeySequence(Qt::Key_Right), mpCentralWidget, SLOT(rNextFrame())); // (k)
	new QShortcut(QKeySequence(Qt::Key_Left), mpCentralWidget, SLOT(rPrevFrame())); // (k)



	QDockWidget *p_dock_widget_imp_browser = new QDockWidget(tr("IMP Browser"), this);
	QWidget *p_second_intermediate_widget = new QWidget(this); // We need this widget for painting the border defined in stylesheet.
	mpWidgetImpBrowser = new WidgetImpBrowser(p_second_intermediate_widget);
	QHBoxLayout *p_second_intermediate_layout = new QHBoxLayout();
	p_second_intermediate_layout->setMargin(0);
	p_second_intermediate_layout->addWidget(mpWidgetImpBrowser);
	p_second_intermediate_widget->setLayout(p_second_intermediate_layout);
	p_dock_widget_imp_browser->setWidget(p_second_intermediate_widget);
	p_dock_widget_imp_browser->setFeatures(QDockWidget::DockWidgetMovable);
#ifdef IMFTOOL
	addDockWidget(Qt::LeftDockWidgetArea, p_dock_widget_imp_browser);
#endif
	connect(mpWidgetImpBrowser, SIGNAL(ShowCpl(const QUuid &)), this, SLOT(ShowCplEditor(const QUuid &)));
	connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), this, SLOT(rFocusChanged(QWidget*, QWidget*)));
	connect(mpWidgetImpBrowser, SIGNAL(WritePackageComplete()), this, SLOT(rReinstallImp()));
	//WR begin
	connect(mpCentralWidget, SIGNAL(SaveAllCplFinished()), mpWidgetImpBrowser, SLOT(RecalcHashForCpls()));
	connect(mpWidgetImpBrowser, SIGNAL(CallSaveAllCpl()), this, SLOT(SaveAllCpl()));
	//WR end
}

void MainWindow::InitMenuAndToolbar() {

	mpUndoGroup = new QUndoGroup(this);

	mpActionSave = new QAction(QIcon(":/save.png"), tr("&Save CPL"), menuBar());
	mpActionSave->setShortcut(QKeySequence::Save);
	mpActionSave->setDisabled(true);
	connect(mpUndoGroup, SIGNAL(cleanChanged(bool)), mpActionSave, SLOT(setDisabled(bool)));
	connect(mpActionSave, SIGNAL(triggered()), this, SLOT(rSaveCPLRequest()));
	//mpActionSaveAll = new QAction(QIcon(":/save_all.png"), tr("&Save All"), menuBar());
	//mpActionSaveAll->setDisabled(true);

	mpActionSaveAsNewCPL = new QAction(QIcon(":/save_as_new.png"), tr("&Save as new CPL"), menuBar());
	mpActionSaveAsNewCPL->setDisabled(true);
	connect(mpUndoGroup, SIGNAL(cleanChanged(bool)), mpActionSaveAsNewCPL, SLOT(setDisabled(bool)));
	connect(mpActionSaveAsNewCPL, SIGNAL(triggered()), this, SLOT(SaveAsNewCPL()));

	QAction *p_action_undo = mpUndoGroup->createUndoAction(this, tr("Undo"));
	p_action_undo->setIcon(QIcon(":/undo.png"));
	p_action_undo->setShortcut(QKeySequence::Undo);
	QAction *p_action_redo = mpUndoGroup->createRedoAction(this, tr("Redo"));
	p_action_redo->setIcon(QIcon(":/redo.png"));
	p_action_redo->setShortcut(QKeySequence::Redo);
	QMenu *p_menu_about = new QMenu(tr("&HELP"), menuBar());
	QAction *p_action_about = new QAction(QIcon(":/information.png"), tr("&About"), menuBar());
	connect(p_action_about, SIGNAL(triggered(bool)), this, SLOT(ShowWidgetAbout()));
	p_menu_about->addAction(p_action_about);
	QMenu *p_menu_file = new QMenu(tr("&FILE"), menuBar());
	QAction *p_action_open = new QAction(QIcon(":/folder.png"), tr("&Open IMF Package"), menuBar());
	connect(p_action_open, SIGNAL(triggered(bool)), this, SLOT(rOpenImpRequest()));
	p_action_open->setShortcut(QKeySequence::Open);
	QAction *p_action_write = new QAction(QIcon(":/inbox_upload.png"), tr("&Write IMF Package"), menuBar());
	p_action_write->setDisabled(true);
	connect(p_action_write, SIGNAL(triggered(bool)), this, SLOT(WritePackage()));
	connect(mpWidgetImpBrowser, SIGNAL(ImpSaveStateChanged(bool)), p_action_write, SLOT(setEnabled(bool)));

	QAction *p_action_writePartial = new QAction(QIcon(":/inbox_upload_plus.png"), tr("&Write Partial IMF Package"), menuBar());
	p_action_writePartial->setDisabled(true);
	connect(p_action_writePartial, SIGNAL(triggered(bool)), this, SLOT(ShowWorkspaceLauncherPartialImp()));
	connect(mpWidgetImpBrowser, SIGNAL(ImpSaveStateChanged(bool)), p_action_writePartial, SLOT(setEnabled(bool)));

	QAction *p_action_close = new QAction(QIcon(":/close.png"), tr("&Close IMF Package"), menuBar());
	connect(p_action_close, SIGNAL(triggered(bool)), this, SLOT(rCloseImpRequest()));
	QAction *p_action_exit = new QAction(tr("&Quit"), menuBar());
	p_action_exit->setShortcut(tr("Ctrl+Q"));
	connect(p_action_exit, SIGNAL(triggered(bool)), qApp, SLOT(closeAllWindows()));

	// WR
	QAction *p_qc_photon = new QAction(QIcon(":/qc.png"), tr("&Create Photon QC Report"), menuBar());
	p_qc_photon->setDisabled(true);
	connect(p_qc_photon, SIGNAL(triggered(bool)), this, SLOT(rCallPhoton()));
	connect(this, SIGNAL(ImpOpened(bool)), p_qc_photon, SLOT(setEnabled(bool)));
	connect(this, SIGNAL(ImpClosed(bool)), p_qc_photon, SLOT(setDisabled(bool)));
	connect(this, SIGNAL(ImpOpened(bool)), this, SLOT(informIsSupplementalImp()));

	// WR

	p_menu_file->addAction(p_action_write);
	p_menu_file->addAction(p_action_writePartial);
	p_menu_file->addAction(p_qc_photon);
	p_menu_file->addAction(p_action_open);
	p_menu_file->addAction(p_action_close);
	p_menu_file->addSeparator();
	p_menu_file->addAction(mpActionSave);
	p_menu_file->addAction(mpActionSaveAsNewCPL);
	//p_menu_file->addAction(mpActionSaveAll);
	p_menu_file->addSeparator();
	p_menu_file->addAction(p_action_exit);
	QMenu *p_menu_tools = new QMenu(tr("&TOOLS"), menuBar());
	QAction *p_action_preferences = new QAction(QIcon(":/gear.png"), tr("&Preferences"), menuBar());
	//WR
	p_action_preferences->setDisabled(true);
	p_menu_tools->addAction(p_action_preferences);

	menuBar()->addMenu(p_menu_file);
	menuBar()->addMenu(p_menu_tools);
	menuBar()->addMenu(p_menu_about);

	QToolBar *p_tool_bar = addToolBar(tr("Main Window Toolbar"));
	p_tool_bar->setIconSize(QSize(20, 20));
	p_tool_bar->addAction(p_action_open);
	p_tool_bar->addAction(p_action_write);
	p_tool_bar->addAction(p_action_writePartial);
	p_tool_bar->addAction(p_qc_photon);
	p_tool_bar->addSeparator();
	//p_tool_bar->addAction(mpActionSaveAll);
	p_tool_bar->addSeparator();
	p_tool_bar->addSeparator();
	p_tool_bar->addAction(mpActionSave);
	p_tool_bar->addAction(mpActionSaveAsNewCPL);
	p_tool_bar->addAction(p_action_undo);
	p_tool_bar->addAction(p_action_redo);

	connect(mpCentralWidget, SIGNAL(UndoStackChanged(QUndoStack*)), mpUndoGroup, SLOT(setActiveStack(QUndoStack*)));
	connect(mpCentralWidget, SIGNAL(UpdateStatusBar(const QString &, const int &, const QString &)), this, SLOT(showStatusMessage(const QString &, const int &, const QString &)));
}

			/* -----Denis Manthey Beg----- */

void MainWindow::rSaveCPLRequest() {

	mpMsgBox->setText(tr("Overwrite CPL?"));
	mpMsgBox->setInformativeText(tr("It is recommended to save modified CPLs as New CPL, do you really want to overwrite the existing CPL ? \nThis action can not be undone and the Id of the CPL will not be updated!"));
	mpMsgBox->setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
	mpMsgBox->setDefaultButton(QMessageBox::Cancel);
	mpMsgBox->setIcon(QMessageBox::Warning);
	int ret = mpMsgBox->exec();
	if(ret == QMessageBox::Save)
		SaveCurrent();
	else if(ret == QMessageBox::Cancel)
		return;
}

//Check unsaved changes when closing application
void MainWindow::closeEvent (QCloseEvent *event)
{
	if (checkUndoStack() == 1)
		event->ignore();
	else
		event->accept();
}

//Check unsaved changes when opening new IMP
void MainWindow::rOpenImpRequest() {
	if (checkUndoStack() == 1)
		return;
	else
		ShowWorkspaceLauncher();
}

//Check unsaved changes when closing IMP
void MainWindow::rCloseImpRequest() {
	if (checkUndoStack() == 1)
		return;
	else
		CloseImfPackage();
}
//WR

void MainWindow::rCallPhoton() {
	if (!mpRootDirection.isEmpty()) {

		// Test for Java VM
		QString qresult;
		QProcess *myProcess = new QProcess();
		const QString program = "java";
		QStringList arg;
		QString error_msg;
		Error error(Error::None);
		arg << "-version";
		myProcess->start(program, arg);
		myProcess->waitForFinished(-1);
		if (myProcess->exitStatus() == QProcess::NormalExit) {
			if (myProcess->exitCode() != 0) { error = Error(Error::ExitCodeNotZero); }
		} else {
			error = Error(Error::ExitStatusError);
		}
		if (myProcess->error() == QProcess::FailedToStart) {
			error = Error(Error::Unknown);
		}

		try {
			qresult = myProcess->readAllStandardError();
			QRegularExpression re("([0-9]+)\\.([0-9]+)\\.([0-9]+)");
			QRegularExpressionMatch match = re.match(qresult, 0);
			if (match.hasMatch()) {
			    QString major = match.captured(1);
			    QString minor = match.captured(2);
			    if ((major.toInt() != 1) || (minor.toInt() < 8)) {
			    	error_msg = "Java version mismatch, current version is " + match.captured(0);
					mpMsgBox->setText(tr("Photon requires Java SDK 1.8 or higher - Photon QC report cannot be created!"));
					mpMsgBox->setIcon(QMessageBox::Warning);
					mpMsgBox->setInformativeText(error_msg);
					mpMsgBox->setStandardButtons(QMessageBox::Ok);
					mpMsgBox->setDefaultButton(QMessageBox::Ok);
					mpMsgBox->exec();
			    }
			}
		}
		catch (...) {
			error = Error(Error::Unknown);
		}

		if(error.IsError()) {
			qDebug() << "No Java / wrong Java version" << qresult;
			error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
			mpMsgBox->setText(tr("java not available - Cannot run Photon ! Photon requires Java JRE Version 1.8 or higher."));
			mpMsgBox->setIcon(QMessageBox::Warning);
			mpMsgBox->setInformativeText(error_msg);
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->exec();
			return;
		}



		mpJobQueue->FlushQueue();
		JobCallPhoton *p_qc_job = new JobCallPhoton(mpRootDirection);
		connect(p_qc_job, SIGNAL(Result(const QString&, const QVariant&)), this, SLOT(ShowQcReport(const QString&, const QVariant&)));
		mpJobQueue->AddJob(p_qc_job);
		mpJobQueue->StartQueue();
	}
}
//WR

bool MainWindow::checkUndoStack() {

	//check if every UndoStack is clean or not!
	bool changes = false;
	for (int i = 0; i < mpUndoGroup->stacks().size(); i++){
		if (mpUndoGroup->stacks().at(i)->isClean() == false)
			changes = true;
	}
	if (mpWidgetImpBrowser->GetUndoStack()->isClean() == false)
		changes = true;

	if(changes == true) {
		mpMsgBox->setText(tr("There are unsaved changes in the current IMP!"));
		mpMsgBox->setInformativeText(tr("Do you want to proceed?"));
		mpMsgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel | QMessageBox::SaveAll);
		mpMsgBox->setDefaultButton(QMessageBox::Cancel);
		mpMsgBox->setIcon(QMessageBox::Warning);
		int ret = mpMsgBox->exec();

		if(ret == QMessageBox::Cancel)
			return 1;
		else if(ret == QMessageBox::SaveAll) {
			WritePackage();
			return 0;
		}
		else {
			for(int i=0; i<mpUnwrittenCPLs.size(); i++){
				QFile::remove(mpUnwrittenCPLs.at(i));
			}
			return 0;
		}
	}
	else
		return 0;
}

			/* -----Denis Manthey End----- */

void MainWindow::ShowWidgetAbout() {

	WidgetAbout *p_about = new WidgetAbout(this);
	p_about->setAttribute(Qt::WA_DeleteOnClose);
	CenterWidget(p_about, true);
	p_about->show();
}

void MainWindow::ShowWorkspaceLauncher() {

	WizardWorkspaceLauncher *p_workspace_launcher = new WizardWorkspaceLauncher(this);
	p_workspace_launcher->setAttribute(Qt::WA_DeleteOnClose);
	p_workspace_launcher->addPage(new WizardWorkspaceLauncherPage(p_workspace_launcher));
	connect(p_workspace_launcher, SIGNAL(accepted()), this, SLOT(rWorkspaceLauncherAccepted()));
	p_workspace_launcher->show();
}


				/* -----Denis Manthey Beg----- */
void MainWindow::ShowWorkspaceLauncherPartialImp() {

	WizardPartialImpGenerator *p_partial_imp_generator = new WizardPartialImpGenerator(this);
	p_partial_imp_generator->setAttribute(Qt::WA_DeleteOnClose);
	p_partial_imp_generator->addPage(new WizardPartialImpGeneratorPage(p_partial_imp_generator));
	connect(p_partial_imp_generator, SIGNAL(accepted()), this, SLOT(rWorkspaceLauncherPartialImpAccepted()));
	p_partial_imp_generator->show();
}
				/* -----Denis Manthey End----- */

void MainWindow::rWorkspaceLauncherAccepted() {

	WizardWorkspaceLauncher *p_workspace_launcher = qobject_cast<WizardWorkspaceLauncher *>(sender());
	if(p_workspace_launcher) {
		QString working_dir = p_workspace_launcher->field(FIELD_NAME_WORKING_DIR).toString();
		QDir dir(working_dir);
		mpRootDirection = working_dir;
		rAutoInstallImp();
	}
}



			/* -----Denis Manthey Beg----- */
void MainWindow::rWorkspaceLauncherPartialImpAccepted() {

	WizardPartialImpGenerator *p_partial_imp_generator = qobject_cast<WizardPartialImpGenerator *>(sender());
	bool success = false;
	if (p_partial_imp_generator) {
		QString partial_imp_dir = QString("%1/%2").arg(p_partial_imp_generator->field(FIELD_NAME_PARTIAL_DIR).toString()).arg(p_partial_imp_generator->field(FIELD_NAME_PARTIAL_NAME).toString());
		QString partial_imp_issuer = QString("%1").arg(p_partial_imp_generator->field(FIELD_NAME_PARTIAL_ISSUER).toString());
		QString partial_imp_annotation = QString("%1").arg(p_partial_imp_generator->field(FIELD_NAME_PARTIAL_ANNOTATION).toString());
		//Check if folder exists
		if (QDir(partial_imp_dir).exists() == false)
			success = QDir().mkdir(partial_imp_dir);
		else {
			mpMsgBox->setText(tr("Couldn't create folder"));
			mpMsgBox->setIcon(QMessageBox::Warning);
			mpMsgBox->setInformativeText("Folder already exists in the selected directory!");
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->exec();
			return;
		}
		//check writing permissions
		if (success == true) {
			mpWidgetImpBrowser->ExportPartialImp(partial_imp_dir, partial_imp_issuer, partial_imp_annotation);
		} else {
			mpMsgBox->setText(tr("Couldn't create folder"));
			mpMsgBox->setIcon(QMessageBox::Warning);
			mpMsgBox->setInformativeText("Please check your writing permissions!");
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->exec();
		}
	}
}
				/* -----Denis Manthey End----- */

void MainWindow::rEnableSaveActions() {

	mpActionSave->setEnabled(true);
	mpActionSaveAll->setEnabled(true);
}

void MainWindow::CloseImfPackage() {

	mpWidgetImpBrowser->UninstallImp();
	mpCentralWidget->UninstallImp();
	emit ImpClosed(true);
}

void MainWindow::ShowCplEditor(const QUuid &rCplAssetId) {

	int index = mpCentralWidget->ShowCplEditor(rCplAssetId);
	if(index >= 0) mpUndoGroup->addStack(mpCentralWidget->GetUndoStack(index));
}

void MainWindow::SaveCurrent() {

	mpCentralWidget->SaveCurrentCpl();
}


			/* -----Denis Manthey Beg----- */
void MainWindow::SaveAsNewCPL() {

	QSharedPointer<AssetCpl> newCPL = mpWidgetImpBrowser->GenerateEmptyCPL();
	mpCentralWidget->CopyCPL(newCPL);
	emit mpWidgetImpBrowser->ShowCpl(newCPL->GetId());		//Opens the new CPL in WidgetCentral as current tab
	//CopyCPL calls WriteNew for the old WidgetComposition object, here we call it for the new WidgetComposition object:
	SaveCurrent();
	SetUnwrittenCPL(newCPL->GetPath().absoluteFilePath());
}
			/* -----Denis Manthey En----- */


void MainWindow::rFocusChanged(QWidget *pOld, QWidget *pNow) {

	if(mpCentralWidget->isAncestorOf(pNow)) {
		mpUndoGroup->setActiveStack(mpCentralWidget->GetCurrentUndoStack());
	}
	else mpUndoGroup->setActiveStack(NULL);
}


void MainWindow::CenterWidget(QWidget *pWidget, bool useSizeHint) {

	QSize size;
	if(useSizeHint)
		size = pWidget->sizeHint();
	else
		size = pWidget->size();

	QDesktopWidget *d = QApplication::desktop();
	int w = d->width();   // returns screen width
	int h = d->height();  // returns screen height
	int mw = size.width();
	int mh = size.height();
	int cw = (w / 2) - (mw / 2);
	int ch = (h / 2) - (mh / 2);
	pWidget->move(cw, ch);
}

			/* -----Denis Manthey Beg----- */
void MainWindow::rReinstallImp() {

	//QDir dir(mpWidgetImpBrowser->GetWorkingDir());
	CloseImfPackage();
	QSharedPointer<ImfPackage> imf_package(new ImfPackage(mpRootDirection));
	imf_package->Ingest();
	mpWidgetImpBrowser->InstallImp(imf_package);
	mpCentralWidget->InstallImp(imf_package);
	emit ImpOpened(true);
}
			/* -----Denis Manthey End----- */

void MainWindow::rAutoInstallImp(const bool rOpenAllCpls /* = false*/) {
	QSharedPointer<ImfPackage> imf_package(new ImfPackage(mpRootDirection));
	ImfError error = imf_package->Ingest();
	if(error.IsError() == false) {
		if(error.IsRecoverableError() == true) {
			QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
			mpMsgBox->setText(tr("Ingest Warning"));
			mpMsgBox->setIcon(QMessageBox::Warning);
			mpMsgBox->setInformativeText(error_msg);
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->exec();
		}
		mpWidgetImpBrowser->InstallImp(imf_package);
		mpCentralWidget->InstallImp(imf_package);
		emit ImpOpened(true);
	}
	else {
		mpWidgetImpBrowser->UninstallImp();
		mpCentralWidget->UninstallImp();
		emit ImpClosed(true);
		QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
		mpMsgBox->setText(tr("Ingest Error"));
		mpMsgBox->setIcon(QMessageBox::Critical);
		mpMsgBox->setInformativeText(error_msg);
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->exec();
		mpRootDirection = QString();
		return;
	}
	if (rOpenAllCpls) {
		// For command-line option --imp-directory and --open-all
		// Open all CPLs in Timeline View
		for (int i = 0; i < imf_package->GetAssetCount(); i++) {
			if (imf_package->GetAsset(i)->GetType() == Asset::cpl) {
				QSharedPointer<AssetCpl> asset_cpl = imf_package->GetAsset(i).objectCast<AssetCpl>();
				if(asset_cpl && asset_cpl->Exists() == true) {
					emit mpWidgetImpBrowser->ShowCpl(asset_cpl->GetId());
				}
			}

		}

	}
}

void MainWindow::WritePackage() {

	mpWidgetImpBrowser->Save();
	mpUnwrittenCPLs.clear();
}

			/* -----Denis Manthey Beg----- */
void MainWindow::SaveAllCpl() {

	mpCentralWidget->SaveAllCpl();
}

void MainWindow::SetUnwrittenCPL(QString FilePath) {

	mpUnwrittenCPLs.append(FilePath);
}
			/* -----Denis Manthey End----- */
//WR
void MainWindow::showStatusMessage(const QString &text, const int &timeout, const QString &color) {
	mpStatusBar->setStyleSheet(color);
	mpStatusBar->showMessage(text, timeout);
}

//WR

void MainWindow::rJobQueueFinished() {
	mpProgressDialog->reset();
	QString error_msg;
	QList<Error> errors = mpJobQueue->GetErrors();
	for(int i = 0; i < errors.size(); i++) {
		error_msg.append(QString("%1: %2\n%3\n").arg(i + 1).arg(errors.at(i).GetErrorMsg()).arg(errors.at(i).GetErrorDescription()));
	}
	error_msg.chop(1); // remove last \n
	if (error_msg != "") {
		qDebug() << "rJobQueueFinished error:" << error_msg;
		mpMsgBox->setText(tr("Photon has returned an error:"));
		mpMsgBox->setInformativeText(error_msg + "\n\n Photon QC failed");
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->setIcon(QMessageBox::Critical);
		mpMsgBox->exec();
	}
}

void MainWindow::ShowQcReport(const QString &rQcResult, const QVariant &rIdentifier) {

	mQcReport = rQcResult;
	// Create wizard
	QWizard *qc_report_wizard = new QWizard(this, Qt::Popup | Qt::Dialog );
	// Create wizard page
	QWizardPage *qc_report_wizard_page = new QWizardPage(qc_report_wizard);
	qc_report_wizard->addPage(qc_report_wizard_page);
	qc_report_wizard->resize(1200,1000);
	qc_report_wizard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	qc_report_wizard->setWindowModality(Qt::WindowModal);
	qc_report_wizard->setWindowTitle(tr("Photon QC Report"));
	qc_report_wizard->setWizardStyle(QWizard::ModernStyle);
	qc_report_wizard->setStyleSheet("QWizard QPushButton {min-width: 60 px;}");

	// Create text widget
	QTextEdit *qc_report_view = new QTextEdit();
	QStringList report_as_list = mQcReport.split("\n"); //,QString::SkipEmptyParts);
	qc_report_view->setFontFamily("Courier New");
	qc_report_view->setAttribute(Qt::WA_DeleteOnClose, true);
	qc_report_view->setReadOnly(true);
	// Set colors for errors and warnings
	foreach (const QString &line, report_as_list) {
		if (line.contains("[ERROR]") || line.contains("[FATAL]"))
			qc_report_view->setTextColor(Qt::red);
		else if (line.contains("[WARN ]"))
			qc_report_view->setTextColor(Qt::yellow);
		else
			qc_report_view->setTextColor(QColor("#b1b1b1"));
		qc_report_view->insertPlainText(line + "\n");
	}

	// Create layout for text widget and buttons
	QVBoxLayout *vbox_layout = new QVBoxLayout();
	vbox_layout->addWidget(qc_report_view);
	qc_report_wizard_page->setLayout(vbox_layout);
	QList<QWizard::WizardButton> layout;
	qc_report_wizard->setOption(QWizard::HaveCustomButton1, true);
	qc_report_wizard->setButtonText(QWizard::CustomButton1, tr("Copy to Clipboard"));
	qc_report_wizard->setOption(QWizard::HaveCustomButton2, true);
	qc_report_wizard->setButtonText(QWizard::CustomButton2, tr("Add report as sidecar asset"));
	layout << QWizard::CustomButton1 << QWizard::Stretch << QWizard::CustomButton2 << QWizard::Stretch << QWizard::CancelButton;
	qc_report_wizard->setButtonLayout(layout);
	connect(qc_report_wizard->button(QWizard::CustomButton1), SIGNAL(clicked()), this, SLOT(CopyQcReport()));
	connect(qc_report_wizard->button(QWizard::CustomButton1), SIGNAL(clicked()), qc_report_wizard, SLOT(close()));
	connect(qc_report_wizard->button(QWizard::CustomButton2), SIGNAL(clicked()), this, SLOT(AddQcReportAsSidecar()));
	connect(qc_report_wizard->button(QWizard::CustomButton2), SIGNAL(clicked()), qc_report_wizard, SLOT(close()));

	qc_report_wizard->setAttribute(Qt::WA_DeleteOnClose, true);
	qc_report_wizard->show();
	qc_report_wizard->activateWindow();

}

void MainWindow::CopyQcReport() {

	QClipboard *clipboard = QGuiApplication::clipboard();
	clipboard->setText(mQcReport);
}
void MainWindow::AddQcReportAsSidecar() {
	mpWidgetImpBrowser->AddQcReportAsSidecar(mQcReport);
}
void MainWindow::setStartupDirectory (const QString &rStartupDirectory, const bool rOpenAllCpls) {
	mpRootDirection = rStartupDirectory;
	rAutoInstallImp(rOpenAllCpls);
}

void MainWindow::informIsSupplementalImp() {
	if (mpWidgetImpBrowser->GetImfPackage()->GetIsSupplemental()) {
		mpMsgBox->setText(tr("Supplemental IMP"));
		mpMsgBox->setInformativeText(tr("This IMP is a Supplemental IMP. \nYou may use the \"Load OV IMP\" button to load ancestor Original Version IMPs for previewing and editing this IMP."));
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->setIcon(QMessageBox::Information);
		mpMsgBox->exec();
	}

}

//WR


