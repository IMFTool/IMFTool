/* Copyright(C) 2018 Justin Hug, Björn Stresing, Denis Manthey, Wolfgang Ruppel, Krispin Weiss
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
#include "WizardSidecarCompositionMapGenerator.h"
#include "global.h"
#include "ImfCommon.h"
#include "QtWaitingSpinner.h"
#include "MetadataExtractor.h"
#include "DelegateComboBox.h"
#include <QFileDialog>
#include <QLabel>
#include <QStringList>
#include <QStringListModel>
#include <QStackedLayout>
#include <QTableView>
#include <QTreeView>
#include <QPushButton>
#include <QGridLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QImage>
#include <QTimer>
#include <QHeaderView>
#include <QCursor>
#include <QLineEdit>
#include <QMessageBox>
#include <qevent.h>
#include "SMPTE_Labels.h"
#include <QSplitter>
#include <QCheckBox>
#include <QMenu>
#include <QTableWidget>

#include "ImfPackage.h"

WizardSidecarCompositionMapGenerator::WizardSidecarCompositionMapGenerator(QWidget *pParent, QSharedPointer<ImfPackage> rImfPackage, QVector< QSharedPointer<AssetCpl> > rCplAssets /*= QVector< QSharedPointer<AssetCpl> >() */, QSharedPointer<AssetScm> rAssetScm /* = 0 */) :
QWizard(pParent), mpImfPackage(rImfPackage), mCplAssets(rCplAssets), mAssetScm(rAssetScm) {

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setWindowModality(Qt::WindowModal);
	setWizardStyle(QWizard::ModernStyle);
	setStyleSheet("QWizard QPushButton {min-width: 60 px;}");
	mImfPackagePath = mpImfPackage->GetRootDir().absolutePath();
	setWindowTitle(tr("Sidecar Composition Map Generator"));

	this->setOption(NoBackButtonOnStartPage);
	this->setOption(HaveNextButtonOnLastPage, false);
	this->setOption(HaveFinishButtonOnEarlyPages, false);
	InitLayout();
	

	setPage(Page_SelectAssets, new sAssetsPage(this, mpImfPackage));

	addPage(new sCplPage(this, mCplAssets, mImfPackagePath));

	AdditionalInfoPage* mAdditionalInfoPage = new AdditionalInfoPage(this, mImfPackagePath);
	addPage(mAdditionalInfoPage);

	setStartId(Page_SelectAssets);
	setOption(HaveHelpButton, false);
	setWindowTitle(tr("Sidecar Composition Map Generator"));

}

void WizardSidecarCompositionMapGenerator::InitLayout() {

	QList<QWizard::WizardButton> layout;
	layout <<  QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::FinishButton;
	setButtonLayout(layout);

}

void WizardSidecarCompositionMapGenerator::SwitchMode(eMode rMode)
{
	mMode = rMode;
	switch (mMode) {
	case WizardSidecarCompositionMapGenerator::NewScm:
		break;
	case WizardSidecarCompositionMapGenerator::EditScm:
	case WizardSidecarCompositionMapGenerator::ViewScm:
		mAssetScm.data()->GetScm();
		foreach( AssetScm::SidecarCompositionMapEntry entry, mAssetScm.data()->GetSidecarCompositionMapEntries()) {
			mInitialSidecarCompositionMapEntryList << new AssetScm::SidecarCompositionMapEntry(entry);
		}
		mSidecarCompositionMapEntryList = mInitialSidecarCompositionMapEntryList;
		if (!mAssetScm->GetAnnotationText().IsEmpty()) {
			this->setAnnotation(mAssetScm.data()->GetAnnotationText().first);
		}
		if (!mAssetScm->GetIssuer().IsEmpty()) {
			this->setIssuer(mAssetScm.data()->GetIssuer().first);
		}
		break;
	default:
		break;

	}
};



void WizardSidecarCompositionMapGenerator::setAssetFiles(QList<QStandardItem*> rSidecarAssets) {
	mSidecarAssets.empty();
	mSidecarAssets = rSidecarAssets;
}

QList<QStandardItem*> WizardSidecarCompositionMapGenerator::getAssetFiles() {
	return mSidecarAssets;
}

QString WizardSidecarCompositionMapGenerator::getAnnotation() {
	return mAnnotation;
}

void WizardSidecarCompositionMapGenerator::setAnnotation(QString rAnnotation) {
	mAnnotation = rAnnotation;
}

