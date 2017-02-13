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
#include <QToolButton>
#include <QFileDialog>
#include <list>


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
	mpViewImp->setDragDropMode(QAbstractItemView::DragOnly);

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
	QAction *p_add_pcm_resource = p_add_track_menu->addAction(QIcon(":/sound.png"), tr("PCM Resource"));
	connect(p_add_pcm_resource, SIGNAL(triggered(bool)), this, SLOT(ShowResourceGeneratorWavMode()));
	QAction *p_add_ttml_resource = p_add_track_menu->addAction(QIcon(":/text.png"), tr("Timed Text Resource"));
	connect(p_add_ttml_resource, SIGNAL(triggered(bool)), this, SLOT(ShowResourceGeneratorTimedTextMode()));
	//p_add_ttml_resource->setDisabled(true);
	p_add_track_menu->addSeparator();
	//QAction *p_add_mxf_resource = p_add_track_menu->addAction(QIcon(":/asset_mxf.png"), tr("MXF Resource"));
	//connect(p_add_mxf_resource, SIGNAL(triggered(bool)), this, SLOT(ShowResourceGeneratorMxfMode()));

	p_button_add_track->setMenu(p_add_track_menu);

	mpToolBar->addAction(p_action_undo);
	mpToolBar->addAction(p_action_redo);
	mpToolBar->addSeparator();
	mpToolBar->addWidget(p_button_add_track);
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
	connect(mpViewImp, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(rImpViewDoubleClicked(const QModelIndex&)));
	connect(mpImfPackage.data(), SIGNAL(DirtyChanged(bool)), this, SIGNAL(ImpSaveStateChanged(bool)));
	emit ImplInstalled(true);
	emit ImpSaveStateChanged(mpImfPackage->IsDirty());
	if(validateHash == true) ValidateHash();
}

void WidgetImpBrowser::UninstallImp() {

	/* if (mpImfPackage.data() != nullptr) {
		disconnect(mpImfPackage.data(), NULL, this, NULL);
	} (k) */

	disconnect(mpViewAssets->selectionModel(), NULL, this, NULL);
	disconnect(mpViewImp, NULL, this, NULL);
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

			QAction *remove_action = new QAction(QIcon(":/delete.png"), tr("Remove selected Asset"), this);
			QAction *delete_action = new QAction(QIcon(":/close.png"), tr("Delete selected Asset"), this);
			//QAction *edit_action = new QAction(QIcon(":/edit.png"), tr("Edit selected Asset"), this);
			QAction *edit_action = new QAction(QIcon(":/edit.png"), tr("Edit CPL"), this);
			QAction *extract_action = new QAction(QIcon(":/attach.png"), tr("Extract ancillary Data"), this);
			extract_action->setDisabled(true);
			connect(remove_action, SIGNAL(triggered(bool)), this, SLOT(rRemoveSelectedRow()));
			connect(delete_action, SIGNAL(triggered(bool)), this, SLOT(rDeleteSelectedRow()));
			connect(edit_action, SIGNAL(triggered(bool)), this, SLOT(rOpenCplTimeline()));
			//connect(edit_action, SIGNAL(triggered(bool)), this, SLOT(rShowResourceGeneratorForSelectedRow()));
			menu.addAction(remove_action);
			menu.addAction(delete_action);
			menu.addSeparator();
			//menu.addAction(edit_action);
			menu.addSeparator();
			QSharedPointer<Asset> asset = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(index).row());
			if(asset) {
				//if(asset->Exists() == true) edit_action->setDisabled(true);
				if(asset->Exists() == false) delete_action->setDisabled(true);
				if(asset->Exists() == true && asset->GetType() == Asset::cpl)
					menu.addAction(edit_action);
				if(asset->Exists() == true && asset->GetType() == Asset::mxf) {
					QSharedPointer<AssetMxfTrack> mxf_asset = asset.staticCast<AssetMxfTrack>();
					menu.addAction(extract_action);
				}
			}
		}
		menu.exec(mpViewImp->viewport()->mapToGlobal(pos));
	}
}

