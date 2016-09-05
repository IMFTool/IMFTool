/* Copyright(C) 2016 Björn Stresing, Denis Manthey, Wolfgang Ruppel
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
#include <QFrame>
#include <QTableView>

class ImfPackage;
class QToolBar;
class QUndoStack;
class QMessageBox;
class UndoProxyModel;
class QSortFilterProxyModel;
class QProgressDialog;
class JobQueue;


class CustomTableView : public QTableView {

	Q_OBJECT

public:
	CustomTableView(QWidget *pParent = NULL);
	virtual ~CustomTableView() {}

protected:
	virtual void startDrag(Qt::DropActions supportedActions);
};


class WidgetImpBrowser : public QFrame {

	Q_OBJECT

public:
	WidgetImpBrowser(QWidget *pParent = NULL);
	virtual ~WidgetImpBrowser();
	void InstallImp(const QSharedPointer<ImfPackage> &rImfPackage, bool validateHash = false);
	void UninstallImp();
	bool IsImpInstalled() const { return mpImfPackage; }
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;
	QUndoStack* GetUndoStack() const { return mpUndoStack; }
	QSharedPointer<AssetCpl> GenerateEmptyCPL();
	QDir GetWorkingDir() {return mpImfPackage->GetRootDir();}

signals:
	void ImpSaveStateChanged(bool isDirty);
	void ImplInstalled(bool installed);
	void ShowCpl(const QUuid &rCplAssetId);
	void WritePackageComplete();
	void CallSaveAllCpl();

	public slots:
	void Save();
	void ShowResourceGeneratorWavMode();
	void ShowResourceGeneratorTimedTextMode();
	void ShowCompositionGenerator();
	//WR begin
	void RecalcHashForCpls();
	//WR end

	private slots :
	void rRemoveSelectedRow();
	void rDeleteSelectedRow();
	void rShowResourceGeneratorForSelectedRow();
	void rShowResourceGeneratorForAsset(const QUuid &rAssetId);
	void rResourceGeneratorAccepted();
	void rCompositionGeneratorAccepted();
	void rCustomMenuRequested(QPoint pos);
	void rMapCurrentRowSelectionChanged(const QModelIndex &rCurrent, const QModelIndex &rPrevious);
	void rJobQueueFinished();
	void rImpViewDoubleClicked(const QModelIndex &rIndex);
	void rOpenCplTimeline();
	void rReinstallImp();

protected:
	virtual void keyPressEvent(QKeyEvent *pEvent);

private:
	Q_DISABLE_COPY(WidgetImpBrowser);
	void InitLayout();
	void InitToolbar();
	void StartOutgest(bool clearUndoStack = true);
	void ValidateHash();

	CustomTableView *mpViewImp;
	CustomTableView *mpViewAssets;
	QSharedPointer<ImfPackage> mpImfPackage;
	QToolBar *mpToolBar;
	QUndoStack *mpUndoStack;
	UndoProxyModel *mpUndoProxyModel;
	QSortFilterProxyModel *mpSortProxyModelImp;
	QSortFilterProxyModel *mpSortProxyModelAssets;
	QMessageBox *mpMsgBox;
	QProgressDialog *mpProgressDialog;
	JobQueue *mpJobQueue;
};