QString WizardSidecarCompositionMapGenerator::getIssuer() {
	return mIssuer;
}

void WizardSidecarCompositionMapGenerator::setIssuer(QString rIssuer) {
	mIssuer = rIssuer;
}


void WizardSidecarCompositionMapGenerator::setSidecarCompositionMapEntry(QStandardItem* file, QVector< QSharedPointer<AssetCpl> > rAssociatedCplAssets, QList<QUuid> rCplIdsNotInCurrentImp) {
	bool elementExists = false;

	for (int i = 0; i < mSidecarCompositionMapEntryList.size(); i++)
	{
		if (mSidecarCompositionMapEntryList[i]->filepath == QFileInfo(file->text())) // TODO check if still works
		{
			//File already exist - the struct needs to be edited (no append)
			mSidecarCompositionMapEntryList[i]->mAssociatedCplAssets = rAssociatedCplAssets;
			mSidecarCompositionMapEntryList[i]->mCplIdsNotInCurrentImp = rCplIdsNotInCurrentImp;
			elementExists = true;
		}
	}
	if (!elementExists)
	{
		AssetScm::SidecarCompositionMapEntry* map = new AssetScm::SidecarCompositionMapEntry();
		map->filepath = QFileInfo(file->text()); // TODO check if still works
		map->mAssociatedCplAssets = rAssociatedCplAssets;
		map->mCplIdsNotInCurrentImp = rCplIdsNotInCurrentImp;

		mSidecarCompositionMapEntryList.append(map);
	}
}

QList<AssetScm::SidecarCompositionMapEntry*> WizardSidecarCompositionMapGenerator::getSidecarCompositionMapEntryList() {
	return mSidecarCompositionMapEntryList;
}

QList<AssetScm::SidecarCompositionMapEntry*> WizardSidecarCompositionMapGenerator::getInitialSidecarCompositionMapEntries() {
	return mInitialSidecarCompositionMapEntryList;
}

void WizardSidecarCompositionMapGenerator::deleteSidecarCompositionMap() {
	mSidecarCompositionMapEntryList.clear();
}

QSize WizardSidecarCompositionMapGenerator::sizeHint() const {

	return QSize(600, 600);
}

sAssetsPage::sAssetsPage(QWidget *parent, QSharedPointer<ImfPackage> rImfPackage)
	: QWizardPage(parent), mpParent(parent), mpImfPackage(rImfPackage)
{
	mImfPackagePath = mpImfPackage->GetRootDir().absolutePath();
	setTitle(tr("Select Assets"));
	setSubTitle(tr("Select Sidecar Assets from the IMP folder"));

	QGridLayout *p_wrapper_layout_sAssetsPage = new QGridLayout();
	p_wrapper_layout_sAssetsPage->setVerticalSpacing(3);
	
	//---- Files for CPL ----
	mpTableViewScmFiles = new QTableView(this);
	mpModelScmFiles = new QStandardItemModel(0, 1);
	mpTableViewScmFiles->setContextMenuPolicy(Qt::CustomContextMenu);
	mpTableViewScmFiles->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mpTableViewScmFiles->setSelectionBehavior(QAbstractItemView::SelectRows);
	mpTableViewScmFiles->setSelectionMode(QAbstractItemView::SingleSelection);
	mpTableViewScmFiles->setShowGrid(false);
	mpTableViewScmFiles->horizontalHeader()->setHidden(true);
	mpTableViewScmFiles->horizontalHeader()->setStretchLastSection(true);
	mpTableViewScmFiles->verticalHeader()->setHidden(true);
	mpTableViewScmFiles->resizeRowsToContents();
	mpTableViewScmFiles->resizeColumnsToContents();
	mpTableViewScmFiles->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpTableViewScmFiles->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	mpTableViewScmFiles->setSortingEnabled(true);
	mpTableViewScmFiles->setModel(mpModelScmFiles);

	connect(mpTableViewScmFiles, SIGNAL(customContextMenuRequested(QPoint)), this,SLOT(rCustomMenuRequested(QPoint)));


	p_wrapper_layout_sAssetsPage->addWidget(new QLabel(tr("Sidecar Assets:"), this), 1, 1, 1,1, Qt::AlignTop);
	// mpTableViewScmFiles spans over two columns
	p_wrapper_layout_sAssetsPage->addWidget(mpTableViewScmFiles, 2, 1, 1, 1);

	mButtonBrowse = new QPushButton(tr("Browse"));
	mButtonBrowse->setAutoDefault(false);
	mButtonBrowse->setToolTip("Select one or more Files");
	p_wrapper_layout_sAssetsPage->addWidget(mButtonBrowse, 3, 1, 1, 1, Qt::AlignTop);
	connect(mButtonBrowse, SIGNAL(clicked()), this, SLOT(ShowFileDialog()));
	
	emit FilesListChanged();
	registerField(FIELD_NAME_SELECTED_FILES"*", this, "FilesSelected", SIGNAL(FilesListChanged()));

	setLayout(p_wrapper_layout_sAssetsPage);
	mpFileDialog = new QFileDialog(this, QString("Sidecar Assets  -  Select one ore more files"), mImfPackagePath);
	mpFileDialog->setOption(QFileDialog::DontUseNativeDialog);
	mpFileDialog->setFileMode(QFileDialog::ExistingFiles);
	mpFileDialog->setViewMode(QFileDialog::Detail);
	mpFileDialog->setNameFilters(QStringList() << "*.*");
	mpFileDialog->setIconProvider(new IconProviderExrWav(this)); // TODO: Does not work.
	connect(mpFileDialog, SIGNAL(directoryEntered(const QString&)), this, SLOT(SetScmFileDirectory(const QString&)));
	connect(mpFileDialog, SIGNAL(filesSelected(const QStringList &)), this, SLOT(SetSourceFiles(const QStringList &)));

}