void WidgetImpBrowser::rRemoveSelectedRow() {

	if(mpImfPackage) {
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
				if(mxf_asset && mxf_asset->Exists() == false) {
					if(mxf_asset->GetEssenceType() == Metadata::Pcm) {
						JobWrapWav *p_wrap_job = new JobWrapWav(mxf_asset->GetSourceFiles(), mxf_asset->GetPath().absoluteFilePath(), mxf_asset->GetSoundfieldGroup(), mxf_asset->GetId(), mxf_asset->GetLanguageTag(), mxf_asset->GetMCATitle(), mxf_asset->GetMCATitleVersion(), mxf_asset->GetMCAAudioContentKind(), mxf_asset->GetMCAAudioElementKind());
						connect(p_wrap_job, SIGNAL(Success()), mxf_asset.data(), SLOT(FileModified()));
						mpJobQueue->AddJob(p_wrap_job);
					}


						/* -----Denis Manthey Beg----- */
					else if(mxf_asset->GetEssenceType() == Metadata::TimedText) {
						JobWrapTimedText *p_wrap_job = new JobWrapTimedText(mxf_asset->GetSourceFiles(), mxf_asset->GetPath().absoluteFilePath(), mxf_asset->GetEditRate(), mxf_asset->GetOriginalDuration(), mxf_asset->GetId(), mxf_asset->GetProfile(), mxf_asset->GetTimedTextFrameRate(), mxf_asset->GetLanguageTag());
						connect(p_wrap_job, SIGNAL(Success()), mxf_asset.data(), SLOT(FileModified()));
						mpJobQueue->AddJob(p_wrap_job);
					}
						/* -----Denis Manthey End----- */

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
//WR
void WidgetImpBrowser::ShowResourceGeneratorMxfMode() {
	mpFileDialog = new QFileDialog(this, QString(), GetWorkingDir().absolutePath());
	mpFileDialog->setFileMode(QFileDialog::ExistingFile);
	mpFileDialog->setViewMode(QFileDialog::Detail);
	mpFileDialog->setNameFilters(QStringList() << "*.mxf");
	mpFileDialog->setIconProvider(new IconProviderExrWav(this)); // TODO: Does not work.
	connect(mpFileDialog, SIGNAL(filesSelected(const QStringList &)), this, SLOT(SetMxfFile(const QStringList &)));
	mpFileDialog->show();
}
//WR

void WidgetImpBrowser::rShowResourceGeneratorForSelectedRow() {

	if(mpImfPackage) {
		QSharedPointer<Asset> asset = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row());
		if(asset) rShowResourceGeneratorForAsset(asset->GetId());
	}
}

void WidgetImpBrowser::rShowResourceGeneratorForAsset(const QUuid &rAssetId) {

	if(mpImfPackage) {
		QSharedPointer<AssetMxfTrack> asset = mpImfPackage->GetAsset(rAssetId).objectCast<AssetMxfTrack>();
		if(asset && asset->Exists() == false) {
			WizardResourceGenerator *p_wizard_resource_generator = new WizardResourceGenerator(this, mpImfPackage->GetImpEditRates());
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
		qDebug() << "rImpViewDoubleClicked";
		if(sender() == mpViewImp && rIndex.column() != ImfPackage::ColumnAnnotation) {
			QModelIndex index = mpSortProxyModelImp->mapToSource(rIndex);
			if(index.isValid() == true) {
				QSharedPointer<AssetMxfTrack> asset = mpImfPackage->GetAsset(index.row()).objectCast<AssetMxfTrack>();
				if(asset && asset->Exists() == false) {
					rShowResourceGeneratorForAsset(asset->GetId());
				}
				QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(index.row()).objectCast<AssetCpl>();
				if(asset_cpl && asset_cpl->Exists() == true) {
					emit ShowCpl(asset_cpl->GetId());
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
				//Add new CPLs to the IMP
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
				//Add new MXF Tracks to the IMP
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
			}
			InstallImp(PartialImp);
			SetPartialOutgestInProgress(false);
		}
		//WR
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

	mpFileDialog->hide();
	if(rFiles.isEmpty() == false) {
		if(is_mxf_file(rFiles.at(0))) {
			//determine if AS-02
			  ASDCP::EssenceType_t EssenceType;
			  ASDCP::Result_t result = ASDCP::EssenceType(QDir(rFiles.at(0)).absolutePath().toStdString(), EssenceType);
			  qDebug() << rFiles.at(0);
			  if ( ASDCP_FAILURE(result) )
			    qDebug() << "ASDCP_FAILURE";
;
			  if ( EssenceType == ASDCP::ESS_AS02_JPEG_2000 )
			    {
				  //FileInfoWrapper<AS_02::JP2K::MXFReader, MyPictureDescriptor> wrapper;
				  //result = wrapper.file_info(Options, "JPEG 2000 pictures");
				  qDebug() << "ASDCP::ESS_AS02_JPEG_2000";
			    }

			//QFileInfo new_asset_path = QFileInfo(GetWorkingDir().absolutePath().append("/").append(rFiles.at(0))); //.c_str()));
			//QSharedPointer<AssetMxfTrack> mxf_track(new AssetMxfTrack(new_asset_path));
			//AddAsset(mxf_track, ImfXmlHelper::Convert(packing_list->getId()));
			//JobExtractEssenceDescriptor *p_ed_job = new JobExtractEssenceDescriptor(mxf_track->GetPath().absoluteFilePath());
			//connect(p_ed_job, SIGNAL(Result(const QString&, const QVariant&)), mxf_track.data(), SLOT(SetEssenceDescriptor(const QString&)));
			//mpJobQueue->AddJob(p_ed_job);
			//Determine essence type
			//Determine size
			//Calc hash (job)
			//Extract Essence descriptor (job?)
			//Add to IMP
		}
	}
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
			if(asset_mxf->Exists()) {
				asset_mxf->SetIsNew(false);
			} else {
				asset_mxf->SetIsNew(true);
			}
		}
	}
	//WR
	Save();
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
