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
#pragma once
#include "ImfPackage.h"
#include <QtWidgets/QMainWindow>
#include <QCloseEvent>
#include <QKeyEvent> // (k)

class QFileDialog;
class QDockWidget;
class WidgetImpBrowser;
class WidgetCentral;
class QMessageBox;
class QUndoGroup;
//WR
class QMessageBox;
class QProgressDialog;
class JobQueue;
//WR

class MainWindow : public QMainWindow {

	Q_OBJECT

public:
	MainWindow(QWidget *pParent = NULL);
	virtual ~MainWindow() {}
	void setStartupDirectory(const QString &rStartupDirectory, const bool rOpenAllCpls);

signals:
	void SettingsSaved();
	void KeyStroke(int); // (k)
	void ImpOpened(bool);
	void ImpClosed(bool);
	void StartupDirectory();
public slots:
	void ShowWidgetAbout();
	void ShowWorkspaceLauncher();
	void ShowWorkspaceLauncherPartialImp();
	void ShowCplEditor(const QUuid &rCplAssetId);
	void CloseImfPackage();
	void SaveCurrent();
	void SaveAllCpl();
	void SaveAsNewCPL();
	void WritePackage();
	void showStatusMessage(const QString &text, const int &timeout, const QString &color);
	void rJobQueueFinished();


private slots:
	void rWorkspaceLauncherAccepted();
	void rWorkspaceLauncherPartialImpAccepted();
	void rEnableSaveActions();
	void rFocusChanged(QWidget *pOld, QWidget *pNow);
	void rSaveCPLRequest();
	void rOpenImpRequest();
	void rCloseImpRequest();
	void rReinstallImp();
	//WR
	void ShowQcReport(const QString &rQcResult, const QVariant &rIdentifier);
	void CopyQcReport();
	void rCallPhoton();
	void rAutoInstallImp(const bool rOpenAllCpls = false);
	void informIsSupplementalImp();
	//WR

private:
	Q_DISABLE_COPY(MainWindow);
	void InitLayout();
	void InitMenuAndToolbar();
	void CenterWidget(QWidget *pWidget, bool useSizeHint);
	void closeEvent (QCloseEvent *event);
	//writes all via "Save as new CPL" created CPL-Filepaths to QList "mpUnwrittenCPL"
	void SetUnwrittenCPL(QString FilePath);
	//returns 0 if undostack is empty
	bool checkUndoStack();

	QMessageBox	*mpMsgBox;
//#ifdef ARCHIVIST
	QDockWidget *mpDockWidgetImagePreview;
//#endif
	WidgetImpBrowser *mpWidgetImpBrowser;
	WidgetCentral	*mpCentralWidget;
	QUndoGroup *mpUndoGroup;
	QAction	*mpActionSave;
	QAction	*mpActionSaveAll;
	QAction	*mpActionSaveAsNewCPL;
	QList <QString> mpUnwrittenCPLs;
	QString mpRootDirection;
	QStatusBar *mpStatusBar;
	//WR
	QProgressDialog *mpProgressDialog;
	JobQueue *mpJobQueue;
	QString mQcReport;
	//WR

};