void sAssetsPage::initializePage() {
	//If Edit-mode is active, assetfiles have to be loaded in table
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	switch(wizard->GetMode()) {
	case WizardSidecarCompositionMapGenerator::NewScm:
		break;
	case WizardSidecarCompositionMapGenerator::EditScm:
	case WizardSidecarCompositionMapGenerator::ViewScm:
		//To edit SCM
		{
			QList<AssetScm::SidecarCompositionMapEntry*> SidecarCompositionMapEntries =  wizard->getSidecarCompositionMapEntryList();

			for (int i = 0; i < SidecarCompositionMapEntries.size(); i++)
			{
				QStandardItem* filepath = new QStandardItem(QDir(mImfPackagePath).relativeFilePath(SidecarCompositionMapEntries[i]->filepath.absoluteFilePath()));
				mpModelScmFiles->appendRow(filepath);
				mSideAssetsToAdd.append(new QStandardItem(SidecarCompositionMapEntries[i]->filepath.absoluteFilePath()));
			}
			wizard->setAssetFiles(mSideAssetsToAdd);
		}
		if (wizard->GetMode() == WizardSidecarCompositionMapGenerator::ViewScm) mButtonBrowse->setDisabled(true);
		break;
	default:
		break;
	}
}

void sAssetsPage::ShowFileDialog() {

	mpFileDialog->show();
}

void sAssetsPage::SetScmFileDirectory(const QString& rPath) {
	if (!rPath.contains(mImfPackagePath)) mpFileDialog->setDirectory(mImfPackagePath);
}

void sAssetsPage::SetSourceFiles(const QStringList &rFiles) {
	bool duplicate = false;
	QStringList files_already_in_imp;
	for (int i = 0; i < rFiles.size(); i++)
	{
		bool fileExistsInCurrentScm = false;
		bool fileIsImpAsset = false;
		for (int j = 0; j < mSideAssetsToAdd.size(); j++) {
			// Check if select asset is already contained in the current SCM
			if (mSideAssetsToAdd[j]->text() == rFiles[i]) {
				fileExistsInCurrentScm = true;
			}
		}
		for (int ii = 0; ii < mpImfPackage->GetAssetCount(); ii++) {
			// Check if selected asset is already a (non-SCM) IMP asset
			if (!mpImfPackage->GetAsset(ii).isNull()) {
				if ( (rFiles[i].toLower() == mpImfPackage->GetAsset(ii)->GetPath().absoluteFilePath().toLower()) &&
						(mpImfPackage->GetAsset(ii)->GetType() != Asset::sidecar)) {
					files_already_in_imp << rFiles[i];
					fileIsImpAsset = true;
					// TODO: Check if asset was in edited SCM foreach(AssetScm::SidecarCompositionMapEntry* entry, wizard->getInitialSidecarCompositionMapEntryList()) {

				}
			}
		}
		if ( (rFiles[i].toLower() == mpImfPackage->GetRootDir().absoluteFilePath("ASSETMAP.xml").toLower()) ||
				(rFiles[i].toLower() == mpImfPackage->GetRootDir().absoluteFilePath("VOLINDEX.xml").toLower()) ) {
			files_already_in_imp << rFiles[i];
			fileIsImpAsset = true;
		}
		if (fileExistsInCurrentScm) {
			duplicate = true;
		}
		else if (!fileIsImpAsset && !fileExistsInCurrentScm){
			mSideAssetsToAdd.append(new QStandardItem(rFiles[i]));
		}
	}

	if (!files_already_in_imp.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText("These file(s) are already IMP assets and cannot be selected:\n\n" + files_already_in_imp.join("\n"));
		msgBox.exec();
	}

	if (duplicate) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText("One or more files already exist in current Sidecar Composition Map.");
		msgBox.exec();
	}

	mpModelScmFiles->clear();
	for (int i = 0; i < mSideAssetsToAdd.size(); i++)
	{
		QFileInfo file_info(mSideAssetsToAdd[i]->text());
		QStandardItem *FileName = new QStandardItem(QDir(mImfPackagePath).relativeFilePath(file_info.filePath()));
		mpModelScmFiles->setItem(i, FileName);
	}

	QStringList selected_files;
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	if (wizard) selected_files = qvariant_cast<QStringList>(wizard->field(FIELD_NAME_SELECTED_FILES));
	wizard->setAssetFiles(mSideAssetsToAdd);

	mpFileDialog->hide();
	emit FilesListChanged();
	emit completeChanged();
}

