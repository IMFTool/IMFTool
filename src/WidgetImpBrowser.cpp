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
#include "WidgetImpBrowser.h"
#include "global.h"
#include "ImfPackageCommands.h"
#include "WizardResourceGenerator.h"
#include "WizardSidecarCompositionMapGenerator.h"
#include "WizardCompositionGenerator.h"
#include "MetadataExtractor.h"
#include "DelegateMetadata.h"
#include "WidgetComposition.h"
#include "UndoProxyModel.h"
#include "JobQueue.h"
#include "Jobs.h"
#include <QStringList>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDesktopWidget>
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QKeyEvent>
#include <QProgressDialog>
#include <QToolBar>
#include <QSplitter>
#include <QDrag>
#include <QFileDialog>
#include <list>
#include <cmath>
#include <QTextEdit>
#include <QByteArray>
#include "WizardEssenceDescriptor.h"
#include "SMPTE-2067-9a-2018-Sidecar.h"

constexpr quint8 const UUIDVersion5::s_asset_id_prefix[UUIDVersion5::NS_ID_LENGTH];


WidgetImpBrowser::WidgetImpBrowser(QWidget *pParent /*= NULL*/) :
QFrame(pParent), mpViewImp(NULL), mpViewAssets(NULL), mpImfPackage(NULL), mpToolBar(NULL), mpUndoStack(NULL), mpUndoProxyModel(NULL), mpSortProxyModelImp(NULL), mpSortProxyModelAssets(NULL), mpMsgBox(NULL), mpJobQueue(NULL), mPartialOutgestInProgress(false) {

	setFrameStyle(QFrame::StyledPanel);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	mpUndoStack = new QUndoStack(this);
	mpJobQueue = new JobQueue(this);
	mpJobQueue->SetInterruptIfError(true);
	connect(mpJobQueue, SIGNAL(finished()), this, SLOT(rJobQueueFinished()));
	InitLayout();
	InitToolbar();
}

WidgetImpBrowser::~WidgetImpBrowser() {

	UninstallImp();
}

void WidgetImpBrowser::InitLayout() {

	mpMsgBox = new QMessageBox(this);
	mpMsgBox->setIcon(QMessageBox::Warning);

	mpProgressDialog = new QProgressDialog(this);
	mpProgressDialog->setWindowModality(Qt::WindowModal);
	mpProgressDialog->setMinimumSize(500, 150);
	mpProgressDialog->setMinimum(0);
	mpProgressDialog->setMaximum(100);
	mpProgressDialog->setValue(100);
	mpProgressDialog->setMinimumDuration(0);

	mpViewImp = new CustomTableView(this);
	mpViewImp->setContextMenuPolicy(Qt::CustomContextMenu);
	mpViewImp->setShowGrid(false);
	mpViewImp->setEditTriggers(QAbstractItemView::AllEditTriggers);
	mpViewImp->setSortingEnabled(true);
	mpViewImp->horizontalHeader()->setSectionsMovable(true);
	mpViewImp->horizontalHeader()->setHighlightSections(false);
	mpViewImp->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	mpViewImp->horizontalHeader()->setStretchLastSection(true);
	mpViewImp->verticalHeader()->setHidden(true);
	mpViewImp->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpViewImp->setSelectionBehavior(QAbstractItemView::SelectRows);
	mpViewImp->setSelectionMode(QAbstractItemView::SingleSelection);
	mpViewImp->setFrameShape(QFrame::NoFrame);
	mpViewImp->setDragEnabled(true);
	mpViewImp->setDragDropMode(QAbstractItemView::DragOnly);

	mpViewAssets = new CustomTableView(this);												//Asset Metadata View
	mpViewAssets->setShowGrid(false);
	mpViewAssets->horizontalHeader()->setSectionsMovable(true);
	mpViewAssets->horizontalHeader()->setHighlightSections(false);
	mpViewAssets->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpViewAssets->horizontalHeader()->setStretchLastSection(true);
	mpViewAssets->horizontalHeader()->setHidden(true);
	mpViewAssets->verticalHeader()->setHidden(true);
	mpViewAssets->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpViewAssets->setSelectionBehavior(QAbstractItemView::SelectRows);
	mpViewAssets->setSelectionMode(QAbstractItemView::SingleSelection);
	mpViewAssets->setFrameShape(QFrame::NoFrame);
	mpViewAssets->setDragEnabled(true);
	mpViewAssets->setDragDropMode(QAbstractItemView::DragOnly);

	mpViewAssets->setItemDelegateForColumn(ImfPackage::ColumnMetadata, new DelegateMetadata(this));

	mpToolBar = new QToolBar(tr("IMP Browser Toolbar"), this);
	mpToolBar->setIconSize(QSize(20, 20));

	QSplitter *p_splitter = new QSplitter(Qt::Vertical, this);
	p_splitter->setOpaqueResize(false);
	p_splitter->addWidget(mpViewImp);
	p_splitter->addWidget(mpViewAssets);

	QVBoxLayout *p_layout = new QVBoxLayout();
	p_layout->setMargin(0);
	p_layout->setSpacing(0);
	p_layout->addWidget(mpToolBar);
	p_layout->addWidget(p_splitter);
	setLayout(p_layout);

	connect(mpViewImp, SIGNAL(customContextMenuRequested(QPoint)), SLOT(rCustomMenuRequested(QPoint)));
	connect(mpJobQueue, SIGNAL(Progress(int)), mpProgressDialog, SLOT(setValue(int)));
	connect(mpJobQueue, SIGNAL(NextJobStarted(const QString&)), mpProgressDialog, SLOT(setLabelText(const QString&)));
	connect(mpProgressDialog, SIGNAL(canceled()), mpJobQueue, SLOT(InterruptQueue()));
	connect(mpViewImp, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(rImpViewDoubleClicked(const QModelIndex&)));
}

void WidgetImpBrowser::InitToolbar() {

	QAction *p_action_undo = mpUndoStack->createUndoAction(this, tr("Undo"));
	p_action_undo->setIcon(QIcon(":/undo.png"));
	QAction *p_action_redo = mpUndoStack->createRedoAction(this, tr("Redo"));
	p_action_redo->setIcon(QIcon(":/redo.png"));

	QToolButton *p_button_add_track = new QToolButton(NULL);
	p_button_add_track->setIcon(QIcon(":/add.png"));
	p_button_add_track->setText(tr("Add Asset"));
	p_button_add_track->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	p_button_add_track->setPopupMode(QToolButton::InstantPopup);
	p_button_add_track->setDisabled(true);
	connect(this, SIGNAL(ImplInstalled(bool)), p_button_add_track, SLOT(setEnabled(bool)));
	QMenu *p_add_track_menu = new QMenu(tr("Add Asset"), this);
	QAction *p_add_pcm_resource = p_add_track_menu->addAction(QIcon(":/sound.png"), tr("Wrap PCM Essence"));
	connect(p_add_pcm_resource, SIGNAL(triggered(bool)), this, SLOT(ShowResourceGeneratorWavMode()));
	QAction *p_add_ttml_resource = p_add_track_menu->addAction(QIcon(":/text.png"), tr("Wrap Timed Text Essence"));
	connect(p_add_ttml_resource, SIGNAL(triggered(bool)), this, SLOT(ShowResourceGeneratorTimedTextMode()));
	//p_add_ttml_resource->setDisabled(true);
	p_add_track_menu->addSeparator();
	QAction *p_add_cpl_asset = p_add_track_menu->addAction(QIcon(":/asset_cpl.png"), tr("Composition Playlist"));
	connect(p_add_cpl_asset, SIGNAL(triggered(bool)), this, SLOT(ShowCompositionGenerator()));
	p_add_track_menu->addSeparator();
	QAction *p_add_mxf_resource = p_add_track_menu->addAction(QIcon(":/asset_mxf.png"), tr("Add existing MXF Track File"));
	connect(p_add_mxf_resource, SIGNAL(triggered(bool)), this, SLOT(ShowResourceGeneratorMxfMode()));
	//p_add_mxf_resource->setDisabled(true);
	QAction *p_add_scm_resource = p_add_track_menu->addAction(QIcon(":/asset_scm.png"), tr("Add Sidecar Assets"));
	connect(p_add_scm_resource, SIGNAL(triggered(bool)), this, SLOT(ShowSidecarCompositionMapGenerator()));
	//p_add_scm_resource->setDisabled(true);

	p_button_add_track->setMenu(p_add_track_menu);

	mpButtonAddOv = new QToolButton(NULL);
	mpButtonAddOv->setIcon(QIcon(":/add.png"));
	mpButtonAddOv->setText(tr("Load OV IMP"));
	mpButtonAddOv->setToolTip("Load all MXF assets from an Original Version to enable full timeline view of Supplemental IMPs. Assets will not be added to the current IMP.");
	mpButtonAddOv->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	mpButtonAddOv->setPopupMode(QToolButton::InstantPopup);
	mpButtonAddOv->setDisabled(true);
	QMenu *p_add_ov_menu = new QMenu(tr("Add Asset"), this);
	QAction *p_add_ov = p_add_ov_menu->addAction(QIcon(":/asset_mxf.png"), tr("Select OV location"));
	connect(p_add_ov, SIGNAL(triggered(bool)), this, SLOT(rLoadRequest()));
	mpButtonAddOv->setMenu(p_add_ov_menu);

	mpToolBar->addAction(p_action_undo);
	mpToolBar->addAction(p_action_redo);
	mpToolBar->addSeparator();
	mpToolBar->addWidget(p_button_add_track);
	mpToolBar->addSeparator();
	mpToolBar->addWidget(mpButtonAddOv);
}

