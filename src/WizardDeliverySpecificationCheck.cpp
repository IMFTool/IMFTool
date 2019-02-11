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
#include "WizardDeliverySpecificationCheck.h"
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
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <qevent.h>
#include "SMPTE_Labels.h"
#include <QSplitter>
#include <QCheckBox>
#include <QMenu>
#include <QButtonGroup>
#include <QTextEdit>

#include "ImfPackage.h"
#include "JobQueue.h"
#include "Jobs.h"

WizardDeliverySpecificationCheck::WizardDeliverySpecificationCheck(QWidget *pParent, QVector< QSharedPointer<AssetCpl> > rCplAssets /*= QVector< QSharedPointer<AssetCpl> >() */) :
QWizard(pParent), mCplAssets(rCplAssets) {

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setWindowModality(Qt::WindowModal);
	setWizardStyle(QWizard::ModernStyle);
	setStyleSheet("QWizard QPushButton {min-width: 60 px;}");

	this->setOption(NoBackButtonOnStartPage);
	this->setOption(HaveNextButtonOnLastPage, false);
	this->setOption(HaveFinishButtonOnEarlyPages, false);

	InitLayout();
	
	//enum { Page_SelectSepcification, Page_SelectDeliverable, Page_SelectCpl, Page_Result};

	setPage(Page_SelectSpecification, new SelectDeliverySpecificationListPage(this));

	addPage(new SelectDeliverySpecificationPage(this, mCplAssets));

	addPage(new SelectCplPage(this, mCplAssets));

	ResultPage* mResultPage = new ResultPage(this);
	addPage(mResultPage);

	setStartId(Page_SelectSpecification);
	setOption(HaveHelpButton, false);
	setWindowTitle(tr("Select a Delivery Specification"));

}

void WizardDeliverySpecificationCheck::InitLayout() {

	QList<QWizard::WizardButton> layout;
	layout <<  QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::FinishButton;
	setButtonLayout(layout);

}


QSize WizardDeliverySpecificationCheck::sizeHint() const {

	return QSize(800, 600);
}

/*
 * SelectDeliverySpecificationListPage
 */