void sAssetsPage::rCustomMenuRequested(QPoint pos) {
	QMenu menu;
	QModelIndex index = mpTableViewScmFiles->indexAt(pos);
	if (index.isValid() == true) {
		QAction *remove_action = new QAction(QIcon(":/delete.png"), tr("Remove selected Asset from Sidecar Composition Map"), this);

		connect(remove_action, SIGNAL(triggered(bool)), this, SLOT(rRemoveSelectedRow()));
		menu.addAction(remove_action);
		menu.exec(mpTableViewScmFiles->viewport()->mapToGlobal(pos));
	}

}

void sAssetsPage::rRemoveSelectedRow() {
	QModelIndex index = mpTableViewScmFiles->currentIndex();
	rRemoveSelectedRow(index);
}

void sAssetsPage::rRemoveSelectedRow(QModelIndex index) {
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	if (mpTableViewScmFiles) {
		QFileInfo file_info = QFileInfo(mpImfPackage->GetRootDir().absolutePath().append("/").append(mpModelScmFiles->item(index.row(), 0)->text()));
		foreach(QStandardItem* item, mSideAssetsToAdd){
			if (QFileInfo(item->text()).absoluteFilePath() == file_info.absoluteFilePath()) {
				mSideAssetsToAdd.removeAll(item);
				break;
			}
		}
		wizard->setAssetFiles(mSideAssetsToAdd);
		QList<AssetScm::SidecarCompositionMapEntry*> entry_list = wizard->getSidecarCompositionMapEntryList();
		foreach(AssetScm::SidecarCompositionMapEntry* entry, entry_list) {
			if (entry->filepath.absoluteFilePath() ==  file_info.absoluteFilePath()) {
				entry_list.removeAll(entry);
				break;
			}
		}
		wizard->setSidecarCompositionMapEntryList(entry_list);
		mpModelScmFiles->removeRow(index.row());

	}
	emit FilesListChanged();
	emit completeChanged();
}

void sAssetsPage::keyPressEvent(QKeyEvent *pEvent) {
	QItemSelectionModel *select = mpTableViewScmFiles->selectionModel();
	if (pEvent->key() == Qt::Key_Delete) {
		for (int i = 0; i < select->selectedIndexes().size(); i++)
		{
			rRemoveSelectedRow(select->selectedIndexes()[i]);
		}
		return;
	}
	QWidget::keyPressEvent(pEvent);
}

QStringList sAssetsPage::GetSourceFiles() const {

	QStringList items;
	for (int row = 0; row < mpModelScmFiles->rowCount(); row++) {
		if (mpModelScmFiles->item(row, 0)) items.append(mpModelScmFiles->item(row, 0)->text());
	}
	return items;
}