void WidgetImpBrowser::InstallImp(const QSharedPointer<ImfPackage> &rImfPackage, bool validateHash /*= false*/) {

	UninstallImp();
	mpImfPackage = rImfPackage;
	mpUndoProxyModel = new UndoProxyModel(mpUndoStack, this);
	mpUndoProxyModel->setSourceModel(mpImfPackage.data()); // WARNING: The ImfPackage shared pointer mustn't be lost.

	mpSortProxyModelImp = new QSortFilterProxyModel(this);
	mpSortProxyModelImp->setFilterRegExp("(?:mxf|cpl|opl|unknown)");
	mpSortProxyModelImp->setFilterKeyColumn(ImfPackage::ColumnAssetType);
	mpSortProxyModelImp->setSourceModel(mpUndoProxyModel);

	mpViewImp->setModel(mpSortProxyModelImp);
	mpViewImp->setColumnHidden(ImfPackage::ColumnAssetType, true);
	mpViewImp->setColumnHidden(ImfPackage::ColumnMetadata, true);
	mpViewImp->setColumnHidden(ImfPackage::ColumnProxyImage, true);
	mpViewImp->horizontalHeader()->setSectionResizeMode(ImfPackage::ColumnIcon, QHeaderView::ResizeToContents);
	mpViewImp->horizontalHeader()->setSectionResizeMode(ImfPackage::ColumnFileSize, QHeaderView::ResizeToContents);
	mpViewImp->horizontalHeader()->setSectionResizeMode(ImfPackage::ColumnFinalized, QHeaderView::ResizeToContents);
	connect(mpViewImp->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(slotCurrentChanged(const QModelIndex &, const QModelIndex &)));

	mpSortProxyModelAssets = new QSortFilterProxyModel(this);
	mpSortProxyModelAssets->setFilterRegExp("(?:mxf)");
	mpSortProxyModelAssets->setFilterKeyColumn(ImfPackage::ColumnAssetType);
	mpSortProxyModelAssets->setSourceModel(mpUndoProxyModel);

	mpViewAssets->setModel(mpSortProxyModelAssets);
	mpViewAssets->setAutoScroll(true);
	mpViewAssets->setColumnHidden(ImfPackage::ColumnIcon, true);
	mpViewAssets->setColumnHidden(ImfPackage::ColumnAssetType, true);
	mpViewAssets->setColumnHidden(ImfPackage::ColumnFilePath, true);
	mpViewAssets->setColumnHidden(ImfPackage::ColumnFileSize, true);
	mpViewAssets->setColumnHidden(ImfPackage::ColumnFinalized, true);
	mpViewAssets->setColumnHidden(ImfPackage::ColumnAnnotation, true);

	connect(mpViewAssets->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(rMapCurrentRowSelectionChanged(const QModelIndex&, const QModelIndex&)));
	connect(mpViewImp->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(rMapCurrentRowSelectionChanged(const QModelIndex&, const QModelIndex&)));
	connect(mpImfPackage.data(), SIGNAL(DirtyChanged(bool)), this, SIGNAL(ImpSaveStateChanged(bool)));
	connect(mpImfPackage.data(), SIGNAL(rIsSupplemental(bool)), mpButtonAddOv, SLOT(setEnabled(bool)));
	if (mpImfPackage->GetIsSupplemental()) {
		emit mpImfPackage.data()->rIsSupplemental(true);
	}
	emit ImplInstalled(true);
	emit ImpSaveStateChanged(mpImfPackage->IsDirty());
	if(validateHash == true) ValidateHash();
}

void WidgetImpBrowser::UninstallImp() {

	/* if (mpImfPackage.data() != nullptr) {
		disconnect(mpImfPackage.data(), NULL, this, NULL);
	} (k) */
	mpButtonAddOv->setEnabled(false);

	disconnect(mpViewAssets->selectionModel(), NULL, this, NULL);
	//disconnect(mpViewImp, NULL, this, NULL);
	disconnect(mpImfPackage.data(), NULL, this, NULL);
	emit ImplInstalled(false);
	emit ImpSaveStateChanged(false);
	mpUndoStack->clear();
	delete mpUndoProxyModel;
	mpUndoProxyModel = NULL;
	delete mpSortProxyModelImp;
	mpSortProxyModelImp = NULL;
	delete mpSortProxyModelAssets;
	mpSortProxyModelAssets = NULL;
	QItemSelectionModel *selection_model = mpViewImp->selectionModel();
	mpViewImp->setModel(NULL);
	delete selection_model; // Delete old selection model.
	selection_model = mpViewAssets->selectionModel();
	mpViewAssets->setModel(NULL);
	delete selection_model; // Delete old selection model.
	mpImfPackage.clear();
	//WR
	mAdditionalPackages.clear();
	mAdditionalAssets.clear();

}

QSize WidgetImpBrowser::sizeHint() const {

	QRect size = QApplication::desktop()->availableGeometry();
	return QSize(size.width() * 0.20, 200);
}

QSize WidgetImpBrowser::minimumSizeHint() const {

	return QSize(300, 100);
}

void WidgetImpBrowser::rCustomMenuRequested(QPoint pos) {

	if(mpImfPackage) {
		QMenu menu;
		QModelIndex index = mpViewImp->indexAt(pos);

		// Over item.
		if(index.isValid() == true) {

			QAction *remove_action = new QAction(QIcon(":/delete.png"), tr("Remove selected Asset from IMP"), this);
			//QAction *delete_action = new QAction(QIcon(":/close.png"), tr("Delete selected Asset from IMP and from disk"), this);
			//edit_action disabled, because Empty TT cannot be added afterwards AND assets already dragged into a timeline don't get updated after editing
			QAction *view_scm_action = new QAction(QIcon(":/information.png"), tr("View SCM"), this);
			QAction *edit_scm_action = new QAction(QIcon(":/edit.png"), tr("Edit SCM"), this);
			QAction *open_action = new QAction(QIcon(":/edit.png"), tr("Open CPL"), this);
			QAction *view_metadata_action = new QAction(QIcon(":/information.png"), tr("View metadata"), this);
			QAction *view_essence_descriptor_action = new QAction(QIcon(":/information.png"), tr("View essence descriptor"), this);
			connect(remove_action, SIGNAL(triggered(bool)), this, SLOT(rRemoveSelectedRow()));
			//connect(delete_action, SIGNAL(triggered(bool)), this, SLOT(rDeleteSelectedRow()));
			connect(view_scm_action, SIGNAL(triggered(bool)), this, SLOT(rShowSidecarCompositionMapGeneratorView()));
			connect(edit_scm_action, SIGNAL(triggered(bool)), this, SLOT(rShowSidecarCompositionMapGeneratorEdit()));
			connect(open_action, SIGNAL(triggered(bool)), this, SLOT(rOpenCplTimeline()));
			//connect(edit_action, SIGNAL(triggered(bool)), this, SLOT(rShowResourceGeneratorForSelectedRow()));
			connect(view_metadata_action, SIGNAL(triggered(bool)), this, SLOT(rShowMetadata()));
			connect(view_essence_descriptor_action, SIGNAL(triggered(bool)), this, SLOT(rShowEssenceDescriptor()));
			menu.addAction(remove_action);
			//menu.addAction(delete_action);
			//menu.addSeparator();
			//menu.addAction(edit_action);
			menu.addSeparator();
			QSharedPointer<Asset> asset = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(index).row());
			if(asset) {
				if (asset->GetIsOutsidePackage()) remove_action->setDisabled(true);
				//if(asset->Exists() == true) edit_action->setDisabled(true);
				//if(asset->Exists() == false) delete_action->setDisabled(true);
				if(asset->Exists() == true && asset->GetType() == Asset::cpl)
					menu.addAction(open_action);
				if(asset->GetType() == Asset::mxf) {
					QSharedPointer <AssetMxfTrack> assetMxfTrack = qSharedPointerCast<AssetMxfTrack>(asset);
					if (asset->Exists() || assetMxfTrack->HasSourceFiles())
						menu.addAction(view_metadata_action);
					if(asset->Exists()) menu.addAction(view_essence_descriptor_action);
				}
				if(asset->GetType() == Asset::scm) {
					menu.addAction(view_scm_action);
					menu.addAction(edit_scm_action);
				}
			}
		}
		menu.exec(mpViewImp->viewport()->mapToGlobal(pos));
	}
}

void WidgetImpBrowser::rRemoveSelectedRow() {

	if(mpImfPackage) {
		// Don't allow deleting assets that do not belong to the current IMP
		if (!mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row())->GetIsOutsidePackage())
		mpUndoStack->push(new RemoveAssetCommand(mpImfPackage, mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row())));
	}
}