SelectDeliverySpecificationListPage::SelectDeliverySpecificationListPage(QWidget *parent, QVector< QSharedPointer<AssetCpl> > rCplAssets)
	: QWizardPage(parent), mpParent(parent)
{
	setTitle(tr("Select a Delivery Specification List (DSL)"));
	setSubTitle(tr("Select a pre-defined DSL or pick one from the file system"));

	QGridLayout *p_specification_selection_layout = new QGridLayout();
	p_specification_selection_layout->setVerticalSpacing(3);
	
	mpTableViewDeliverySpecs = new QTableView(this);
	mpModelDeliverySpecs = new QStandardItemModel(0, 1);
	mpTableViewDeliverySpecs->setContextMenuPolicy(Qt::CustomContextMenu);
	mpTableViewDeliverySpecs->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mpTableViewDeliverySpecs->setSelectionBehavior(QAbstractItemView::SelectRows);
	mpTableViewDeliverySpecs->setSelectionMode(QAbstractItemView::SingleSelection);
	mpTableViewDeliverySpecs->setShowGrid(false);
	mpTableViewDeliverySpecs->horizontalHeader()->setHidden(true);
	mpTableViewDeliverySpecs->horizontalHeader()->setStretchLastSection(true);
	mpTableViewDeliverySpecs->verticalHeader()->setHidden(true);
	mpTableViewDeliverySpecs->resizeRowsToContents();
	mpTableViewDeliverySpecs->resizeColumnsToContents();
	mpTableViewDeliverySpecs->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpTableViewDeliverySpecs->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpTableViewDeliverySpecs->setSortingEnabled(true);
	mpTableViewDeliverySpecs->setModel(mpModelDeliverySpecs);
	connect(mpTableViewDeliverySpecs, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotCellClicked(const QModelIndex &)));

	mRadioButton1 = new QRadioButton(this);
	connect(mRadioButton1, SIGNAL(clicked()), this, SLOT(button1Clicked()));
	mRadioButton1->setChecked(true);
	p_specification_selection_layout->addWidget(mRadioButton1, 1, 1, 1, 1);
	p_specification_selection_layout->addWidget(new QLabel(tr("Select a Delivery Specification List from the table below"), this), 1, 2, 1, 2, Qt::AlignTop);

	p_specification_selection_layout->addWidget(mpTableViewDeliverySpecs, 2, 2, 1, 2);

	mRadioButton2 = new QRadioButton(this);
	connect(mRadioButton2, SIGNAL(clicked()), this, SLOT(button2Clicked()));
	p_specification_selection_layout->addWidget(mRadioButton2, 3, 1, 1, 1);
	p_specification_selection_layout->addWidget(new QLabel(tr("Select a Delivery Specification from the file system"), this), 3, 2, 1, 2, Qt::AlignTop);

	mButtonBrowse = new QPushButton(tr("Browse"));
	mButtonBrowse->setAutoDefault(false);
	mButtonBrowse->setToolTip("Select one or more Files");
	p_specification_selection_layout->addWidget(mButtonBrowse, 4, 2, 1, 1, Qt::AlignTop);
	connect(mButtonBrowse, SIGNAL(clicked()), this, SLOT(ShowFileDialog()));
	mSelectedFile = new QLineEdit();
	mSelectedFile->setPlaceholderText("Browse to select a delivery specification");

	p_specification_selection_layout->addWidget(mSelectedFile, 4, 3, 1, 1, Qt::AlignTop);
	
	registerField(FIELD_NAME_DELIVERY_SPECIFICATION_LIST"*", this, "DeliverySpecificationListSelected", SIGNAL(DeliverySpecChanged()));

	setLayout(p_specification_selection_layout);
	mpFileDialog = new QFileDialog(this, QString("Select a Delivery Specification List"), QDir::homePath());
	mpFileDialog->setOption(QFileDialog::DontUseNativeDialog);
	mpFileDialog->setFileMode(QFileDialog::ExistingFile);
	mpFileDialog->setViewMode(QFileDialog::Detail);
	mpFileDialog->setNameFilters(QStringList() << "*.xml");
	connect(mpFileDialog, SIGNAL(filesSelected(const QStringList &)), this, SLOT(SetSourceFiles(const QStringList &)));

}

void SelectDeliverySpecificationListPage::initializePage() {
	mDeliverySpecifications = QDir(QApplication::applicationDirPath().append("/delivery-specifications")).entryList(QDir::Files|QDir::Readable);
	for (int i = 0; i < mDeliverySpecifications.size(); i++) {
		QStandardItem* filepath = new QStandardItem(mDeliverySpecifications[i]);
		mpModelDeliverySpecs->appendRow(filepath);
	}
}

void SelectDeliverySpecificationListPage::ShowFileDialog() {

	mpFileDialog->show();
}

void SelectDeliverySpecificationListPage::SetSourceFiles(const QStringList &rFiles) {

	if (rFiles.length() == 1) {
		mSelectedFile->setText(rFiles[0]);
		mSelectedDeliverySpecificationListPath = rFiles[0];
		mRadioButton2->setChecked(true);
		emit DeliverySpecChanged();
		emit completeChanged();
	}
	mpFileDialog->hide();
}

void SelectDeliverySpecificationListPage::slotCellClicked(const QModelIndex &rIndex) {
	qDebug() << rIndex.row() << rIndex.column();
	if (mDeliverySpecifications.size() > rIndex.row()) {
		mSelectedDeliverySpecificationListPath = QApplication::applicationDirPath().append("/delivery-specifications/").append(mDeliverySpecifications[rIndex.row()]);
		emit DeliverySpecChanged();
		mRadioButton1->setChecked(true);
	}
}