sCplPage::sCplPage(QWidget *parent, QVector< QSharedPointer<AssetCpl> > rCplAssets, QString filedirectory)
	: QWizardPage(parent), mpParent(parent), mImfPackagePath(filedirectory), mCplAssets(rCplAssets)
{
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;

	setTitle(tr("CPL assignment"));
	setSubTitle(tr("Assign one or more CPLs to each sidecar asset"));

	QGridLayout *p_wrapper_layout_sCplPage = new QGridLayout();
	p_wrapper_layout_sCplPage->setVerticalSpacing(3);

	// ----Table for all CPLs in IMF - package----
	mpTableViewScmCPL = new QTableView(this);
	mpTableViewScmCPL->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mpModelScmCPL = new QStandardItemModel(0, 3); // Check box | CPL filename | UUID
	mpTableViewScmCPL->setModel(mpModelScmCPL);
	//Signal is triggered whenever a check box is checked or unchecked (changed)
	connect(mpModelScmCPL, SIGNAL(itemChanged(QStandardItem *)), this, SLOT(CplChecked(QStandardItem *)));
	mpTableViewScmCPL->setSelectionBehavior(QAbstractItemView::SelectRows);
	mpTableViewScmCPL->setSelectionMode(QAbstractItemView::NoSelection);
	mpTableViewScmCPL->setShowGrid(false);
	mpTableViewScmCPL->horizontalHeader()->setHidden(true);
	mpTableViewScmCPL->horizontalHeader()->setStretchLastSection(true);
	mpTableViewScmCPL->verticalHeader()->setHidden(true);
	mpTableViewScmCPL->resizeRowsToContents();
	mpTableViewScmCPL->resizeColumnsToContents();
	mpTableViewScmCPL->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpTableViewScmCPL->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	//mpTableViewScm->setItemDelegateForColumn(mpScmModel::ColumnDstChannel, new DelegateComboBox(this, false, false));
	// ---- END: Table for all CPLs in IMF-package ---- 

	p_wrapper_layout_sCplPage->addWidget(mpTableViewScmCPL, 1, 1, 1, 1);
	setLayout(p_wrapper_layout_sCplPage);

	emit completeChanged();
	SetCplFilesList(mCplAssets);
	isComplete();

	registerField(FIELD_NAME_SCM_CPL_LIST"*", this, "CplsSelected", SIGNAL(CplListChanged()));
	QAbstractButton *backButton = wizard->button(QWizard::BackButton);
	disconnect(backButton, SIGNAL(clicked(bool)), 0, 0);
	connect(backButton, SIGNAL(clicked()), this, SLOT(handleBackButton()));
}

void sCplPage::initializePage() {
	mpModelScmCPL->clear();
	SetCplFilesList(mCplAssets);
}

void sCplPage::mpTableViewScmCPL_ItemClicked() {
	QMessageBox msgBox;
	msgBox.setText("mpTableViewScmCPL_ItemClicked");
	msgBox.exec();	
}

void sCplPage::SetCplFilesList(QVector< QSharedPointer<AssetCpl> > rCplAssets) {

	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	QList<QStandardItem*> sidecar_assets;

	sidecar_assets = wizard->getAssetFiles();
	QList<AssetScm::SidecarCompositionMapEntry*> SidecarCompositionMapEntries = wizard->getSidecarCompositionMapEntryList();

	int line_count = 0;

	for (int j = 0; j < sidecar_assets.size(); j++)
	{
		QStandardItem* item = new QStandardItem(true);
		item->setText(QDir(mImfPackagePath).relativeFilePath(sidecar_assets[j]->text()));
		//int rowcounter = (rCplAssets.size() + 1) * j;
		mpModelScmCPL->setItem(line_count++, 0, item);

		int AssetNoInList = -1;

		for (int k = 0; k < SidecarCompositionMapEntries.size(); k++)
		{
			if (SidecarCompositionMapEntries[k]->filepath.absoluteFilePath() == QFileInfo(sidecar_assets[j]->text()).absoluteFilePath())
			{
				AssetNoInList = k;
				break;
			}
		}

		for (int i = 0; i < rCplAssets.size(); i++) {
			QStandardItem* item0 = new QStandardItem(true);
			item0->setCheckable(true);
			item0->setCheckState(Qt::Unchecked);
			item0->setText(rCplAssets[i].data()->GetPath().fileName());
			if (wizard->GetMode() == WizardSidecarCompositionMapGenerator::ViewScm) item0->setEnabled(false);

			QStandardItem* item1 = new QStandardItem(true);
			item1->setText(rCplAssets[i].data()->GetId().toString());
			mpModelScmCPL->setItem(line_count, 0, item0);
			mpModelScmCPL->setItem(line_count++,   1, item1);
			if ((SidecarCompositionMapEntries.size() > 0) && ( AssetNoInList != -1))
			{
				for (int z = 0; z < SidecarCompositionMapEntries[AssetNoInList]->mAssociatedCplAssets.size(); z++)
				{
					if (SidecarCompositionMapEntries[AssetNoInList]->mAssociatedCplAssets[z].data()->GetId() == rCplAssets[i].data()->GetId())
					{
						mpModelScmCPL->item(line_count - 1, 0)->setCheckState(Qt::Checked);
					}
				}
			}
		}
		if ( AssetNoInList != -1 ) { // Add CPL references that are not contained in current IMP
			foreach (QUuid id, SidecarCompositionMapEntries[AssetNoInList]->mCplIdsNotInCurrentImp) {
				QStandardItem* item0 = new QStandardItem(true);
				item0->setCheckable(true);
				item0->setCheckState(Qt::Unchecked); // Initial state Qt::Checked crashes for unknown reasons
				item0->setText("< CPL not contained in current IMP >");
				if (wizard->GetMode() == WizardSidecarCompositionMapGenerator::ViewScm) item0->setEnabled(false);

				QStandardItem* item1 = new QStandardItem(true);
				item1->setText(id.toString());
				mpModelScmCPL->setItem(line_count, 0, item0);
				mpModelScmCPL->setItem(line_count++,   1, item1);
				mpModelScmCPL->item(line_count - 1, 0)->setCheckState(Qt::Checked);
			}
		}
	}
}