void WidgetImpBrowser::keyPressEvent(QKeyEvent *pEvent) {

	if(pEvent->key() == Qt::Key_Delete) {
		rRemoveSelectedRow();
		return;
	}
	QWidget::keyPressEvent(pEvent);
}

void WidgetImpBrowser::Save() {

	if(mpImfPackage) {
		mpMsgBox->setText(tr("Start Outgest?"));
		mpMsgBox->setInformativeText(tr("All changes will be written on the file system and can not be undone. Proceed?"));
		mpMsgBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Abort);
		mpMsgBox->setDefaultButton(QMessageBox::Abort);
		mpMsgBox->setIcon(QMessageBox::Information);
		int ret = mpMsgBox->exec();
		if(ret == QMessageBox::Ok) {
			mpJobQueue->FlushQueue();
			for(int i = 0; i < mpImfPackage->GetAssetCount(); i++) {
				QSharedPointer<AssetMxfTrack> mxf_asset = mpImfPackage->GetAsset(i).objectCast<AssetMxfTrack>();
				if((!mxf_asset.isNull()) && (!mxf_asset->GetIsOutsidePackage()) && mxf_asset->Exists() == false) {
					if(mxf_asset->GetEssenceType() == Metadata::Pcm) {
						JobWrapWav *p_wrap_job = new JobWrapWav(mxf_asset->GetSourceFiles(), mxf_asset->GetPath().absoluteFilePath(), mxf_asset->GetSoundfieldGroup(), mxf_asset->GetId(), mxf_asset->GetLanguageTag(), mxf_asset->GetMCATitle(), mxf_asset->GetMCATitleVersion(), mxf_asset->GetMCAAudioContentKind(), mxf_asset->GetMCAAudioElementKind());
						connect(p_wrap_job, SIGNAL(Success()), mxf_asset.data(), SLOT(FileModified()));
						mpJobQueue->AddJob(p_wrap_job);
					}


						/* -----Denis Manthey Beg----- */
					else if(mxf_asset->GetEssenceType() == Metadata::TimedText) {
						JobWrapTimedText *p_wrap_job = new JobWrapTimedText(mxf_asset->GetSourceFiles(), mxf_asset->GetPath().absoluteFilePath(), mxf_asset->GetEditRate(), mxf_asset->GetDuration(), mxf_asset->GetId(), mxf_asset->GetProfile(), mxf_asset->GetLanguageTag());
						connect(p_wrap_job, SIGNAL(Success()), mxf_asset.data(), SLOT(FileModified()));
						mpJobQueue->AddJob(p_wrap_job);
					}
						/* -----Denis Manthey End----- */

				}
				QSharedPointer<AssetScm> scm_asset = mpImfPackage->GetAsset(i).objectCast<AssetScm>();
				if(scm_asset && scm_asset->Exists() == false) {
					JobCreateScm *p_scm_job = new JobCreateScm(scm_asset);
					connect(p_scm_job, SIGNAL(Success()), scm_asset.data(), SLOT(FileModified()));
					mpJobQueue->AddJob(p_scm_job);
				}

				QSharedPointer<Asset> abstract_asset = mpImfPackage->GetAsset(i);
				if(abstract_asset && abstract_asset->NeedsNewHash() && abstract_asset->GetType() != Asset::pkl) {
					JobCalculateHash *p_hash_job = new JobCalculateHash(abstract_asset->GetPath().absoluteFilePath());
					connect(p_hash_job, SIGNAL(Result(const QByteArray&, const QVariant&)), abstract_asset.data(), SLOT(SetHash(const QByteArray&)));
					mpJobQueue->AddJob(p_hash_job);
				}
			}
			mpJobQueue->StartQueue();
			//connect(mpJobQueue, SIGNAL(finished()), this, SLOT(rReinstallImp()));
		}
	}
}

						/* -----Denis Manthey Beg----- */
void WidgetImpBrowser::rReinstallImp() {

	emit WritePackageComplete();
}
						/* -----Denis Manthey End----- */

void WidgetImpBrowser::ShowResourceGeneratorWavMode() {

	WizardResourceGenerator *p_wizard_resource_generator = new WizardResourceGenerator(this);
	p_wizard_resource_generator->setAttribute(Qt::WA_DeleteOnClose, true);
	p_wizard_resource_generator->SwitchMode(WizardResourceGenerator::WavMode);
	p_wizard_resource_generator->resize( QSize(600, 600).expandedTo(minimumSizeHint()) );
	p_wizard_resource_generator->show();
	connect(p_wizard_resource_generator, SIGNAL(accepted()), this, SLOT(rResourceGeneratorAccepted()));
}


		/* -----Denis Manthey Beg----- */
void WidgetImpBrowser::ShowResourceGeneratorTimedTextMode() {

	WizardResourceGenerator *p_wizard_resource_generator = new WizardResourceGenerator(this, mpImfPackage->GetImpEditRates());
	p_wizard_resource_generator->setAttribute(Qt::WA_DeleteOnClose, true);
	p_wizard_resource_generator->SwitchMode(WizardResourceGenerator::TTMLMode);
	p_wizard_resource_generator->resize( QSize(600, 500).expandedTo(minimumSizeHint()) );
	p_wizard_resource_generator->show();
	connect(p_wizard_resource_generator, SIGNAL(accepted()), this, SLOT(rResourceGeneratorAccepted()));
}
		/* -----Denis Manthey End----- */

		/* -----Sidecar Composition Map----- */
void WidgetImpBrowser::ShowSidecarCompositionMapGenerator(QSharedPointer<AssetScm> rAssetScm /* = QSharedPointer<AssetScm>() */,
		WizardSidecarCompositionMapGenerator::eMode rMode /*= WizardSidecarCompositionMapGenerator::eMode::NewScm*/) {
	QVector<QSharedPointer<AssetCpl>> CPLfiles;

	for (int i = 0; i < mpImfPackage->GetAssetCount(); i++) {
		if (mpImfPackage->GetAsset(i)->GetType() == Asset::cpl) {
			CPLfiles.append(mpImfPackage->GetAsset(i).objectCast<AssetCpl>());
		}
	}
	WizardSidecarCompositionMapGenerator *p_wizard_scm_generator;
	switch (rMode) {
	case WizardSidecarCompositionMapGenerator::eMode::NewScm://default value for rMode
		p_wizard_scm_generator = new WizardSidecarCompositionMapGenerator(this, mpImfPackage, CPLfiles);
		p_wizard_scm_generator->SwitchMode(WizardSidecarCompositionMapGenerator::NewScm);
		break;
	case WizardSidecarCompositionMapGenerator::eMode::EditScm:
		p_wizard_scm_generator = new WizardSidecarCompositionMapGenerator(this, mpImfPackage, CPLfiles, rAssetScm);
		p_wizard_scm_generator->SwitchMode(WizardSidecarCompositionMapGenerator::EditScm);
		break;
	case WizardSidecarCompositionMapGenerator::eMode::ViewScm:
		p_wizard_scm_generator = new WizardSidecarCompositionMapGenerator(this, mpImfPackage, CPLfiles, rAssetScm);
		p_wizard_scm_generator->SwitchMode(WizardSidecarCompositionMapGenerator::ViewScm);
		break;
	default:
		break;
	}
	p_wizard_scm_generator->setAttribute(Qt::WA_DeleteOnClose, true);
	p_wizard_scm_generator->resize(QSize(600, 600).expandedTo(minimumSizeHint()));
	p_wizard_scm_generator->show();
	connect(p_wizard_scm_generator, SIGNAL(accepted()), this, SLOT(rSidecarCompositionMapGeneratorAccepted()));
}

//WR
void WidgetImpBrowser::ShowResourceGeneratorMxfMode() {
	mpFileDialog = new QFileDialog(this, QString("Select MXF file"), mpImfPackage->GetRootDir().absolutePath());
	mpFileDialog->setOption(QFileDialog::DontUseNativeDialog);
	mpFileDialog->setOption(QFileDialog::DontUseCustomDirectoryIcons);
	mpFileDialog->setFileMode(QFileDialog::ExistingFile);
	mpFileDialog->setViewMode(QFileDialog::Detail);
	mpFileDialog->setNameFilters(QStringList() << "*.mxf");
	mpFileDialog->setIconProvider(new IconProviderExrWav(this));
	connect(mpFileDialog, SIGNAL(directoryEntered(const QString&)), this, SLOT(SetMxfFileDirectory(const QString&)));
	connect(mpFileDialog, SIGNAL(filesSelected(const QStringList &)), this, SLOT(SetMxfFile(const QStringList &)));
    QToolButton* backButton = mpFileDialog->findChild<QToolButton *>("backButton");
    QToolButton* forwardButton = mpFileDialog->findChild<QToolButton *>("forwardButton");
    backButton->setVisible(false);
    forwardButton->setVisible(false);

	mpFileDialog->show();
}
//WR

