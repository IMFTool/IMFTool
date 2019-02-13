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
#include <QFrame>
#include <QTableView>
#include <QFileDialog>
#include <QToolButton>
#include "WizardSidecarCompositionMapGenerator.h"

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

	void ExportPartialImp(QString &rDir, QString &rIssuer, QString &rAnnotation);
	//WR
	void SetPartialOutgestInProgress(bool rPartialOutgestInProgress) {mPartialOutgestInProgress = rPartialOutgestInProgress;}
	bool GetPartialOutgestInProgress() {return mPartialOutgestInProgress;}
	void SetPartialImpPath (QString rPartialImpPath) {mPartialImpPath = rPartialImpPath;}
	QString GetPartialImpPath () {return mPartialImpPath;}
	QSharedPointer<ImfPackage> GetImfPackage () {return mpImfPackage;}
	void LoadAdditionalImpPackage(const QSharedPointer<ImfPackage> &rImfPackage);
	void AddQcReportAsSidecar(const QString rQcReport);
	//WR

signals:
	void ImpSaveStateChanged(bool isDirty);
	void ImplInstalled(bool installed);
	void ShowCpl(const QUuid &rCplAssetId);
	void WritePackageComplete();
	void CallSaveAllCpl();
	void AssetAdded(QSharedPointer<AssetMxfTrack> &rAsset);

public slots:
	void Save();
	void ShowResourceGeneratorWavMode();
	void ShowResourceGeneratorTimedTextMode();
	void ShowCompositionGenerator();
	//WR begin
	void RecalcHashForCpls();
	void ShowResourceGeneratorMxfMode();
	void slotCurrentChanged(const QModelIndex &selected, const QModelIndex &deselected);
	void rExtractTargetFramesWidget(const QStringList, const QVariant &rIdentifier);
	void SaveTargetFrames();
	void slotPreviewClicked(int);
	//WR end
	void ShowSidecarCompositionMapGenerator(QSharedPointer<AssetScm> rAssetScm = QSharedPointer<AssetScm>(), WizardSidecarCompositionMapGenerator::eMode rMode = WizardSidecarCompositionMapGenerator::NewScm);

private slots :
	void rRemoveSelectedRow();
	void rDeleteSelectedRow();
	void rShowResourceGeneratorForSelectedRow();
	void rShowResourceGeneratorForAsset(const QUuid &rAssetId);
	void rResourceGeneratorAccepted();
	void rSidecarCompositionMapGeneratorAccepted();
	void rShowEssenceDescriptorForAsset(const QSharedPointer<AssetMxfTrack> &rAsset);
	void rCompositionGeneratorAccepted();
	void rCustomMenuRequested(QPoint pos);
	void rMapCurrentRowSelectionChanged(const QModelIndex &rCurrent, const QModelIndex &rPrevious);
	void rJobQueueFinished();
	void rJobQueueFinishedTargetFrames();
	void rImpViewDoubleClicked(const QModelIndex &rIndex);
	void rOpenCplTimeline();
	void rReinstallImp();
	//WR
	void rShowMetadata();
	void rShowEssenceDescriptor();
#ifdef APP5_ACES
	void rExtractTargetFrames();
#endif
	void SetMxfFile(const QStringList &rFiles);
	void SetMxfFileDirectory(const QString&);
	void rLoadRequest();
	void rShowSidecarCompositionMapGeneratorView();
	void rShowSidecarCompositionMapGeneratorEdit();
	//WR

	//---- SCM ---- 
	void SetScmFile(const QStringList &rFiles);
	void SetScmFileDirectory(const QString&);

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
	//WR
	bool mPartialOutgestInProgress;
	QString mPartialImpPath;
	QFileDialog *mpFileDialog;
	QList<QSharedPointer<ImfPackage> > mAdditionalPackages;
	QList<QSharedPointer<Asset> > mAdditionalAssets;
	QToolButton *mpButtonAddOv;
	QStringList mTargetFrameFileList;
	//WR
};