void SelectDeliverySpecificationListPage::button1Clicked() {
	QModelIndexList indexes = mpTableViewDeliverySpecs->selectionModel()->selection().indexes();
	if (indexes.count() == 1) {
		QModelIndex index = indexes.at(0);
		qDebug() << index.row() <<  index.column();
		mSelectedDeliverySpecificationListPath = QApplication::applicationDirPath().append("/delivery-specifications/").append(mDeliverySpecifications[index.row()]);
		emit DeliverySpecChanged();
	}
}

void SelectDeliverySpecificationListPage::button2Clicked() {
	if (!mSelectedFile->text().isEmpty()) {
		mSelectedDeliverySpecificationListPath = mSelectedFile->text();
	} else {
		mSelectedDeliverySpecificationListPath = "";
	}
	emit DeliverySpecChanged();

}

bool SelectDeliverySpecificationListPage::validatePage() {
	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;
	if (wizard) {
		wizard->SetDeliverySpecificationListPath(mSelectedDeliverySpecificationListPath);
		qDebug() << "SetDeliverySpecificationList(mSelectedDeliverySpecificationListPath)" << mSelectedDeliverySpecificationListPath;
		return true;
	} else return false;
}

/*
 * SelectDeliverySpecificationPage
 */
SelectDeliverySpecificationPage::SelectDeliverySpecificationPage(QWidget *parent, QVector< QSharedPointer<AssetCpl> > rCplAssets)
		: QWizardPage(parent), mpParent(parent), mCplAssets(rCplAssets)
{
	setTitle(tr("Select a Delivery Specification to test against"));
	setSubTitle(tr(" "));
	mButtonGroup = new QButtonGroup();
	connect(mButtonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [=](int id){
		slotButtonClicked(id);});
	registerField(FIELD_NAME_DELIVERY_SPECIFICATION"*", this, "DeliverySpecificationSelected", SIGNAL(DeliverySpecificationIdChanged()));
}