void WidgetImpBrowser::ShowCompositionGenerator() {

	EditRate edit_rate = EditRate::EditRate23_98;
	QStringList application_identification_cpl;
	bool found = false;
	for (int ii = 0; ii < mpImfPackage->GetAssetCount(); ii++) {
		if (!found) {
			if (!mpImfPackage->GetAsset(ii).isNull()) {
				QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(ii).objectCast<AssetCpl>();

				if (asset_cpl) {
					QString file_path = asset_cpl->GetPath().absoluteFilePath();
					if(QFile::exists(file_path)) {
						std::auto_ptr< cpl2016::CompositionPlaylistType> cpl_data;
						std::auto_ptr< cpl::CompositionPlaylistType> cpl2013_data;
						try { cpl_data = cpl2016::parseCompositionPlaylist(file_path.toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
						catch(...) {  }
						try { cpl2013_data = cpl::parseCompositionPlaylist(file_path.toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
						catch(...) {  }
						if (cpl_data.get()) {
							edit_rate = ImfXmlHelper::Convert(cpl_data->getEditRate());
							if (cpl_data->getExtensionProperties().present()) {
								cpl2016::CompositionPlaylistType_ExtensionPropertiesType sequence_list = cpl_data->getExtensionProperties().get();
								cpl2016::CompositionPlaylistType_ExtensionPropertiesType::AnySequence &r_any_sequence(sequence_list.getAny());
								for(cpl2016::CompositionPlaylistType_ExtensionPropertiesType::AnySequence::iterator sequence_iter(r_any_sequence.begin()); sequence_iter != r_any_sequence.end(); ++sequence_iter) {
									if (QString(XMLString::transcode(sequence_iter->getNodeName())).contains("ApplicationIdentification")) {
										application_identification_cpl << QString(XMLString::transcode(sequence_iter->getTextContent()));
									}
								}
							}
							found = true;
						} else if (cpl2013_data.get()) {
							edit_rate = ImfXmlHelper::Convert(cpl2013_data->getEditRate());
							if (cpl2013_data->getExtensionProperties().present()) {
								cpl::CompositionPlaylistType_ExtensionPropertiesType sequence_list = cpl2013_data->getExtensionProperties().get();
								cpl::CompositionPlaylistType_ExtensionPropertiesType::AnySequence &r_any_sequence(sequence_list.getAny());
								for(cpl::CompositionPlaylistType_ExtensionPropertiesType::AnySequence::iterator sequence_iter(r_any_sequence.begin()); sequence_iter != r_any_sequence.end(); ++sequence_iter) {
									if (QString(XMLString::transcode(sequence_iter->getNodeName())).contains("ApplicationIdentification")) {
										application_identification_cpl << QString(XMLString::transcode(sequence_iter->getTextContent()));
									}
								}
							}
							found = true;
						}
					}
				}

			}
		}
	}
	WizardCompositionGenerator *p_wizard_composition_generator = new WizardCompositionGenerator(this, edit_rate, application_identification_cpl);
	p_wizard_composition_generator->setAttribute(Qt::WA_DeleteOnClose, true);
	p_wizard_composition_generator->show();
	connect(p_wizard_composition_generator, SIGNAL(accepted()), this, SLOT(rCompositionGeneratorAccepted()));
}

void WidgetImpBrowser::rShowResourceGeneratorForSelectedRow() {

	if(mpImfPackage) {
		QSharedPointer<Asset> asset = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row());
		if(asset) rShowResourceGeneratorForAsset(asset->GetId());
	}
}

void WidgetImpBrowser::rShowResourceGeneratorForAsset(const QUuid &rAssetId) {

	if(mpImfPackage) {
		QSharedPointer<AssetMxfTrack> asset = mpImfPackage->GetAsset(rAssetId).objectCast<AssetMxfTrack>();
		if(asset) { // && asset->Exists() == false) {
			WizardResourceGenerator *p_wizard_resource_generator;
			if (asset->Exists() == false)
				p_wizard_resource_generator = new WizardResourceGenerator(this, mpImfPackage->GetImpEditRates());
			else
				p_wizard_resource_generator = new WizardResourceGenerator(this, mpImfPackage->GetImpEditRates(), asset);
			p_wizard_resource_generator->setAttribute(Qt::WA_DeleteOnClose, true);
			p_wizard_resource_generator->setField(FIELD_NAME_SELECTED_FILES, QVariant(asset->GetSourceFiles()));
			p_wizard_resource_generator->setField(FIELD_NAME_SOUNDFIELD_GROUP, QVariant::fromValue<SoundfieldGroup>(asset->GetSoundfieldGroup()));
			p_wizard_resource_generator->setField(FIELD_NAME_EDIT_RATE, QVariant::fromValue<EditRate>(asset->GetEditRate()));
			//WR
			p_wizard_resource_generator->setField(FIELD_NAME_LANGUAGETAG_WAV, QVariant::fromValue<QString>(asset->GetLanguageTag()));
			p_wizard_resource_generator->setField(FIELD_NAME_LANGUAGETAG_TT, QVariant(asset->GetLanguageTag()));
			p_wizard_resource_generator->setField(FIELD_NAME_MCA_TITLE, QVariant(asset->GetMCATitle()));
			p_wizard_resource_generator->setField(FIELD_NAME_MCA_TITLE_VERSION, QVariant(asset->GetMCATitleVersion()));
			p_wizard_resource_generator->setField(FIELD_NAME_MCA_AUDIO_CONTENT_KIND, QVariant(asset->GetMCAAudioContentKind()));
			p_wizard_resource_generator->setField(FIELD_NAME_MCA_AUDIO_ELEMENT_KIND, QVariant(asset->GetMCAAudioElementKind()));
			p_wizard_resource_generator->setField(FIELD_NAME_CPL_EDIT_RATE, QVariant::fromValue<EditRate>(asset->GetCplEditRate()));
			//WR
			p_wizard_resource_generator->setProperty(ASSET_ID_DYNAMIK_PROPERTY, QVariant(asset->GetId()));
			p_wizard_resource_generator->show();
			connect(p_wizard_resource_generator, SIGNAL(accepted()), this, SLOT(rResourceGeneratorAccepted()));
		}
	}
}

void WidgetImpBrowser::rShowEssenceDescriptorForAsset(const QSharedPointer<AssetMxfTrack> &rAsset) {

	if(rAsset) {
		WizardEssenceDescriptor *p_wizard_essence_descriptor= new WizardEssenceDescriptor(this,rAsset);
		p_wizard_essence_descriptor->setAttribute(Qt::WA_DeleteOnClose, true);
		p_wizard_essence_descriptor->show();
		}
}


void WidgetImpBrowser::rResourceGeneratorAccepted() {

	WizardResourceGenerator *p_resource_generator = qobject_cast<WizardResourceGenerator *>(sender());
	if(p_resource_generator) {
		QStringList selected_files = p_resource_generator->field(FIELD_NAME_SELECTED_FILES).toStringList();
		SoundfieldGroup soundfield_group = qvariant_cast<SoundfieldGroup>(p_resource_generator->field(FIELD_NAME_SOUNDFIELD_GROUP));
		EditRate edit_rate = qvariant_cast<EditRate>(p_resource_generator->field(FIELD_NAME_EDIT_RATE));
		Duration duration = qvariant_cast<Duration>(p_resource_generator->field(FIELD_NAME_DURATION));
		//WR
		QString language_tag_wav = qvariant_cast<QString>(p_resource_generator->field(FIELD_NAME_LANGUAGETAG_WAV));
		QString language_tag_tt = qvariant_cast<QString>(p_resource_generator->field(FIELD_NAME_LANGUAGETAG_TT));
		QString mca_title = qvariant_cast<QString>(p_resource_generator->field(FIELD_NAME_MCA_TITLE));
		QString mca_title_version = qvariant_cast<QString>(p_resource_generator->field(FIELD_NAME_MCA_TITLE_VERSION));
		QString mca_audio_content_kind = qvariant_cast<QString>(p_resource_generator->field(FIELD_NAME_MCA_AUDIO_CONTENT_KIND));
		QString mca_audio_element_kind = qvariant_cast<QString>(p_resource_generator->field(FIELD_NAME_MCA_AUDIO_ELEMENT_KIND));
		EditRate cpl_edit_rate = qvariant_cast<EditRate>(p_resource_generator->field(FIELD_NAME_CPL_EDIT_RATE));
		//WR
		if(mpImfPackage) {
			QVariant asset_id = p_resource_generator->property(ASSET_ID_DYNAMIK_PROPERTY);
			// New Asset
			if(asset_id.isValid() == false) {
				if(selected_files.isEmpty() == false && is_wav_file(selected_files.first())) {
					QUuid id = QUuid::createUuid();
					QString file_name = QString("WAV_%1.mxf").arg(strip_uuid(id));
					QFileInfo mxf_asset_file_path(mpImfPackage->GetRootDir().absoluteFilePath(file_name));
					QSharedPointer<AssetMxfTrack> mxf_asset(new AssetMxfTrack(mxf_asset_file_path, id));
					mxf_asset->SetSourceFiles(selected_files);
					mxf_asset->SetSoundfieldGroup(soundfield_group);
					//WR
					mxf_asset->SetLanguageTag(language_tag_wav);
					mxf_asset->SetMCATitle(mca_title);
					mxf_asset->SetMCATitleVersion(mca_title_version);
					mxf_asset->SetMCAAudioContentKind(mca_audio_content_kind);
					mxf_asset->SetMCAAudioElementKind(mca_audio_element_kind);
					//WR
					mpUndoStack->push(new AddAssetCommand(mpImfPackage, mxf_asset, mpImfPackage->GetPackingListId()));
				}



						/* -----Denis Manthey Beg----- */
				else if(selected_files.isEmpty() == false && is_ttml_file(selected_files.first())) {
					QUuid id = QUuid::createUuid();
					QString file_name = QString("TimedText_%1.mxf").arg(strip_uuid(id));
					QFileInfo mxf_asset_file_path(mpImfPackage->GetRootDir().absoluteFilePath(file_name));
					QSharedPointer<AssetMxfTrack> mxf_asset(new AssetMxfTrack(mxf_asset_file_path, id));
					mxf_asset->SetCplEditRate(cpl_edit_rate);
					mxf_asset->SetSourceFiles(selected_files);
					if (mxf_asset->GetDuration().GetCount() == 0) {
						mxf_asset->SetDuration(duration);
					}
					mxf_asset->SetLanguageTag(language_tag_tt);
					//WR
					mpUndoStack->push(new AddAssetCommand(mpImfPackage, mxf_asset, mpImfPackage->GetPackingListId()));
				}
						/* -----Denis Manthey End----- */


			}
			// Asset change request
			else {
				QSharedPointer<AssetMxfTrack> mxf_asset = mpImfPackage->GetAsset(asset_id.toUuid()).objectCast<AssetMxfTrack>();
				if(mxf_asset) {
					mxf_asset->SetSourceFiles(selected_files);
					mxf_asset->SetSoundfieldGroup(soundfield_group);
					//WR
					if (is_wav_file(selected_files.first())) {
						mxf_asset->SetLanguageTag(language_tag_wav);
						mxf_asset->SetMCATitle(mca_title);
						mxf_asset->SetMCATitleVersion(mca_title_version);
						mxf_asset->SetMCAAudioContentKind(mca_audio_content_kind);
						mxf_asset->SetMCAAudioElementKind(mca_audio_element_kind);
					}
					if (is_ttml_file(selected_files.first())) {
						mxf_asset->SetLanguageTag(language_tag_tt);
						mxf_asset->SetCplEditRate(cpl_edit_rate);
					}
					//WR
				}
			}
		}
	}
}

// This slot is triggered by SIGNAL(accepted) from Wizard (FINISH button)
void WidgetImpBrowser::rSidecarCompositionMapGeneratorAccepted() {

	WizardSidecarCompositionMapGenerator *p_sidecar_composition_map_generator = qobject_cast<WizardSidecarCompositionMapGenerator *>(sender());
	if(p_sidecar_composition_map_generator) {
		QList<AssetScm::SidecarCompositionMapEntry*> sidecar_composition_map_entries = p_sidecar_composition_map_generator->getSidecarCompositionMapEntryList();
		WizardSidecarCompositionMapGenerator::eMode mode = p_sidecar_composition_map_generator->GetMode();
		QUuid scm_id;
		QSharedPointer<AssetScm> asset_scm;
		QFileInfo file_name;
		QString annotation_text, issuer;
		switch (mode) {

		case WizardSidecarCompositionMapGenerator::NewScm:
		case WizardSidecarCompositionMapGenerator::EditScm:
			scm_id = QUuid::createUuid();
			annotation_text = p_sidecar_composition_map_generator->getAnnotation();
			issuer = p_sidecar_composition_map_generator->getIssuer();
			file_name = QFileInfo(mpImfPackage->GetRootDir().absoluteFilePath(QString("SCM_%1.xml").arg(strip_uuid(scm_id))));
			asset_scm = QSharedPointer<AssetScm>(new AssetScm(file_name, scm_id, annotation_text));
			asset_scm->SetIsNew(true);
			asset_scm->SetIssuer(UserText(issuer));
			for (int i=0; i < sidecar_composition_map_entries.size(); i++) {
				QFileInfo sidecar_file_path = sidecar_composition_map_entries[i]->filepath;
				Error hash_error = Error(UUIDVersion5::CalculateFromEntireFile(
						UUIDVersion5::s_asset_id_prefix,
						sidecar_file_path.absoluteFilePath(),
						sidecar_composition_map_entries[i]->id)
				);
				if (!hash_error.IsError() && !hash_error.IsRecoverableError()) {
					if (mpImfPackage->GetAsset(sidecar_composition_map_entries[i]->id).isNull()) {
						QSharedPointer<AssetSidecar> sidecar_asset(new AssetSidecar(sidecar_file_path, sidecar_composition_map_entries[i]->id));
						sidecar_asset->GetPklData()->setType(MediaType::GetMediaType(sidecar_file_path));
						if (sidecar_asset) {
							mpUndoStack->push(new AddAssetCommand(mpImfPackage, sidecar_asset, mpImfPackage->GetPackingListId()));

						}
					}
					asset_scm->AddSidecarCompositionMapEntry(*sidecar_composition_map_entries[i]);
			    }
			}
			mpUndoStack->push(new AddAssetCommand(mpImfPackage, asset_scm, mpImfPackage->GetPackingListId()));
			if ( mode == WizardSidecarCompositionMapGenerator::NewScm) break;

			mpUndoStack->push(new RemoveAssetCommand(mpImfPackage, mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row())));
			break;
		case WizardSidecarCompositionMapGenerator::ViewScm: // Do nothing when in View mode
		default:
			break;
		}


	}
}



				/* -----Denis Manthey Beg----- */
QSharedPointer<AssetCpl> WidgetImpBrowser::GenerateEmptyCPL() {
	EditRate edit_rate;
	QString title;
	QString issuer;
	QString content_originator;
	QUuid id = QUuid::createUuid();
	QString file_name = QString("CPL_%1.xml").arg(strip_uuid(id));
	QFileInfo cpl_file_path(mpImfPackage->GetRootDir().absoluteFilePath(file_name));
	QSharedPointer<AssetCpl> cpl_asset(new AssetCpl(cpl_file_path, id));
	cpl_asset->SetIsNew(true);
	XmlSerializationError error = WidgetComposition::WriteMinimal(cpl_file_path.absoluteFilePath(), id, edit_rate, title, issuer, content_originator);
	if(error.IsError() == false)
		mpUndoStack->push(new AddAssetCommand(mpImfPackage, cpl_asset, mpImfPackage->GetPackingListId()));
	else {
		QString error_string = error.GetErrorMsg();
		error.AppendErrorDescription(error_string);
		mpMsgBox->setText(tr("XML Serialization Error"));
		mpMsgBox->setInformativeText(error_string);
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->setIcon(QMessageBox::Critical);
		mpMsgBox->exec();
	}
	return cpl_asset;
}
				/* -----Denis Manthey End----- */



void WidgetImpBrowser::rCompositionGeneratorAccepted() {

	WizardCompositionGenerator *p_composition_generator = qobject_cast<WizardCompositionGenerator *>(sender());
	if(p_composition_generator) {
		EditRate edit_rate = qvariant_cast<EditRate>(p_composition_generator->field(FIELD_NAME_EDIT_RATE));
		QString title = qvariant_cast<QString>(p_composition_generator->field(FIELD_NAME_TITLE));
		QString issuer = qvariant_cast<QString>(p_composition_generator->field(FIELD_NAME_ISSUER));
		QString content_originator = qvariant_cast<QString>(p_composition_generator->field(FIELD_NAME_CONTENT_ORIGINATOR));
		QString application_identification = qvariant_cast<QString>(p_composition_generator->field(FIELD_NAME_APP));
		if(mpImfPackage) {
			QUuid id = QUuid::createUuid();
			QString file_name = QString("CPL_%1.xml").arg(strip_uuid(id));
			QFileInfo mxf_cpl_file_path(mpImfPackage->GetRootDir().absoluteFilePath(file_name));
			QSharedPointer<AssetCpl> cpl_asset(new AssetCpl(mxf_cpl_file_path, id));
			cpl_asset->SetIsNew(true);
			cpl_asset->SetIsNewOrModified(true);
			XmlSerializationError error = WidgetComposition::WriteMinimal(mxf_cpl_file_path.absoluteFilePath(), id, edit_rate, title, issuer, content_originator, application_identification);
			if(error.IsError() == false) mpUndoStack->push(new AddAssetCommand(mpImfPackage, cpl_asset, mpImfPackage->GetPackingListId()));
			else {
				QString error_string = error.GetErrorMsg();
				error.AppendErrorDescription(error_string);
				mpMsgBox->setText(tr("XML Serialization Error"));
				mpMsgBox->setInformativeText(error_string);
				mpMsgBox->setStandardButtons(QMessageBox::Ok);
				mpMsgBox->setDefaultButton(QMessageBox::Ok);
				mpMsgBox->setIcon(QMessageBox::Critical);
				mpMsgBox->exec();
			}
		}
	}
}

void WidgetImpBrowser::rMapCurrentRowSelectionChanged(const QModelIndex &rCurrent, const QModelIndex &rPrevious) {

	QItemSelectionModel *p_source_selection_model = qobject_cast<QItemSelectionModel*>(sender());
	if(p_source_selection_model) {
		if(p_source_selection_model == mpViewImp->selectionModel()) {
			if(mpSortProxyModelImp && mpSortProxyModelAssets && mpViewAssets->selectionModel() && rCurrent.isValid()) {
				QModelIndex intermediate_index = mpSortProxyModelImp->mapToSource(rCurrent);
				QModelIndex target_index = mpSortProxyModelAssets->mapFromSource(intermediate_index);
				if(target_index.isValid() == true) {
					mpViewAssets->selectRow(target_index.row());
					mpViewAssets->scrollTo(target_index, QAbstractItemView::EnsureVisible);
				}
				else {
					mpViewAssets->clearSelection();
				}
			}
		}
		else if(p_source_selection_model == mpViewAssets->selectionModel()) {
			if(mpSortProxyModelImp && mpSortProxyModelAssets && mpViewImp->selectionModel() && rCurrent.isValid()) {
				QModelIndex intermediate_index = mpSortProxyModelAssets->mapToSource(rCurrent);
				QModelIndex target_index = mpSortProxyModelImp->mapFromSource(intermediate_index);
				if(target_index.isValid() == true) {
					mpViewImp->selectRow(target_index.row());
					mpViewImp->scrollTo(target_index, QAbstractItemView::EnsureVisible);
				}
				else {
					mpViewImp->clearSelection();
				}
			}
		}
		else {
			qWarning() << "Couldn't map selection models.";
		}
	}
}

void WidgetImpBrowser::rJobQueueFinished() {

	mpProgressDialog->reset();
	QString error_msg;
	QList<Error> errors = mpJobQueue->GetErrors();
	for(int i = 0; i < errors.size(); i++) {
		error_msg.append(QString("%1: %2\n%3\n").arg(i + 1).arg(errors.at(i).GetErrorMsg()).arg(errors.at(i).GetErrorDescription()));
	}
	error_msg.chop(1); // remove last \n
	bool cplsChanged = false;
	if(errors.empty() == true) {

		//Open all CPLs for writing (updates the essence descriptor list)
		for (int i = 0; i < mpImfPackage->GetAssetCount(); i++) {
			if (mpImfPackage->GetAsset(i)->GetType() == Asset::cpl) {
				QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(i).objectCast<AssetCpl>();
				//WR begin
				if (asset_cpl && asset_cpl->GetIsNewOrModified()) {
				//WR end
					cplsChanged = true;
					emit ShowCpl(asset_cpl->GetId());
					asset_cpl->FileModified();
					qDebug() << "Re-open CPL" << asset_cpl->GetId();
				}
			}
		}
		if (!cplsChanged) {
			StartOutgest();
			emit WritePackageComplete();
		} else {
			emit CallSaveAllCpl();
		}
		//WR begin
		//StartOutgest();
		//WR end
		//emit WritePackageComplete();
	}
	else {
		mpMsgBox->setText(tr("Wrapping Error"));
		mpMsgBox->setInformativeText(error_msg);
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->exec();
	}
}

void WidgetImpBrowser::rImpViewDoubleClicked(const QModelIndex &rIndex) {

	if(mpImfPackage) {
		//qDebug() << "rImpViewDoubleClicked";
		if(sender() == mpViewImp && rIndex.column() != ImfPackage::ColumnAnnotation) {
			QModelIndex index = mpSortProxyModelImp->mapToSource(rIndex);
			if(index.isValid() == true) {
				QSharedPointer<AssetMxfTrack> asset = mpImfPackage->GetAsset(index.row()).objectCast<AssetMxfTrack>();
				if(asset && (asset->HasSourceFiles() || asset->Exists())) { // Do not call rShowResourceGeneratorForAsset for assets that are listed in the PKL but are not present in the file system.
					rShowResourceGeneratorForAsset(asset->GetId());
				}
				QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(index.row()).objectCast<AssetCpl>();
				if(asset_cpl && asset_cpl->Exists() == true) {
					emit ShowCpl(asset_cpl->GetId());
				}
				QSharedPointer<AssetScm> asset_scm = mpImfPackage->GetAsset(index.row()).objectCast<AssetScm>();
				if(asset_scm) {
					rShowSidecarCompositionMapGeneratorView();
				}
			}
		}
	}
}

			/* -----Denis Manthey Beg----- */
void WidgetImpBrowser::rOpenCplTimeline() {
	QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row()).objectCast<AssetCpl>();
	if(asset_cpl && asset_cpl->Exists() == true) {
		emit ShowCpl(asset_cpl->GetId());
	}
}
			/* -----Denis Manthey End----- */

void WidgetImpBrowser::rShowMetadata() {
	QSharedPointer<AssetMxfTrack> asset = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row()).objectCast<AssetMxfTrack>();
	if(asset) {
		rShowResourceGeneratorForAsset(asset->GetId());
	}
}