void sCplPage::cleanupPage() {
	mpModelScmCPL->clear();
}

bool sCplPage::isComplete() const {
	bool are_mandatory_fields_filled = QWizardPage::isComplete();
	if(selectedOneOfEach()){
		return are_mandatory_fields_filled;
	}
	return false;
}

bool sCplPage::selectedOneOfEach() const {
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	QList<QStandardItem*> sidecar_assets;
	QList<AssetScm::SidecarCompositionMapEntry*> SidecarCompositionMapEntries = wizard->getSidecarCompositionMapEntryList();
	bool selectedOne = false;
	int counter = 0;
	int line_count = 0;

	if (!wizard->getAssetFiles().isEmpty())
	{
		sidecar_assets = wizard->getAssetFiles();
		for (int files = 0; files < sidecar_assets.size(); files++)
		{
			selectedOne = false;
			int index = -1;
			foreach (AssetScm::SidecarCompositionMapEntry* entry,SidecarCompositionMapEntries ) {
				index++;
				if ( entry->filepath.absoluteFilePath() == QFileInfo(sidecar_assets.at(files)->text()).absoluteFilePath() ) {
					break;
				}
			}
			// For each asset, all CPLs of the current IMP plus all assigned CPLs in other IMPs are listed
			int num_cpl_assignments = mCplAssets.size();
			if (index != -1) num_cpl_assignments += SidecarCompositionMapEntries[index]->mCplIdsNotInCurrentImp.size();
			for (int CplList = 1; CplList < num_cpl_assignments + 1; CplList++)
			{
				if (mpModelScmCPL->data(mpModelScmCPL->index(line_count + CplList, 0), Qt::CheckStateRole) == Qt::Checked) {
					selectedOne = true;
				}
			}
			if (selectedOne){
				counter++;
			}
			line_count += 1 + num_cpl_assignments;
		}
	}
	if (counter == sidecar_assets.size()){
		return true;
	}
	else {
		return false;
	}
}

bool sCplPage::handleBackButton() {
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	if (wizard->currentId() == WizardSidecarCompositionMapGenerator::Page_SelectCpl) {
		validatePage();
	} else if (wizard->currentId() == WizardSidecarCompositionMapGenerator::Page_AddAditionalInfo) {
		initializePage();
	}
	wizard->back();
	return true;
}

