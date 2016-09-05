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
#include "WidgetImpBrowser.h"
#include "global.h"
#include "ImfPackageCommands.h"
#include "WizardResourceGenerator.h"
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
#include <QToolButton>
#include <QFileDialog>
#include <list>


WidgetImpBrowser::WidgetImpBrowser(QWidget *pParent /*= NULL*/) :
QFrame(pParent), mpViewImp(NULL), mpViewAssets(NULL), mpImfPackage(NULL), mpToolBar(NULL), mpUndoStack(NULL), mpUndoProxyModel(NULL), mpSortProxyModelImp(NULL), mpSortProxyModelAssets(NULL), mpMsgBox(NULL), mpJobQueue(NULL) {

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
		mpMsgBox->setInformativeText(tr("All changes will be written on the file system an can not be undone. Proceed?"));
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
						JobWrapWav *p_wrap_job = new JobWrapWav(mxf_asset->GetSourceFiles(), mxf_asset->GetPath().absoluteFilePath(), mxf_asset->GetSoundfieldGroup(), mxf_asset->GetId());
						connect(p_wrap_job, SIGNAL(Success()), mxf_asset.data(), SLOT(FileModified()));
						mpJobQueue->AddJob(p_wrap_job);
					}


						/* -----Denis Manthey----- */
					else if(mxf_asset->GetEssenceType() == Metadata::TimedText) {
						JobWrapTimedText *p_wrap_job = new JobWrapTimedText(mxf_asset->GetSourceFiles(), mxf_asset->GetPath().absoluteFilePath(), mxf_asset->GetEditRate(), mxf_asset->GetDuration(), mxf_asset->GetId(), mxf_asset->GetProfile(), mxf_asset->GetTimedTextFrameRate());
						connect(p_wrap_job, SIGNAL(Success()), mxf_asset.data(), SLOT(FileModified()));
						mpJobQueue->AddJob(p_wrap_job);
					}
						/* -----Denis Manthey----- */

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

						/* -----Denis Manthey----- */



void WidgetImpBrowser::rReinstallImp() {

	emit WritePackageComplete();
}
						/* -----Denis Manthey----- */

void WidgetImpBrowser::ShowResourceGeneratorWavMode() {

	WizardResourceGenerator *p_wizard_resource_generator = new WizardResourceGenerator(this);
	p_wizard_resource_generator->setAttribute(Qt::WA_DeleteOnClose, true);
	p_wizard_resource_generator->SwitchMode(WizardResourceGenerator::WavMode);
	p_wizard_resource_generator->show();
	connect(p_wizard_resource_generator, SIGNAL(accepted()), this, SLOT(rResourceGeneratorAccepted()));
}


		/* -----Denis Manthey----- */
void WidgetImpBrowser::ShowResourceGeneratorTimedTextMode() {

	WizardResourceGenerator *p_wizard_resource_generator = new WizardResourceGenerator(this);
	p_wizard_resource_generator->setAttribute(Qt::WA_DeleteOnClose, true);
	p_wizard_resource_generator->SwitchMode(WizardResourceGenerator::TTMLMode);
	p_wizard_resource_generator->show();
	connect(p_wizard_resource_generator, SIGNAL(accepted()), this, SLOT(rResourceGeneratorAccepted()));
}
		/* -----Denis Manthey----- */