void WidgetImpBrowser::rShowEssenceDescriptor() {
	QSharedPointer<AssetMxfTrack> asset = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row()).objectCast<AssetMxfTrack>();
	if(asset) {
		rShowEssenceDescriptorForAsset(asset);
	}

}

void WidgetImpBrowser::rShowSidecarCompositionMapGeneratorEdit() {
	QSharedPointer<AssetScm> asset = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row()).objectCast<AssetScm>();
	if(asset) {
		ShowSidecarCompositionMapGenerator(asset, WizardSidecarCompositionMapGenerator::EditScm);
	}
}

void WidgetImpBrowser::rShowSidecarCompositionMapGeneratorView() {
	QSharedPointer<AssetScm> asset = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row()).objectCast<AssetScm>();
	if(asset) {
		ShowSidecarCompositionMapGenerator(asset, WizardSidecarCompositionMapGenerator::ViewScm);
	}
}

void WidgetImpBrowser::StartOutgest(bool clearUndoStack /*= true*/) {

	if(mpImfPackage) {
		//WR
		qDebug() << "StartOutgest";
		if (GetPartialOutgestInProgress()) {
			QSharedPointer<ImfPackage> PartialImp(new ImfPackage(QDir(GetPartialImpPath())));
			qDebug() << "StartOutgest:" << GetPartialImpPath();
			ImfError error = PartialImp->Ingest();
			if(error.IsError()) {
				QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
				mpMsgBox->setText(tr("Ingest Error"));
				mpMsgBox->setIcon(QMessageBox::Critical);
				mpMsgBox->setInformativeText(error_msg);
				mpMsgBox->setStandardButtons(QMessageBox::Ok);
				mpMsgBox->setDefaultButton(QMessageBox::Ok);
				mpMsgBox->exec();
			}

			for (int i = 0; i < mpImfPackage->GetAssetCount(); i++) {
				//Add new CPLs to the Partial IMP
				if (mpImfPackage->GetAsset(i)->GetType() == Asset::cpl) {
					QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(i).objectCast<AssetCpl>();
					if (asset_cpl->GetIsNew() == true) {
						QFile::copy(asset_cpl->GetPath().absoluteFilePath(), QString("%1/%2").arg(PartialImp->GetRootDir().absolutePath()).arg(asset_cpl->GetOriginalFileName().first));
						QSharedPointer<AssetCpl> newCPL(new AssetCpl(QString("%1/%2").arg(PartialImp->GetRootDir().absolutePath()).arg(asset_cpl->GetOriginalFileName().first), asset_cpl->GetId(), asset_cpl->GetAnnotationText()));
						newCPL->SetHash(asset_cpl->GetHash());
						QFile::remove(asset_cpl->GetPath().absoluteFilePath());
						PartialImp->AddAsset(newCPL, PartialImp->GetPackingListId());
					}
				}
				//Add new MXF Tracks to the Partial IMP
				if (mpImfPackage->GetAsset(i)->GetType() == Asset::mxf) {
					QSharedPointer<AssetMxfTrack> asset_mxf = mpImfPackage->GetAsset(i).objectCast<AssetMxfTrack>();
					if (asset_mxf->GetIsNew() == true) {
						QFile::copy(asset_mxf->GetPath().absoluteFilePath(), QString("%1/%2").arg(PartialImp->GetRootDir().absolutePath()).arg(asset_mxf->GetOriginalFileName().first));
						QSharedPointer<AssetMxfTrack> newMXF(new AssetMxfTrack(QString("%1/%2").arg(PartialImp->GetRootDir().absolutePath()).arg(asset_mxf->GetOriginalFileName().first), asset_mxf->GetId()));
						newMXF->SetHash(asset_mxf->GetHash());
						QFile::remove(asset_mxf->GetPath().absoluteFilePath());
						PartialImp->AddAsset(newMXF, PartialImp->GetPackingListId());
					}
				}
				//Add new SCMs to the Partial IMP
				if (mpImfPackage->GetAsset(i)->GetType() == Asset::scm) {
					QSharedPointer<AssetScm> asset_scm = mpImfPackage->GetAsset(i).objectCast<AssetScm>();
					if (asset_scm->GetIsNew() == true) {
						QFile::copy(asset_scm->GetPath().absoluteFilePath(), QString("%1/%2").arg(PartialImp->GetRootDir().absolutePath()).arg(asset_scm->GetOriginalFileName().first));
						QSharedPointer<AssetScm> newSCM(new AssetScm(QString("%1/%2").arg(PartialImp->GetRootDir().absolutePath()).arg(asset_scm->GetOriginalFileName().first), asset_scm->GetId(), asset_scm->GetAnnotationText()));
						newSCM->SetHash(asset_scm->GetHash());
						QFile::remove(asset_scm->GetPath().absoluteFilePath());
						PartialImp->AddAsset(newSCM, PartialImp->GetPackingListId());
						// Copy all sidecar assets to new Partial IMP
						foreach (AssetScm::SidecarCompositionMapEntry entry, asset_scm.data()->GetSidecarCompositionMapEntries()) {
							QSharedPointer<AssetSidecar> asset_sidecar = mpImfPackage.data()->GetAsset(entry.id).objectCast<AssetSidecar>();
							if (asset_sidecar) {
								// Copy asset, but do not delete. Per ST2067-9, all sidecar assets need to be present in the new IMP.
								QFile::copy(asset_sidecar->GetPath().absoluteFilePath(), QString("%1/%2").arg(PartialImp->GetRootDir().absolutePath()).arg(asset_sidecar->GetOriginalFileName().first));
								QSharedPointer<AssetSidecar> newSidecar(new AssetSidecar(QString("%1/%2").arg(PartialImp->GetRootDir().absolutePath()).arg(asset_sidecar->GetOriginalFileName().first), asset_sidecar->GetId()));
								newSidecar->SetHash(asset_sidecar->GetHash());
								newSidecar->GetPklData()->setType(asset_sidecar->GetPklData()->getType());
								PartialImp->AddAsset(newSidecar, PartialImp->GetPackingListId());
							}
						}
					}
				}
			}
			InstallImp(PartialImp);
			SetPartialOutgestInProgress(false);
		}
		ImfError error = mpImfPackage->Outgest();
		if(error.IsError() == false) {
			if(error.IsRecoverableError() == true) {
				QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
				mpMsgBox->setText(tr("Outgest Warning"));
				mpMsgBox->setInformativeText(error_msg);
				mpMsgBox->setIcon(QMessageBox::Warning);
				mpMsgBox->setStandardButtons(QMessageBox::Ok);
				mpMsgBox->setDefaultButton(QMessageBox::Ok);
				mpMsgBox->exec();
			}
			if(clearUndoStack == true) mpUndoStack->clear();
		}
		else {
			QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
			mpMsgBox->setText(tr("Outgest Error"));
			mpMsgBox->setInformativeText(error_msg);
			mpMsgBox->setIcon(QMessageBox::Critical);
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->exec();
		}
	}
}