void SelectDeliverySpecificationPage::initializePage() {
	mDeliverySpecificationId = QUuid();
	qDebug() << "SelectDeliverySpecificationPage::initializePage";
	emit DeliverySpecificationIdChanged();
	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;
	if (wizard) {
		QString dsl_file = wizard->GetDeliverySpecificationListPath();

		if (!dsl_file.isEmpty()) {
			bool success = true;
			std::auto_ptr< dsl::DeliverySpecificationList> dsl_data;
			XmlParsingError parse_error;
			try { dsl_data = dsl::parseDeliverySpecificationList(dsl_file.toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
			catch(const xml_schema::Parsing &e) { parse_error = XmlParsingError(e); }
			catch(const xml_schema::ExpectedElement &e) { parse_error = XmlParsingError(e); }
			catch(const xml_schema::UnexpectedElement &e) { parse_error = XmlParsingError(e); }
			catch(const xml_schema::ExpectedAttribute &e) { parse_error = XmlParsingError(e); }
			catch(const xml_schema::UnexpectedEnumerator &e) { parse_error = XmlParsingError(e); }
			catch(const xml_schema::ExpectedTextContent &e) { parse_error = XmlParsingError(e); }
			catch(const xml_schema::NoTypeInfo &e) { parse_error = XmlParsingError(e); }
			catch(const xml_schema::NotDerived &e) { parse_error = XmlParsingError(e); }
			catch(const xml_schema::NoPrefixMapping &e) { parse_error = XmlParsingError(e); }
			catch(...) { parse_error = XmlParsingError(XmlParsingError::Unknown); }
			QString error_message = parse_error.GetErrorMsg();
			success = error_message.contains("No Error");

			RemoveLayout(this);
			QGridLayout *p_specification_selection_layout = new QGridLayout();
			if (success) {
				mDeliverySpecificationList = dsl_data.release();
				int size = mDeliverySpecificationList->getDeliverableList().getDeliverable().size();
				p_specification_selection_layout->setVerticalSpacing(3);
				for (int i = 0; i< size; i++) {
					QString label = QString(mDeliverySpecificationList->getDeliverableList().getDeliverable().at(i).getLabel().c_str());
					QRadioButton* button = new QRadioButton(label);
					mButtonGroup->addButton(button, i+1);
					p_specification_selection_layout->addWidget(button, i+1, 1, 1, 1);
				}
			} else {
				QTextEdit* text = new QTextEdit();
				text->setTextColor(Qt::red);
				text->setText("Fatal error:\n\n"+error_message + "\n" + parse_error.GetErrorDescription());
				p_specification_selection_layout->addWidget(text, 1, 1, 1, 1);
			}
			setLayout(p_specification_selection_layout);
		}
	}
}

void SelectDeliverySpecificationPage::slotButtonClicked(const int index) {
	qDebug() << index;
	qDebug() << QString(mDeliverySpecificationList->getDeliverableList().getDeliverable().at(index-1).getLabel().c_str());
	mDeliverySpecificationId = ImfXmlHelper::Convert(mDeliverySpecificationList->getDeliverableList().getDeliverable().at(index-1).getId());
	qDebug() << index;
	emit DeliverySpecificationIdChanged();
}

void SelectDeliverySpecificationPage::RemoveLayout (QWidget* widget) {
    QLayout* layout = widget->layout ();
    if (layout != 0) {
    	QLayoutItem *item;
    	QLayout * sublayout;
    	QWidget * widget;
    	while ((item = layout->takeAt(0)) != 0) {
			if ((sublayout = item->layout()) != 0) {/* do the same for sublayout*/}
			else if ((widget = item->widget()) != 0) {widget->hide(); delete widget;}
			else {delete item;}
		}
    	delete layout;
    }
}

bool SelectDeliverySpecificationPage::validatePage() {
	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;
	if (wizard) {
		wizard->SetDeliverySpecificationList(mDeliverySpecificationList);
		wizard->SetDeliverableId(mDeliverySpecificationId);
		return true;
	} else return false;
}

/*
 * SelectCplPage
 */
SelectCplPage::SelectCplPage(QWidget *parent, QVector< QSharedPointer<AssetCpl> > rCplAssets)
	: QWizardPage(parent), mpParent(parent), mCplAssets(rCplAssets)
{
	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;

	setTitle(tr("CPL assignment"));
	setSubTitle(tr("Select one or more CPLs to test against the chosen Delivery Specification (default: Test all CPLs)"));

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

void SelectCplPage::initializePage() {
	mpModelScmCPL->clear();
	SetCplFilesList(mCplAssets);
}

void SelectCplPage::mpTableViewScmCPL_ItemClicked() {
	QMessageBox msgBox;
	msgBox.setText("mpTableViewScmCPL_ItemClicked");
	msgBox.exec();	
}

void SelectCplPage::SetCplFilesList(QVector< QSharedPointer<AssetCpl> > rCplAssets) {

	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;


	int line_count = 0;

	QStandardItem* item = new QStandardItem(true);
	item->setText("Select one or more CPLs");
	mpModelScmCPL->setItem(line_count++, 0, item);



	for (int i = 0; i < rCplAssets.size(); i++) {
		QStandardItem* item0 = new QStandardItem(true);
		item0->setCheckable(true);
		item0->setCheckState(Qt::Unchecked);
		item0->setText(rCplAssets[i].data()->GetPath().fileName());

		QStandardItem* item1 = new QStandardItem(true);
		item1->setText(rCplAssets[i].data()->GetId().toString());
		mpModelScmCPL->setItem(line_count, 0, item0);
		mpModelScmCPL->setItem(line_count++,   1, item1);
		mpModelScmCPL->item(line_count - 1, 0)->setCheckState(Qt::Checked);
	}
}

void SelectCplPage::cleanupPage() {
	mpModelScmCPL->clear();
}

bool SelectCplPage::isComplete() const {
	bool are_mandatory_fields_filled = QWizardPage::isComplete();
	if(selectedOneOfEach()){
		return are_mandatory_fields_filled;
	}
	return false;
}

bool SelectCplPage::selectedOneOfEach() const {
	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;
	QList<QStandardItem*> sidecar_assets;
	bool selectedOne = false;
	int counter = 0;
	int line_count = 0;

	// For each asset, all CPLs of the current IMP plus all assigned CPLs in other IMPs are listed
	int num_cpl_assignments = mCplAssets.size();
	for (int CplList = 1; CplList < num_cpl_assignments + 1; CplList++)
	{
		if (mpModelScmCPL->data(mpModelScmCPL->index(line_count + CplList, 0), Qt::CheckStateRole) == Qt::Checked) {
			selectedOne = true;
		}
	}
	return selectedOne;
}

bool SelectCplPage::handleBackButton() {
	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;
	wizard->back();
	if (wizard->currentId() == WizardDeliverySpecificationCheck::Page_SelectDeliverable) {
		wizard->currentPage()->initializePage();
	}
	return true;
}

bool SelectCplPage::validatePage() {
	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;

	QVector< QSharedPointer<AssetCpl> > cpl_assets_checked;
	for (int CplList = 1; CplList < mCplAssets.size() + 1; CplList++)
	{
		if (mpModelScmCPL->data(mpModelScmCPL->index(CplList, 0), Qt::CheckStateRole) == Qt::Checked) {
			cpl_assets_checked.append(mCplAssets[CplList-1]);
		}
	}
	wizard->SetSelectedCpls(cpl_assets_checked);
	return true;
}

QList<QUuid> SelectCplPage::GetSelectedCpls() const {
	QList<QUuid> items;
	for (int row = 0; row < mpModelScmCPL->rowCount(); row++) {
		if (mpModelScmCPL->item(row, 0)->checkState() == Qt::Checked) {
			items.append(QUuid(mpModelScmCPL->item(row, 1)->text()));
		}
	}
	return items;
}

void SelectCplPage::SetSelectedCpls(QList<QUuid> rSelectedCpls) {
	emit CplListChanged();
	emit completeChanged();
}

void SelectCplPage::CplChecked(QStandardItem *) {
	emit CplListChanged();
	emit completeChanged();
}

/*
 * ResultPage
 */
ResultPage::ResultPage(QWidget *parent)
	: QWizardPage(parent), mpParent(parent)
{
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
	mpMsgBox = new QMessageBox();

	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;
	setTitle(tr("Validation result"));
	setSubTitle(tr("Copy to clipboard or add report as sidecar asset"));
	
	QGridLayout *p_layout_result = new QGridLayout();
	p_layout_result->setVerticalSpacing(3);

	p_layout_result->addWidget(new QLabel(tr("Deliverable Id:"), this), 0, 0, 1, 1);
	mpLineEditDeliverableId = new QLineEdit(this);
	mpLineEditDeliverableId->setAlignment(Qt::AlignLeft);
	mpLineEditDeliverableId->setEnabled(false);
	p_layout_result->addWidget(mpLineEditDeliverableId, 0, 1, 1, 1);

	p_layout_result->addWidget(new QLabel(tr("Label:"), this), 1, 0, 1, 1);
	mpLineEditDeliverable = new QLineEdit(this);
	mpLineEditDeliverable->setAlignment(Qt::AlignLeft);
	mpLineEditDeliverable->setEnabled(false);
	p_layout_result->addWidget(mpLineEditDeliverable, 1, 1, 1, 1);

	mpTree = new QTreeWidget(this);
	mpTree->setColumnCount(2);
	mpTree->setHeaderHidden(true);
	mpTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpTree->header()->setStretchLastSection(false);
	p_layout_result->addWidget(mpTree, 2, 0, 1, 2);
	QAbstractButton *backButton = wizard->button(QWizard::BackButton);
	disconnect(backButton, SIGNAL(clicked(bool)), 0, 0);
	connect(backButton, SIGNAL(clicked()), this, SLOT(handleBackButton()));


	setLayout(p_layout_result);
}

void ResultPage::initializePage() {
	mpTree->clear();
	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;
	dsl::DeliverySpecificationList* dsl = wizard->GetDeliverySpecificationList();
	dsl::DeliverableType_CompositionPlaylistConstraintsType* constraints;
	QUuid deliverable_id = wizard->GetDeliverableId();
	for (dsl::DeliverySpecificationList_DeliverableListType::DeliverableSequence::iterator
			it = dsl->getDeliverableList().getDeliverable().begin();
			it < dsl->getDeliverableList().getDeliverable().end();
			it++) {
		if (ImfXmlHelper::Convert(it->getId()) == deliverable_id) {
			constraints = new dsl::DeliverableType_CompositionPlaylistConstraintsType(it->getCompositionPlaylistConstraints());
			mpLineEditDeliverableId->setText(deliverable_id.toString());
			mpLineEditDeliverable->setText(QString(it->getLabel().c_str()));
		}
	}
	//delete dsl;
	QVector< QSharedPointer<AssetCpl> > selected_cpls = wizard->GetSelectedCpls();
	// 	JobDeliverySpecificationCheck(const QSharedPointer<AssetCpl> rAssetCpl, const dsl::DeliverySpecificationList rDsl);
	for (int i=0; i < selected_cpls.size(); i++ ) {
		qDebug() << "Adding Check to Queue: " << ImfXmlHelper::Convert(constraints->getOwnerId());
		JobDeliverySpecificationCheck *p_dsl_job = new JobDeliverySpecificationCheck(selected_cpls.at(i), constraints);
		connect(p_dsl_job, SIGNAL(Result(const QList<QStringList>)), this, SLOT(ShowResult(const QList<QStringList>)));
		mpJobQueue->AddJob(p_dsl_job);
	}
	mpJobQueue->StartQueue();
}


void ResultPage::ShowResult(const QList<QStringList> rResult) {

	unsigned int index = 0;
	auto t = new QTreeWidgetItem(QStringList() << rResult[index].at(1) << rResult[index].at(2));
	index++;
	while ((index < rResult.size()) && (rResult[index].at(0).compare("1") == 0)) {
		auto i = new QTreeWidgetItem(QStringList() << rResult[index].at(1) << rResult[index].at(2));
		if(((index+1) < rResult.size()) && (rResult[index+1].at(0).compare("1") == 0)) {
			for (int j = 3; j < rResult[index].length(); j++) {
				auto ii = new QTreeWidgetItem(QStringList() << rResult[index].at(j));
				i->addChild(ii);
			}
			index++;
		} else {
			index++;
			while ((index < rResult.size()) && (rResult[index].at(0).compare("2") == 0)) {
				auto ii = new QTreeWidgetItem(QStringList() << rResult[index].at(1) << rResult[index].at(2));
				for (int j = 3; j < rResult[index].length(); j++) {
					auto iii = new QTreeWidgetItem(QStringList() << rResult[index].at(j));
					ii->addChild(iii);
				}
				i->addChild(ii);
				index++;
			}
		}
		t->addChild(i);
	}
	mpTree->addTopLevelItem(t);

    for(int i = 0; i < mpTree->columnCount(); i++)
    	mpTree->resizeColumnToContents(i);

	mpTree->show();
}

bool ResultPage::handleBackButton() {
	WizardDeliverySpecificationCheck* wizard = (WizardDeliverySpecificationCheck *)mpParent;
	wizard->back();
	if (wizard->currentId() == WizardDeliverySpecificationCheck::Page_SelectCpl) {
		wizard->currentPage()->initializePage();
	}
	return true;
}

void ResultPage::rJobQueueFinished() {
	qDebug() << "Queue finished!";
	mpProgressDialog->reset();
	QString error_msg;
	QList<Error> errors = mpJobQueue->GetErrors();
	for(int i = 0; i < errors.size(); i++) {
		error_msg.append(QString("%1: %2\n%3\n").arg(i + 1).arg(errors.at(i).GetErrorMsg()).arg(errors.at(i).GetErrorDescription()));
	}



	error_msg.chop(1); // remove last \n
	if (error_msg != "") {
		mpMsgBox->setText(tr("Critical error, check against Delivery Specification failed:"));
		mpMsgBox->setInformativeText(error_msg + "\n\n Aborting");
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->setIcon(QMessageBox::Critical);
		mpMsgBox->exec();
	}
}