bool sCplPage::validatePage() {
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	QList<AssetScm::SidecarCompositionMapEntry*> SidecarCompositionMapEntries = wizard->getSidecarCompositionMapEntryList();
	QList<QStandardItem*> sidecar_assets;

	if (!wizard->getAssetFiles().isEmpty())
	{
		sidecar_assets = wizard->getAssetFiles();
		int line_count = 0;
		for (int files = 0; files < sidecar_assets.size(); files++)
		{
			int index = -1;
			foreach (AssetScm::SidecarCompositionMapEntry* entry,SidecarCompositionMapEntries ) {
				index++;
				if ( entry->filepath.absoluteFilePath() == QFileInfo(sidecar_assets.at(files)->text()).absoluteFilePath() ) {
					break;
				}
			}
			QVector< QSharedPointer<AssetCpl> > cpl_assets_checked;
			QList<QUuid> cpls_not_in_imp_checked;
			for (int CplList = 1; CplList < mCplAssets.size() + 1; CplList++)
			{
				if (mpModelScmCPL->data(mpModelScmCPL->index(line_count + CplList, 0), Qt::CheckStateRole) == Qt::Checked) {
					cpl_assets_checked.append(mCplAssets[CplList-1]);
				}
			}
			int num_cpl_assignments = mCplAssets.size();
			if (index != -1) {
				for (int i = 0; i < SidecarCompositionMapEntries[index]->mCplIdsNotInCurrentImp.size(); i++ ) {
					if (mpModelScmCPL->data(mpModelScmCPL->index(line_count + mCplAssets.size() + i + 1, 0), Qt::CheckStateRole) == Qt::Checked) {
						QUuid id = QUuid( mpModelScmCPL->index(line_count + mCplAssets.size() + i + 1, 1).data().toString());
						cpls_not_in_imp_checked.append(id);
					}
				}
				num_cpl_assignments += SidecarCompositionMapEntries[index]->mCplIdsNotInCurrentImp.size();
			}
			line_count += 1 + num_cpl_assignments;
			wizard->setSidecarCompositionMapEntry(sidecar_assets[files],cpl_assets_checked, cpls_not_in_imp_checked);
		}
	}
	return true;
}

QList<QUuid> sCplPage::GetSelectedCpls() const {
	QList<QUuid> items;
	for (int row = 0; row < mpModelScmCPL->rowCount(); row++) {
		if (mpModelScmCPL->item(row, 0)->checkState() == Qt::Checked) {
			items.append(QUuid(mpModelScmCPL->item(row, 1)->text()));
		}
	}
	return items;
}

void sCplPage::SetSelectedCpls(QList<QUuid> rSelectedCpls) {
	emit CplListChanged();
	emit completeChanged();
}

void sCplPage::CplChecked(QStandardItem *) {
	emit CplListChanged();
	emit completeChanged();
}


AdditionalInfoPage::AdditionalInfoPage(QWidget *parent, QString filedirectory)
	: QWizardPage(parent), mpParent(parent), mImfPackagePath(filedirectory)
{
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	setTitle(tr("Add more information"));
	setSubTitle(tr("Add additional metadata to Sidecar Composition Map"));
	
	QGridLayout *p_wrapper_layout_AddaditionalInfoPage = new QGridLayout();
	p_wrapper_layout_AddaditionalInfoPage->setVerticalSpacing(3);

	p_wrapper_layout_AddaditionalInfoPage->addWidget(new QLabel(tr("Annotation:"), this), 0, 0, 1, 1);
	mpLineEditAnnotation = new QLineEdit(this);
	mpLineEditAnnotation->setAlignment(Qt::AlignLeft);
	mpLineEditAnnotation->setPlaceholderText("Annotation");
	p_wrapper_layout_AddaditionalInfoPage->addWidget(mpLineEditAnnotation, 0, 1, 1, 1);

	p_wrapper_layout_AddaditionalInfoPage->addWidget(new QLabel(tr("Issuer:"), this), 1, 0, 1, 1);
	mpLineEditIssuer = new QLineEdit(this);
	mpLineEditIssuer->setAlignment(Qt::AlignLeft);
	mpLineEditIssuer->setPlaceholderText("Issuer");
	p_wrapper_layout_AddaditionalInfoPage->addWidget(mpLineEditIssuer, 1, 1, 1, 1);

	// ----Table for all CPLs in IMF - package----
	mpTableViewScmInfo = new QTableView(this);
	mpTableViewScmInfo->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mpModelScmInfo = new QStandardItemModel(0, 3); // Check box | CPL filename | UUID
	mpTableViewScmInfo->setModel(mpModelScmInfo);
	mpTableViewScmInfo->setSelectionBehavior(QAbstractItemView::SelectRows);
	mpTableViewScmInfo->setSelectionMode(QAbstractItemView::NoSelection);
	mpTableViewScmInfo->setShowGrid(false);
	mpTableViewScmInfo->horizontalHeader()->setHidden(true);
	mpTableViewScmInfo->horizontalHeader()->setStretchLastSection(true);
	mpTableViewScmInfo->verticalHeader()->setHidden(true);
	mpTableViewScmInfo->resizeRowsToContents();
	mpTableViewScmInfo->resizeColumnsToContents();
	mpTableViewScmInfo->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpTableViewScmInfo->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	//mpTableViewScm->setItemDelegateForColumn(mpScmModel::ColumnDstChannel, new DelegateComboBox(this, false, false));
	// ---- END: Table for all CPLs in IMF-package ---- 

	p_wrapper_layout_AddaditionalInfoPage->addWidget(mpTableViewScmInfo, 2, 0, 1, 2);

	connect(mpLineEditAnnotation, SIGNAL(textChanged(const QString &)), this, SLOT(SetAnnotation(const QString &)));
	connect(mpLineEditIssuer, SIGNAL(textChanged(const QString &)), this, SLOT(SetIssuer(const QString &)));

	setLayout(p_wrapper_layout_AddaditionalInfoPage);
}