void WidgetImpBrowser::ValidateHash() {

	// TODO
}

void WidgetImpBrowser::rDeleteSelectedRow() {

	if(mpImfPackage) {
		mpMsgBox->setText(tr("Delete selected Asset?"));
		mpMsgBox->setInformativeText(tr("Are you sure you want to delete the selected Asset permanently?\nThis can't be undone."));
		mpMsgBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Abort);
		mpMsgBox->setDefaultButton(QMessageBox::Abort);
		mpMsgBox->setIcon(QMessageBox::Information);
		int ret = mpMsgBox->exec();
		if(ret == QMessageBox::Ok) {
			QSharedPointer<Asset> asset = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row());
			if(asset) {
				QFileInfo asset_path = asset->GetPath();
				mpImfPackage->RemoveAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row());
				if(asset->Exists() == true) {
					bool success = QFile::remove(asset_path.absoluteFilePath());
					if(success == false) {
						mpMsgBox->setText(tr("Couldn't delete selected Asset:"));
						mpMsgBox->setInformativeText(asset_path.absoluteFilePath());
						mpMsgBox->setStandardButtons(QMessageBox::Ok);
						mpMsgBox->setDefaultButton(QMessageBox::Ok);
						mpMsgBox->setIcon(QMessageBox::Critical);
						mpMsgBox->exec();
					}
					else {
						StartOutgest(false);
						rReinstallImp();
					}
				}
			}
		}
	}
}
//WR begin
// Called via signal SaveAllCplFinished() by WidgetCentral::SaveAllCpl
void WidgetImpBrowser::RecalcHashForCpls() {
	mpJobQueue->FlushQueue();
	for(int i = 0; i < mpImfPackage->GetAssetCount(); i++) {
		QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(i).objectCast<AssetCpl>();
		if(asset_cpl && asset_cpl->NeedsNewHash() ) {
			JobCalculateHash *p_hash_job = new JobCalculateHash(asset_cpl->GetPath().absoluteFilePath());
			connect(p_hash_job, SIGNAL(Result(const QByteArray&, const QVariant&)), asset_cpl.data(), SLOT(SetHash(const QByteArray&)));
			mpJobQueue->AddJob(p_hash_job);
			asset_cpl->SetIsNewOrModified(false);
		}
	}
	mpJobQueue->StartQueue();
	qDebug() << "mpJobQueue->StartQueue";
	//mpJobQueue will send signal finished(), calling SLOT WidgetImpBrowser::rJobQueueFinished
}