void WidgetImpBrowser::ShowCompositionGenerator() {

	WizardCompositionGenerator *p_wizard_composition_generator = new WizardCompositionGenerator(this);
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
		if(asset && asset->Exists() == false) {
			WizardResourceGenerator *p_wizard_resource_generator = new WizardResourceGenerator(this);
			p_wizard_resource_generator->setAttribute(Qt::WA_DeleteOnClose, true);
			p_wizard_resource_generator->setField(FIELD_NAME_SELECTED_FILES, QVariant(asset->GetSourceFiles()));
			p_wizard_resource_generator->setField(FIELD_NAME_SOUNDFIELD_GROUP, QVariant::fromValue<SoundfieldGroup>(asset->GetSoundfieldGroup()));
			p_wizard_resource_generator->setField(FIELD_NAME_EDIT_RATE, QVariant::fromValue<EditRate>(asset->GetEditRate()));
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
					mpUndoStack->push(new AddAssetCommand(mpImfPackage, mxf_asset, mpImfPackage->GetPackingListId()));
				}



						/* -----Denis Manthey----- */
				else if(selected_files.isEmpty() == false && is_ttml_file(selected_files.first())) {
					QUuid id = QUuid::createUuid();
					QString file_name = QString("TimedText_%1.mxf").arg(strip_uuid(id));
					QFileInfo mxf_asset_file_path(mpImfPackage->GetRootDir().absoluteFilePath(file_name));
					QSharedPointer<AssetMxfTrack> mxf_asset(new AssetMxfTrack(mxf_asset_file_path, id));
					mxf_asset->SetSourceFiles(selected_files);
					if (mxf_asset->GetDuration().GetCount() == 0)
						mxf_asset->SetDuration(duration);

					mpUndoStack->push(new AddAssetCommand(mpImfPackage, mxf_asset, mpImfPackage->GetPackingListId()));
				}
						/* -----Denis Manthey----- */


			}
			// Asset change request
			else {
				QSharedPointer<AssetMxfTrack> mxf_asset = mpImfPackage->GetAsset(asset_id.toUuid()).objectCast<AssetMxfTrack>();
				if(mxf_asset) {
					mxf_asset->SetSourceFiles(selected_files);
					mxf_asset->SetFrameRate(edit_rate);
					mxf_asset->SetSoundfieldGroup(soundfield_group);
				}
			}
		}
	}
}



				/* -----Denis Manthey----- */
QSharedPointer<AssetCpl> WidgetImpBrowser::GenerateEmptyCPL() {
	EditRate edit_rate;
	QString title;
	QString issuer;
	QString content_originator;
	QUuid id = QUuid::createUuid();
	QString file_name = QString("CPL_%1.xml").arg(strip_uuid(id));
	QFileInfo cpl_file_path(mpImfPackage->GetRootDir().absoluteFilePath(file_name));
	QSharedPointer<AssetCpl> cpl_asset(new AssetCpl(cpl_file_path, id));
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
				/* -----Denis Manthey----- */



void WidgetImpBrowser::rCompositionGeneratorAccepted() {

	WizardCompositionGenerator *p_composition_generator = qobject_cast<WizardCompositionGenerator *>(sender());
	if(p_composition_generator) {
		EditRate edit_rate = qvariant_cast<EditRate>(p_composition_generator->field(FIELD_NAME_EDIT_RATE));
		QString title = qvariant_cast<QString>(p_composition_generator->field(FIELD_NAME_TITLE));
		QString issuer = qvariant_cast<QString>(p_composition_generator->field(FIELD_NAME_ISSUER));
		QString content_originator = qvariant_cast<QString>(p_composition_generator->field(FIELD_NAME_CONTENT_ORIGINATOR));
		if(mpImfPackage) {
			QUuid id = QUuid::createUuid();
			QString file_name = QString("CPL_%1.xml").arg(strip_uuid(id));
			QFileInfo mxf_cpl_file_path(mpImfPackage->GetRootDir().absoluteFilePath(file_name));
			QSharedPointer<AssetCpl> cpl_asset(new AssetCpl(mxf_cpl_file_path, id));
			XmlSerializationError error = WidgetComposition::WriteMinimal(mxf_cpl_file_path.absoluteFilePath(), id, edit_rate, title, issuer, content_originator);
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

			/* -----Denis Manthey----- */
void WidgetImpBrowser::rOpenCplTimeline() {
	QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(mpSortProxyModelImp->mapToSource(mpViewImp->currentIndex()).row()).objectCast<AssetCpl>();
	if(asset_cpl && asset_cpl->Exists() == true) {
		emit ShowCpl(asset_cpl->GetId());
	}
}
			/* -----Denis Manthey----- */


void WidgetImpBrowser::StartOutgest(bool clearUndoStack /*= true*/) {

	if(mpImfPackage) {
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
//WR end


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