void AdditionalInfoPage::initializePage() {
	mpModelScmInfo->clear();
	setScmList();
}

void AdditionalInfoPage::setScmList() {
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	mSidecarCompositionMapEntryList = wizard->getSidecarCompositionMapEntryList();
	

	if (!wizard->getAnnotation().isEmpty()) mpLineEditAnnotation->setText(wizard->getAnnotation());
	if (wizard->GetMode() == WizardSidecarCompositionMapGenerator::ViewScm) mpLineEditAnnotation->setEnabled(false);

	if (!wizard->getIssuer().isEmpty()) mpLineEditIssuer->setText(wizard->getIssuer());
	if (wizard->GetMode() == WizardSidecarCompositionMapGenerator::ViewScm) mpLineEditIssuer->setEnabled(false);

	if (mSidecarCompositionMapEntryList.size() > 0) {
		for (int i = 0; i < mSidecarCompositionMapEntryList.size(); i++)
		{
			QStandardItem* filepathLabel = new QStandardItem(true);
			filepathLabel->setText("File:");
			mpModelScmInfo->appendRow(filepathLabel);

			QStandardItem* filepath = new QStandardItem(true);
			QFont font;
			font.setBold(true);
			filepath->setFont(font);
			filepath->setText(mSidecarCompositionMapEntryList[i]->filepath.absoluteFilePath());
			mpModelScmInfo->setItem(mpModelScmInfo->rowCount()-1, 1, filepath);

			int cpl_num = 1;
			foreach(QSharedPointer<AssetCpl> asset_cpl, mSidecarCompositionMapEntryList[i]->mAssociatedCplAssets) {
				QStandardItem* CplLabel = new QStandardItem(true);
				CplLabel->setText(QString("CPL %1:").arg(cpl_num++));
				mpModelScmInfo->appendRow(CplLabel);

				QStandardItem* AssetCpl = new QStandardItem(true);
				AssetCpl->setText(asset_cpl.data()->GetPath().fileName());
				mpModelScmInfo->setItem(mpModelScmInfo->rowCount() - 1, 1, AssetCpl);
			}
			foreach(QUuid id, mSidecarCompositionMapEntryList[i]->mCplIdsNotInCurrentImp) {
				QStandardItem* CplLabel = new QStandardItem(true);
				CplLabel->setText(QString("CPL %1:").arg(cpl_num++));
				mpModelScmInfo->appendRow(CplLabel);

				QStandardItem* AssetCpl = new QStandardItem(true);
				AssetCpl->setText(QString("<CPL not in IMP> ID: %1").arg(id.toString()));
				mpModelScmInfo->setItem(mpModelScmInfo->rowCount() - 1, 1, AssetCpl);
			}
			QStandardItem* spacer = new QStandardItem(true);
			spacer->setText("");
			spacer->setEditable(false);
			spacer->setEnabled(false);
			mpModelScmInfo->appendRow(spacer);
		}
	}
	else {
		QMessageBox msgBox;
		msgBox.setText("SidecarCompositionMapList is empty");
		msgBox.exec();
	}

}

void AdditionalInfoPage::SetAnnotation(QString rAnnotation)
{	
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	wizard->setAnnotation(rAnnotation);
	emit mAnnotationChanged();
}

void AdditionalInfoPage::SetIssuer(QString rIssuer)
{
	WizardSidecarCompositionMapGenerator* wizard = (WizardSidecarCompositionMapGenerator *)mpParent;
	wizard->setIssuer(rIssuer);
	emit mIssuerChanged();
}

bool AdditionalInfoPage::isComplete() const {
	return true;
}