void WidgetImpBrowser::SetMxfFile(const QStringList &rFiles) {

		MetadataExtractor extractor;
		Metadata metadata;
		Error error(Error::None);
		extractor.ReadMetadata(metadata, QDir(rFiles.at(0)).absolutePath());
		qDebug() << "Asset ID of imported MXF track file:" << metadata.assetId.toString();
		QFileInfo path = QFileInfo(QDir(rFiles.at(0)).absolutePath());
		if (metadata.assetId.isNull()) {
			mpMsgBox->setText(tr("MXF Error"));
			mpMsgBox->setInformativeText("Cannot determine Asset ID");
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->setIcon(QMessageBox::Critical);
			mpMsgBox->exec();
			return;
		}
		if (!mpImfPackage->GetAsset(metadata.assetId).isNull()) {
			mpMsgBox->setText(tr("MXF Error"));
			mpMsgBox->setInformativeText(tr("Asset ID %1 already exists in IMP").arg(metadata.assetId.toString()));
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->setIcon(QMessageBox::Critical);
			mpMsgBox->exec();
			return;
		}
		if(rFiles.isEmpty() == false) {
			if(is_mxf_file(rFiles.at(0))) {
				QFileInfo source_file(rFiles.at(0));
				ASDCP::EssenceType_t EssenceType;
				ASDCP::Result_t result = ASDCP::EssenceType(QDir(rFiles.at(0)).absolutePath().toStdString(), EssenceType);
				if ( ASDCP_FAILURE(result) )
					qDebug() << "ASDCP_FAILURE";

				switch(EssenceType) {
				#ifdef ARCHIVIST
					case  ASDCP::ESS_ACES:
						break;
				#endif
					case ASDCP::ESS_AS02_JPEG_2000:
					case ASDCP::ESS_AS02_PCM_24b_48k:
					case ASDCP::ESS_AS02_PCM_24b_96k:
						break;

					case ASDCP::ESS_AS02_TIMED_TEXT:
						// Convert originalDuration to CPL Edit Units
						metadata.editRate = mpImfPackage->GetImpEditRates().first();
						metadata.duration = Duration(ceil(metadata.originalDuration.GetCount() / metadata.effectiveFrameRate.GetQuotient() * metadata.editRate.GetQuotient()));

						break;
					default:
						mpMsgBox->setText(tr("MXF Error"));
						mpMsgBox->setInformativeText("Unknown Essence Type");
						mpMsgBox->setStandardButtons(QMessageBox::Ok);
						mpMsgBox->setDefaultButton(QMessageBox::Ok);
						mpMsgBox->setIcon(QMessageBox::Critical);
						mpMsgBox->exec();
						return;
				}

				QSharedPointer<AssetMxfTrack> mxf_asset(new AssetMxfTrack(path, metadata));
				mxf_asset->SetIsNew(true);
				mpUndoStack->push(new AddAssetCommand(mpImfPackage, mxf_asset, mpImfPackage->GetPackingListId()));

				mxf_asset->ExtractEssenceDescriptor(mxf_asset->GetPath().absoluteFilePath());
			}
		}

		mpFileDialog->hide();
}

void WidgetImpBrowser::SetScmFile(const QStringList &rFiles) {
	//Reading and validating file here
	
	mpMsgBox->setText(tr("Read File"));
	mpMsgBox->setInformativeText("Reading SCM XML File from: " + QDir(rFiles.at(0)).absolutePath());
	mpMsgBox->setStandardButtons(QMessageBox::Ok);
	mpMsgBox->setDefaultButton(QMessageBox::Ok);
	mpMsgBox->setIcon(QMessageBox::Information);
	mpMsgBox->exec();
}

void WidgetImpBrowser::SetMxfFileDirectory(const QString& rPath) {
	if (!rPath.contains(mpImfPackage->GetRootDir().absolutePath())) mpFileDialog->setDirectory(mpImfPackage->GetRootDir().absolutePath());
}

void WidgetImpBrowser::SetScmFileDirectory(const QString& rPath) {
	if (!rPath.contains(mpImfPackage->GetRootDir().absolutePath())) mpFileDialog->setDirectory(mpImfPackage->GetRootDir().absolutePath());
}

//WR end


void WidgetImpBrowser::ExportPartialImp(QString &rDir, QString &rIssuer, QString &rAnnotation) {

	QSharedPointer<ImfPackage> PartialImp(new ImfPackage(rDir, rIssuer, rAnnotation));
	//Write Packing List and Assetmap to Root Folder. This is neccessary for adding assets to the IMP
	PartialImp->Outgest();
	//WR
	SetPartialImpPath(rDir);
	SetPartialOutgestInProgress(true);
	for (int i = 0; i < mpImfPackage->GetAssetCount(); i++) {
		if (mpImfPackage->GetAsset(i)->GetType() == Asset::mxf) {
			QSharedPointer<AssetMxfTrack> asset_mxf = mpImfPackage->GetAsset(i).objectCast<AssetMxfTrack>();
			if(asset_mxf->Exists() && !asset_mxf->GetIsNew()) { //GetIsNew() is true for imported MXF files
				asset_mxf->SetIsNew(false);
			} else {
				asset_mxf->SetIsNew(true);
			}
		}
	}
	//WR
	Save();
}

void WidgetImpBrowser::LoadAdditionalImpPackage(const QSharedPointer<ImfPackage> &rImfPackage) {
	for (int i = 0; i < rImfPackage->GetAssetCount(); i++) {
		QSharedPointer<AssetMxfTrack> asset = rImfPackage->GetAsset(i).objectCast<AssetMxfTrack>();
		if (!asset.isNull()) {
			asset->SetIsOutsidePackage(true);
			// Create a fancy color for the additional IMP
			QColor asset_color = QColor(Qt::yellow); //.darker(120*mAdditionalPackages.size() - 20);
			float hueF = 360.0 * asset_color.hueF() * (1.25 -  (mAdditionalPackages.size()%5) * 0.25);
			hueF = (hueF < 0 ? 0.0 : hueF);
			asset_color.setHsv(hueF, asset_color.saturation(), asset_color.value(), asset_color.alpha());
			asset->SetColor(asset_color);

			if (mpImfPackage->AddAsset(asset, QUuid(0)))
				mAdditionalAssets.append(asset);
		}
	}
	return;
}

void WidgetImpBrowser::AddQcReportAsSidecar(const QString rQcReport) {
	QString report_file_name = this->GetImfPackage().data()->GetRootDir().absolutePath();
	report_file_name.append(QString("/photon-qc-report-%1.txt").arg(QDateTime::currentDateTime().toString(Qt::DateFormat::ISODate).remove(':')));
	QFile file(report_file_name);
	bool success = file.open(QIODevice::WriteOnly);
	if(success) {
		file.write(rQcReport.toUtf8());
		file.close();
	} else {
		QString error_msg = QString("Cannot write to file: %1").arg(report_file_name);
		mpMsgBox->setText(tr("Error saving QC report"));
		mpMsgBox->setIcon(QMessageBox::Warning);
		mpMsgBox->setInformativeText(error_msg);
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->exec();
		return;
	}
	QFileInfo report_file_info(report_file_name);
	QUuid id;
	Error hash_error = Error(UUIDVersion5::CalculateFromEntireFile(
			UUIDVersion5::s_asset_id_prefix,
			report_file_info.absoluteFilePath(),
			id)
	);
	if (!hash_error.IsError() && !hash_error.IsRecoverableError()) {

		QSharedPointer<AssetSidecar> sidecar_asset(new AssetSidecar(report_file_info, id));
		sidecar_asset->GetPklData()->setType(MediaType::GetMediaType(report_file_info));
		if (sidecar_asset) {
			mpUndoStack->push(new AddAssetCommand(mpImfPackage, sidecar_asset, mpImfPackage->GetPackingListId()));

		}
	} else {
		return;
	}
	QUuid scm_id = QUuid::createUuid();
	QString annotation_text = "Photon QC report";
	QString issuer = "IMF Tool";
	QFileInfo scm_file_info = QFileInfo(mpImfPackage->GetRootDir().absoluteFilePath(QString("SCM_%1.xml").arg(strip_uuid(scm_id))));
	QSharedPointer<AssetScm> asset_scm = QSharedPointer<AssetScm>(new AssetScm(scm_file_info, scm_id, annotation_text));
	asset_scm->SetIsNew(true);
	asset_scm->SetIssuer(UserText(issuer));
	AssetScm::SidecarCompositionMapEntry entry;
	entry.filepath =  report_file_info;
	entry.id = id;
	for (int i = 0; i < mpImfPackage->GetAssetCount(); i++) {
		if (mpImfPackage->GetAsset(i).data()->GetType() == Asset::cpl) {
			QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(i).objectCast<AssetCpl>();
			if (asset_cpl) entry.mAssociatedCplAssets.append(asset_cpl);
		}
	}
	asset_scm->AddSidecarCompositionMapEntry(entry);
	mpUndoStack->push(new AddAssetCommand(mpImfPackage, asset_scm, mpImfPackage->GetPackingListId()));
	QString error_msg = QString("It is recommended to export the Sidecar QC Report as a Partial IMP to a separate folder! \n\nThis will leave the Original IMP unmodified.");
	mpMsgBox->setText(tr("Hint"));
	mpMsgBox->setIcon(QMessageBox::Information);
	mpMsgBox->setInformativeText(error_msg);
	mpMsgBox->setStandardButtons(QMessageBox::Ok);
	mpMsgBox->setDefaultButton(QMessageBox::Ok);
	mpMsgBox->exec();
	return;

}

void WidgetImpBrowser::slotCurrentChanged(const QModelIndex &selected, const QModelIndex &deselected) {
	QModelIndex modelIndex = mpSortProxyModelImp->mapToSource(selected);
	QSharedPointer<AssetMxfTrack> asset = mpImfPackage->GetAsset(modelIndex.row()).objectCast<AssetMxfTrack>();

	if (mpImfPackage->selectedIsOutsidePackage(modelIndex)) {
		QSharedPointer<AssetMxfTrack> asset = mpImfPackage->GetAsset(modelIndex.row()).objectCast<AssetMxfTrack>();
		int r = 255, g = 255, b = 255, a = 255;
		if (!asset.isNull()) asset->GetColor().getRgb(&r,&g,&b,&a);
		mpViewImp->setStyleSheet("QTableView {selection-color: rgb(" + QString::number(r)+ "," + QString::number(g)+ ","+ QString::number(b)+ ");}");
	} else {
		mpViewImp->setStyleSheet("QTableView {selection-color: #FFFFFF;}");
	}
}

void WidgetImpBrowser::rLoadRequest() {
	QFileDialog* pFileDialog = new QFileDialog(this, QString("Select MXF file"));
	//pFileDialog->setOption(QFileDialog::DontUseNativeDialog);
	pFileDialog->setFileMode(QFileDialog::DirectoryOnly);
	pFileDialog->setViewMode(QFileDialog::Detail);

	if(pFileDialog->exec()) {
		QStringList dir_list = pFileDialog->selectedFiles();
		if(dir_list.isEmpty() == false) {
			QString dir = dir_list.first();
			QSharedPointer<ImfPackage> imf_package(new ImfPackage(dir));
			ImfError error = imf_package->Ingest();
			if(error.IsError() == false) {
				if(error.IsRecoverableError() == true) {
					QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
					mpMsgBox->setText(tr("Load OV Warning"));
					mpMsgBox->setIcon(QMessageBox::Warning);
					mpMsgBox->setInformativeText(error_msg);
					mpMsgBox->setStandardButtons(QMessageBox::Ok);
					mpMsgBox->setDefaultButton(QMessageBox::Ok);
					mpMsgBox->exec();
				}
				mAdditionalPackages.append(imf_package);
				this->LoadAdditionalImpPackage(imf_package);
			} else {
				QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
				mpMsgBox->setText(tr("Load OV Error"));
				mpMsgBox->setIcon(QMessageBox::Critical);
				mpMsgBox->setInformativeText(error_msg);
				mpMsgBox->setStandardButtons(QMessageBox::Ok);
				mpMsgBox->setDefaultButton(QMessageBox::Ok);
				mpMsgBox->exec();
			}

		}
	}
}


CustomTableView::CustomTableView(QWidget *pParent /*= NULL*/) :
QTableView(pParent) {

}

void CustomTableView::startDrag(Qt::DropActions supportedActions) {

	QModelIndexList indexes = selectedIndexes();
	for(int i = 0; i < indexes.size(); i++) if(indexes.at(i).flags() != Qt::ItemIsDragEnabled) indexes.removeAt(i);
	if(indexes.count() > 0) {
		QMimeData *data = model()->mimeData(indexes);
		if(!data) return;
		QRect rect;
		rect.adjust(horizontalOffset(), verticalOffset(), 0, 0);
		QDrag *drag = new QDrag(this);
		drag->setPixmap(QPixmap(":/asset_mxf.png"));
		drag->setMimeData(data);
		Qt::DropAction default_drop_action = Qt::IgnoreAction;
		if(defaultDropAction() != Qt::IgnoreAction && (supportedActions & defaultDropAction()))
			default_drop_action = defaultDropAction();
		else if(supportedActions & Qt::CopyAction && dragDropMode() != QAbstractItemView::InternalMove)
			default_drop_action = Qt::CopyAction;
		if(drag->exec(supportedActions, default_drop_action) == Qt::MoveAction) {
			qWarning() << "Not implemented";
		}
	}
}
