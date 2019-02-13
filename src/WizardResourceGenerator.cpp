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
#include "WizardResourceGenerator.h"
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
#include "EmptyTimedTextGenerator.h"
#include "SMPTE_Labels.h"


WizardResourceGenerator::WizardResourceGenerator(QWidget *pParent /*= NULL*/, QVector<EditRate> rEditRates /* = QVector<EditRates>()*/, QSharedPointer<AssetMxfTrack> rAsset /* = 0 */) :
QWizard(pParent) {

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setWindowModality(Qt::WindowModal);
	setWizardStyle(QWizard::ModernStyle);
	setStyleSheet("QWizard QPushButton {min-width: 60 px;}");
	mEditRates = rEditRates;
	mAsset = rAsset;
	mReadOnly = (mAsset != NULL);
	if (mReadOnly)
		setWindowTitle(tr("Resource Generator"));
	else
		setWindowTitle(tr("Metadata View"));
	InitLayout();
}

QSize WizardResourceGenerator::sizeHint() const {

	return QSize(600, 600);
}

void WizardResourceGenerator::InitLayout() {

	WizardResourceGeneratorPage *p_wizard_page = new WizardResourceGeneratorPage(this, mEditRates, mReadOnly, mAsset);
	mPageId = addPage(p_wizard_page);
	setOption(QWizard::HaveCustomButton1, true);
	setButtonText(QWizard::CustomButton1, tr("Browse"));
	QList<QWizard::WizardButton> layout;
	if (mReadOnly)
		layout << QWizard::Stretch << QWizard::CancelButton;
	else
		layout << QWizard::CustomButton1 << QWizard::Stretch << QWizard::CancelButton << QWizard::FinishButton;
	setButtonLayout(layout);

	connect(button(QWizard::CustomButton1), SIGNAL(clicked()), p_wizard_page, SLOT(ShowFileDialog()));
	if (mReadOnly && (mAsset != NULL)) { //Display metadata of existing asset
		switch(mAsset->GetEssenceType()) {
		case Metadata::Jpeg2000:
			SwitchMode(eMode::Jpeg2000Mode);
			break;
#ifdef APP5_ACES
		case Metadata::Aces:
			SwitchMode(eMode::ExrMode);
			break;
#endif
		case Metadata::Pcm:
			SwitchMode(eMode::WavMode);
			break;
		case Metadata::TimedText:
			SwitchMode(eMode::TTMLMode);
			break;
		default:
			break;
		}
	}
}

void WizardResourceGenerator::SwitchMode(eMode mode) {

	WizardResourceGeneratorPage *p_wizard_page = qobject_cast<WizardResourceGeneratorPage*>(page(mPageId));
	if(p_wizard_page) {
		p_wizard_page->SwitchMode(mode);
	}
}

WizardResourceGeneratorPage::WizardResourceGeneratorPage(QWidget *pParent /*= NULL*/, QVector<EditRate> rEditRates /* = QVector<EditRate>()*/, bool rReadOnly, QSharedPointer<AssetMxfTrack> rAsset ) :
QWizardPage(pParent), mpFileDialog(NULL), mpSoundFieldGroupModel(NULL), mpTimedTextModel(NULL), mpTableViewExr(NULL), mpTableViewWav(NULL), mpTableViewTimedText(NULL), mpProxyImageWidget(NULL), mpStackedLayout(NULL), mpComboBoxEditRate(NULL),
mpComboBoxSoundfieldGroup(NULL), mpMsgBox(NULL), mpAs02Wrapper(NULL), mpLineEditDuration(NULL), mpComboBoxCplEditRate(NULL) {
	mpAs02Wrapper = new MetadataExtractor(this);
	mReadOnly = rReadOnly;
	mAsset = rAsset;
	if (!mReadOnly) setTitle(tr("Edit Resource"));
	else setTitle(tr("Metadata view"));
	if (!mReadOnly) setSubTitle(tr("Select essence file(s) that should be wrapped as MXF AS-02."));
	else setSubTitle(tr("MXF metadata items cannot be edited!"));
	mEditRates = rEditRates;
	InitLayout();
}

void WizardResourceGeneratorPage::InitLayout() {

	mpMsgBox = new QMessageBox(this);
	mpMsgBox->setMinimumSize(400, 300);
	mpMsgBox->setIcon(QMessageBox::Warning);
	if (mReadOnly) this->setStyleSheet("QLineEdit, QComboBox {color: #b1b1b1;}");

	mpFileDialog = new QFileDialog(this, QString(), QDir::homePath());
	mpFileDialog->setFileMode(QFileDialog::ExistingFiles);
	mpFileDialog->setViewMode(QFileDialog::Detail);
	mpFileDialog->setNameFilters(QStringList() << "*.exr" << "*.wav" << "*.ttml");
	mpFileDialog->setIconProvider(new IconProviderExrWav(this)); // TODO: Does not work.

	mpProxyImageWidget = new WidgetProxyImage(this);

	mpComboBoxEditRate = new QComboBox(this);
	mpComboBoxEditRate->setWhatsThis(tr("The edit rate can be changed later as long as the resource is not played out."));
	QStringListModel *p_edit_rates_model = new QStringListModel(this);
	p_edit_rates_model->setStringList(EditRate::GetFrameRateNames());
	mpComboBoxEditRate->setModel(p_edit_rates_model);


	//--- soundfield group ---
	mpComboBoxSoundfieldGroup = new QComboBox(this);
	mpComboBoxSoundfieldGroup->setWhatsThis(tr("Select a soundfield group. Every soundfield group channel must be assigned."));
	QStringListModel *p_sound_field_group_model = new QStringListModel(this);
	p_sound_field_group_model->setStringList(SoundfieldGroup::GetSoundFieldGroupNames());
	mpComboBoxSoundfieldGroup->setModel(p_sound_field_group_model);
	if (mReadOnly) mpComboBoxSoundfieldGroup->setDisabled(true);
	mpSoundFieldGroupModel = new SoundFieldGroupModel(this);
	mpTableViewWav = new QTableView(this);
	mpTableViewWav->setModel(mpSoundFieldGroupModel);
	mpTableViewWav->setEditTriggers(QAbstractItemView::AllEditTriggers);
	mpTableViewWav->setSelectionBehavior(QAbstractItemView::SelectRows);
	mpTableViewWav->setSelectionMode(QAbstractItemView::NoSelection);
	mpTableViewWav->setShowGrid(false);
	mpTableViewWav->horizontalHeader()->setHidden(true);
	mpTableViewWav->horizontalHeader()->setStretchLastSection(true);
	mpTableViewWav->verticalHeader()->setHidden(true);
	mpTableViewWav->resizeRowsToContents();
	mpTableViewWav->resizeColumnsToContents();
	mpTableViewWav->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpTableViewWav->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpTableViewWav->setItemDelegateForColumn(SoundFieldGroupModel::ColumnDstChannel, new DelegateComboBox(this, false, false));
	//WR
	//language code is two or three lowercase letters, region code is either two uppercase letters or three digits"
	const QRegExp rx_lang("[a-z]{2,3}\\-([A-Z]{2}|[0-9]{3})");
	QRegExpValidator *v_lang = new QRegExpValidator(rx_lang, this);
	mpLineEditLanguageTagWav = new QLineEdit(this);
	mpLineEditLanguageTagWav->setAlignment(Qt::AlignRight);
	if (!mReadOnly) mpLineEditLanguageTagWav->setPlaceholderText("en-US");
	mpLineEditLanguageTagWav->setValidator(v_lang);
	if (mReadOnly) mpLineEditLanguageTagWav->setDisabled(true);
	connect(mpLineEditLanguageTagWav, SIGNAL(textEdited(QString)), this, SLOT(languageTagWavChanged()));
	QRegExpValidator *v_lang2 = new QRegExpValidator(rx_lang, this);
	mpLineEditLanguageTagTT = new QLineEdit(this);
	mpLineEditLanguageTagTT->setAlignment(Qt::AlignRight);
	if (!mReadOnly) mpLineEditLanguageTagTT->setPlaceholderText("en-US");
	mpLineEditLanguageTagTT->setValidator(v_lang2);
	if (mReadOnly) mpLineEditLanguageTagTT->setDisabled(true);
	connect(mpLineEditLanguageTagTT, SIGNAL(textEdited(QString)), this, SLOT(languageTagTTChanged()));
	QRegExp mca_items("[0-9a-zA-Z_]{1,15}");
	QRegExpValidator *v_mca_items = new QRegExpValidator(mca_items, this);
	mpLineEditMCATitle = new QLineEdit(this);
	mpLineEditMCATitle->setAlignment(Qt::AlignRight);
	mpLineEditMCATitle->setText("Default Title");
	mpLineEditMCATitle->setValidator(v_mca_items);
	if (mReadOnly) mpLineEditMCATitle->setDisabled(true);
	//connect(mpLineEditMCATitle, SIGNAL(textEdited(QString)), this, SLOT(mcaTitleChanged()));
	mpLineEditMCATitleVersion = new QLineEdit(this);
	mpLineEditMCATitleVersion->setAlignment(Qt::AlignRight);
	mpLineEditMCATitleVersion->setText("Domestic");
	mpLineEditMCATitleVersion->setValidator(v_mca_items);
	if (mReadOnly) mpLineEditMCATitleVersion->setDisabled(true);
	//connect(mpLineEditMCATitleVersion, SIGNAL(textEdited(QString)), this, SLOT(mcaTitleVersionChanged()));
	mpLineEditMCAAudioContentKind = new QLineEdit(this);
	mpLineEditMCAAudioContentKind->setAlignment(Qt::AlignRight);
	mpLineEditMCAAudioContentKind->setText("Master");
	mpLineEditMCAAudioContentKind->setValidator(v_mca_items);
	if (mReadOnly) mpLineEditMCAAudioContentKind->setDisabled(true);
	//connect(mpLineEditMCAAudioContentKind, SIGNAL(textEdited(QString)), this, SLOT(mcaAudioContentKindChanged()));
	mpLineEditMCAAudioElementKind = new QLineEdit(this);
	mpLineEditMCAAudioElementKind->setAlignment(Qt::AlignRight);
	mpLineEditMCAAudioElementKind->setText("Composite Mix");
	mpLineEditMCAAudioElementKind->setValidator(v_mca_items);
	if (mReadOnly) mpLineEditMCAAudioElementKind->setDisabled(true);
	//connect(mpLineEditMCAAudioElementKind, SIGNAL(textEdited(QString)), this, SLOT(mcaAudioElementKindChanged()));
	mpComboBoxCplEditRate = new QComboBox(this);
	mpComboBoxCplEditRate->setWhatsThis(tr("Select a frame rate. It shall match the CPL Edit Rate"));
	QStringListModel *p_edit_rate_group_model = new QStringListModel(this);
	QStringList p_edit_rate_string_list;
	for (QVector<EditRate>::iterator i=mEditRates.begin(); i < mEditRates.end(); i++) {
		p_edit_rate_string_list << i->GetName();
	}
	p_edit_rate_group_model->setStringList(p_edit_rate_string_list);
	mpComboBoxCplEditRate->setModel(p_edit_rate_group_model);
	if (mReadOnly) mpComboBoxCplEditRate->setDisabled(true);
	//WR

			/* -----Denis Manthey----- */

	mpTimedTextModel = new TimedTextModel(this);
	mpTableViewTimedText = new QTableView(this);
	mpTableViewTimedText->setModel(mpTimedTextModel);
	mpTableViewTimedText->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mpTableViewTimedText->setSelectionBehavior(QAbstractItemView::SelectRows);
	mpTableViewTimedText->setSelectionMode(QAbstractItemView::SingleSelection);
	mpTableViewTimedText->setShowGrid(false);
	mpTableViewTimedText->horizontalHeader()->setHidden(true);
	mpTableViewTimedText->horizontalHeader()->setStretchLastSection(true);
	mpTableViewTimedText->verticalHeader()->setHidden(true);
	mpTableViewTimedText->resizeRowsToContents();
	mpTableViewTimedText->resizeColumnsToContents();

	QPushButton *pGenNew = new QPushButton("Generate Empty Timed Text Resource");
	pGenNew->setAutoDefault(false);
	mpGroupBox = new QGroupBox;
	mpDirDialog = new QFileDialog(this, QString(), QDir::homePath());
	mpDirDialog->setFileMode(QFileDialog::Directory);
	mpDirDialog->setOption(QFileDialog::ShowDirsOnly);
	mpLineEditFileDir = new QLineEdit(this);
	mpLineEditFileDir->setEnabled(false);
	mpLineEditFileDir->setAlignment(Qt::AlignRight);
	QPushButton *pBrowseDir = new QPushButton(this);
	pBrowseDir->setText(tr("Browse"));
	pBrowseDir->setAutoDefault(false);
	//connect(pBrowseDir, SIGNAL(clicked(bool)), this, SLOT(ShowDirDialog()));
	connect(mpDirDialog, SIGNAL(fileSelected(const QString &)), mpLineEditFileDir, SLOT(setText(const QString &)));
	QRegExp rx("[A-Za-z0-9-_]+");
	QRegExpValidator *v = new QRegExpValidator(rx, this);
	mpLineEditFileName = new QLineEdit(this);
	mpLineEditFileName->setAlignment(Qt::AlignRight);
	mpLineEditFileName->setPlaceholderText("file name");
	mpLineEditFileName->setValidator(v);
	mpLineEditDuration = new QLineEdit(this);
	mpLineEditDuration->setAlignment(Qt::AlignRight);
	mpLineEditDuration->setPlaceholderText("Duration [frames]");
	mpLineEditDuration->setValidator(new QIntValidator(this));
	mpGenerateEmpty_button = new QPushButton(this);
	mpGenerateEmpty_button->setText(tr("Generate"));
	mpGenerateEmpty_button->setAutoDefault(false);
	mpGenerateEmpty_button->setEnabled(false);
	connect(pBrowseDir, SIGNAL(clicked(bool)), this, SLOT(ShowDirDialog()));
	connect(mpDirDialog, SIGNAL(fileSelected(const QString &)), mpLineEditFileDir, SLOT(setText(const QString &)));
	connect(pGenNew,SIGNAL(clicked()),this,SLOT(hideGroupBox()));
	connect(mpLineEditDuration, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
	connect(mpLineEditFileName, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
	connect(mpLineEditFileDir, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
	connect(mpGenerateEmpty_button, SIGNAL(clicked(bool)), this, SLOT(GenerateEmptyTimedText()));
	QLabel *GenNew = new QLabel(this);
	GenNew->setStyleSheet("font: bold; text-decoration: underline");
	GenNew->setText("Generate Empty Timed Text Resource");
	mpTimedTextModel = new TimedTextModel(this);
	mpTableViewTimedText = new QTableView(this);
	mpTableViewTimedText->setModel(mpTimedTextModel);
	mpTableViewTimedText->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mpTableViewTimedText->setSelectionBehavior(QAbstractItemView::SelectRows);
	mpTableViewTimedText->setSelectionMode(QAbstractItemView::SingleSelection);
	mpTableViewTimedText->setShowGrid(false);
	mpTableViewTimedText->horizontalHeader()->setHidden(true);
	mpTableViewTimedText->horizontalHeader()->setStretchLastSection(true);
	mpTableViewTimedText->verticalHeader()->setHidden(true);
	mpTableViewTimedText->resizeRowsToContents();
	mpTableViewTimedText->resizeColumnsToContents();

			/* -----Denis Manthey----- */


	QWidget *p_wrapper_widget_one = new QWidget(this);
	QGridLayout *p_wrapper_layout_one = new QGridLayout();
	p_wrapper_layout_one->setContentsMargins(0, 0, 0, 0);
	p_wrapper_layout_one->addWidget(new QLabel(tr("Frame Rate:"), this), 0, 0, 1, 1);
	p_wrapper_layout_one->addWidget(mpComboBoxEditRate, 0, 1, 1, 1);
	QWidget *p_wrapper_widget_two = new QWidget(this);
	QGridLayout *p_wrapper_layout_two = new QGridLayout();
	p_wrapper_layout_two->setContentsMargins(0, 0, 0, 0);
	p_wrapper_layout_two->addWidget(new QLabel(tr("Soundfield group:"), this), 0, 0, 1, 1);
	p_wrapper_layout_two->addWidget(mpComboBoxSoundfieldGroup, 0, 1, 1, 1);
	p_wrapper_layout_two->addWidget(new QLabel(tr("RFC 5646 Language Tag (e.g. en-US):"), this), 1, 0, 1, 1);
	p_wrapper_layout_two->addWidget(mpLineEditLanguageTagWav, 1, 1, 1, 1);
	p_wrapper_layout_two->addWidget(new QLabel(tr("MCA Title:"), this), 2, 0, 1, 1);
	p_wrapper_layout_two->addWidget(mpLineEditMCATitle, 2, 1, 1, 1);
	p_wrapper_layout_two->addWidget(new QLabel(tr("MCA Title Version:"), this), 3, 0, 1, 1);
	p_wrapper_layout_two->addWidget(mpLineEditMCATitleVersion, 3, 1, 1, 1);
	p_wrapper_layout_two->addWidget(new QLabel(tr("MCA Audio Content Kind:"), this), 4, 0, 1, 1);
	p_wrapper_layout_two->addWidget(mpLineEditMCAAudioContentKind, 4, 1, 1, 1);
	p_wrapper_layout_two->addWidget(new QLabel(tr("MCA Audio Element Kind:"), this), 5, 0, 1, 1);
	p_wrapper_layout_two->addWidget(mpLineEditMCAAudioElementKind, 5, 1, 1, 1);
	p_wrapper_layout_two->addWidget(mpTableViewWav, 6, 0, 1, 2);
	p_wrapper_widget_two->setLayout(p_wrapper_layout_two);


			/* -----Denis Manthey----- */

	QWidget *p_wrapper_widget_three = new QWidget(this);
	QGridLayout *p_wrapper_layout_three = new QGridLayout();
	QGridLayout *vbox = new QGridLayout;
	p_wrapper_layout_three->setContentsMargins(0, 0, 0, 0);
	if (!mReadOnly) p_wrapper_layout_three->addWidget(new QLabel(tr("Select a Timed Text Resource (.xml) compliant to IMSC1"), this), 0, 0, 1, 3);
	p_wrapper_layout_three->addWidget(new QLabel(tr("CPL Edit Rate:"), this), 1, 0, 1, 2);
	p_wrapper_layout_three->addWidget(mpComboBoxCplEditRate, 1, 2, 1, 1);
	p_wrapper_layout_three->addWidget(new QLabel(tr("RFC 5646 Language Tag (e.g. en-US):"), this), 2, 0, 1, 2);
	p_wrapper_layout_three->addWidget(mpLineEditLanguageTagTT, 2, 2, 1, 1);
	p_wrapper_layout_three->addWidget(mpTableViewTimedText, 3, 0, 1, 3);
	if (!mReadOnly) p_wrapper_layout_three->addWidget(pGenNew, 4, 0, 1, 3);

	vbox->addWidget(new QLabel(tr("Set the file name of the empty tt resource:"), this), 1, 0, 1, 1);
	vbox->addWidget(mpLineEditFileName, 1, 1, 1, 1);
	vbox->addWidget(new QLabel(tr(".xml"), this), 1, 2, 1, 1);
	vbox->addWidget(new QLabel(tr("Set the directory of the empty tt resource:"), this), 2, 0, 1, 1);
	vbox->addWidget(mpLineEditFileDir, 2, 1, 1, 1);
	vbox->addWidget(pBrowseDir, 2, 2, 1, 1);
	vbox->addWidget(new QLabel(tr("Set the duration of the empty tt resource:"), this), 3, 0, 1, 1);
	vbox->addWidget(mpLineEditDuration, 3, 1, 1, 1);
	vbox->addWidget(mpGenerateEmpty_button, 3, 2, 1, 1);
	mpGroupBox->setLayout(vbox);
	mpGroupBox->hide();

	p_wrapper_layout_three->addWidget(mpGroupBox, 3, 0, 1, 3);
	p_wrapper_widget_three->setLayout(p_wrapper_layout_three);


			/* -----Denis Manthey----- */

	QWidget *p_wrapper_widget_four = new QWidget(this);
	QGridLayout *p_wrapper_layout_four = new QGridLayout();
	p_wrapper_layout_four->setContentsMargins(0, 0, 0, 0);

	if (mReadOnly && mAsset) {
		int i = -1;
		Metadata metadata = mAsset->GetMetadata();
		QLabel* label = new QLabel();
		label->setTextInteractionFlags(Qt::TextSelectableByMouse);
		label->setText(metadata.assetId.toString());
		p_wrapper_layout_four->addWidget(new QLabel(tr("Track File ID:")), ++i, 0, 1, 1);
		p_wrapper_layout_four->addWidget(label, i, 1, 1, 1);
		label = new QLabel();
		label->setTextInteractionFlags(Qt::TextSelectableByMouse);
		label->setText(metadata.pictureEssenceCoding);
		p_wrapper_layout_four->addWidget(new QLabel(tr("Picture Essence Encoding UL:")), ++i, 0, 1, 1);
		p_wrapper_layout_four->addWidget(label, i, 1, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(tr("Picture Essence Encoding:")), ++i, 0, 1, 1);
		if (SMPTE::J2K_ProfilesMap.contains(metadata.pictureEssenceCoding))
			p_wrapper_layout_four->addWidget(new QLabel(SMPTE::vJ2K_Profiles[SMPTE::J2K_ProfilesMap[metadata.pictureEssenceCoding]]), i, 1, 1, 1);
		else if (SMPTE::ACES_ProfilesMap.contains(metadata.pictureEssenceCoding))
			p_wrapper_layout_four->addWidget(new QLabel(SMPTE::vACES_Profiles[SMPTE::ACES_ProfilesMap[metadata.pictureEssenceCoding]]), i, 1, 1, 1);
		else
			p_wrapper_layout_four->addWidget(new QLabel("Unknown"), ++i, 1, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(tr("Duration:")), ++i, 0, 1, 1);
		if(metadata.duration.IsValid() && metadata.editRate.IsValid())
			p_wrapper_layout_four->addWidget(new QLabel(metadata.duration.GetAsString(metadata.editRate)), i, 1, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(tr("Frame Rate:")), ++i, 0, 1, 1);
		if(metadata.editRate.IsValid() == true)
			p_wrapper_layout_four->addWidget(new QLabel(metadata.editRate.GetName()), i, 1, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(tr("Stored Resolution:")), ++i, 0, 1, 1);
		if(metadata.storedHeight != 0 || metadata.storedWidth != 0)
			p_wrapper_layout_four->addWidget(new QLabel(QObject::tr("%1 x %2").arg(metadata.storedWidth).arg(metadata.storedHeight)), i, 1, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(tr("Displayed Resolution:")), ++i, 0, 1, 1);
		if(metadata.displayHeight != 0 || metadata.displayWidth != 0)
			p_wrapper_layout_four->addWidget(new QLabel(QObject::tr("%1 x %2").arg(metadata.displayWidth).arg(metadata.displayHeight)), i, 1, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(tr("Aspect Ratio:")), ++i, 0, 1, 1);
		if(metadata.aspectRatio != ASDCP::Rational())
			p_wrapper_layout_four->addWidget(new QLabel(QObject::tr("%1 (%2:%3)").arg(metadata.aspectRatio.Quotient()).arg(metadata.aspectRatio.Numerator).arg(metadata.aspectRatio.Denominator)), i, 1, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(tr("Color Mode:")), ++i, 0, 1, 1);
		if(metadata.horizontalSubsampling != 0 && metadata.colorEncoding != metadata.Unknown_Color_Encoding) {
			if(metadata.colorEncoding == Metadata::RGBA)
				p_wrapper_layout_four->addWidget(new QLabel("RGB"), i, 1, 1, 1);
			else if(metadata.colorEncoding == Metadata::CDCI)
				p_wrapper_layout_four->addWidget(new QLabel("YCbCr"), i, 1, 1, 1);
		}
		p_wrapper_layout_four->addWidget(new QLabel(tr("Color Sampling:")), ++i, 0, 1, 1);
		if(metadata.horizontalSubsampling != 0)
			p_wrapper_layout_four->addWidget(new QLabel(QObject::tr("%1:%2:%3").arg(4).arg(4 / metadata.horizontalSubsampling).arg(4 / metadata.horizontalSubsampling)), i, 1, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(tr("Color Depth:")), ++i, 0, 1, 1);
		if(metadata.componentDepth == 253) {
			p_wrapper_layout_four->addWidget(new QLabel(QObject::tr("16 bit float")), i, 1, 1, 1);
		} else if(metadata.componentDepth != 0) {
			p_wrapper_layout_four->addWidget(new QLabel(QObject::tr("%1 bit").arg(metadata.componentDepth)), i, 1, 1, 1);
		}
		p_wrapper_layout_four->addWidget(new QLabel(tr("Primaries:")), ++i, 0, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(SMPTE::vColorPrimaries[metadata.colorPrimaries]), i, 1, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(tr("OETF:")), ++i, 0, 1, 1);
		p_wrapper_layout_four->addWidget(new QLabel(SMPTE::vTransferCharacteristic[metadata.transferCharcteristics]), i, 1, 1, 1);

		QWidget* empty = new QWidget();
		empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
		p_wrapper_layout_four->addWidget(empty);

	}

	p_wrapper_widget_four->setLayout(p_wrapper_layout_four);


	mpStackedLayout = new QStackedLayout();
	mpStackedLayout->insertWidget(WizardResourceGeneratorPage::WavIndex, p_wrapper_widget_two);
	mpStackedLayout->insertWidget(WizardResourceGeneratorPage::TTMLIndex, p_wrapper_widget_three);
	mpStackedLayout->insertWidget(WizardResourceGeneratorPage::Jpeg2000Index, p_wrapper_widget_four);
	mpStackedLayout->insertWidget(WizardResourceGeneratorPage::ExrIndex, p_wrapper_widget_four);
	setLayout(mpStackedLayout);

	registerField(FIELD_NAME_SELECTED_FILES"*", this, "FilesSelected", SIGNAL(FilesListChanged()));
	registerField(FIELD_NAME_SOUNDFIELD_GROUP, this, "SoundfieldGroupSelected", SIGNAL(SoundfieldGroupChanged()));
	registerField(FIELD_NAME_EDIT_RATE, this, "EditRateSelected", SIGNAL(EditRateChanged()));
	registerField(FIELD_NAME_DURATION, this, "DurationSelected", SIGNAL(DurationChanged()));
	//WR
	registerField(FIELD_NAME_LANGUAGETAG_WAV, this, "LanguageTagWavSelected", SIGNAL(LanguageTagWavChanged()));
	registerField(FIELD_NAME_LANGUAGETAG_TT, this, "LanguageTagTTSelected", SIGNAL(LanguageTagTTChanged()));
	registerField(FIELD_NAME_MCA_TITLE, this, "MCATitleSelected", SIGNAL(MCATitleChanged()));
	registerField(FIELD_NAME_MCA_TITLE_VERSION, this, "MCATitleVersionSelected", SIGNAL(MCATitleVersionChanged()));
	registerField(FIELD_NAME_MCA_AUDIO_CONTENT_KIND, this, "MCAAudioContentKindSelected", SIGNAL(MCAAudioContentKindChanged()));
	registerField(FIELD_NAME_MCA_AUDIO_ELEMENT_KIND, this, "MCAAudioElementKindSelected", SIGNAL(MCAAudioElementKindChanged()));
	registerField(FIELD_NAME_CPL_EDIT_RATE, this, "CplEditRateSelected", SIGNAL(CplEditRateChanged()));
	//WR

	connect(mpFileDialog, SIGNAL(filesSelected(const QStringList &)), this, SLOT(SetSourceFiles(const QStringList &)));
	connect(mpComboBoxSoundfieldGroup, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(ChangeSoundfieldGroup(const QString&)));
	connect(mpSoundFieldGroupModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int>&)), this, SIGNAL(completeChanged()));
}

//set "Generate" button enabled if all necessary values are edited
void WizardResourceGeneratorPage::textChanged()
{
	if (mpLineEditDuration->text().toInt() > 0 && !mpLineEditFileName->text().isEmpty() && !mpLineEditFileDir->text().isEmpty()){
		mpGenerateEmpty_button->setEnabled(true);
	}
	else
		mpGenerateEmpty_button->setEnabled(false);
}
//WR
void WizardResourceGeneratorPage::languageTagWavChanged()
{
	QRegExp rx_lang("[a-z]{2,3}\\-([A-Z]{2}|[0-9]{3})");
	// ^$ empty string
	//qDebug() << mpLineEditLanguageTagWav->text();
	// TO-DO: Disable QWizard::FinishButton if string doesn't match regexp
}
void WizardResourceGeneratorPage::languageTagTTChanged()
{
	QRegExp rx_lang("[a-z]{2,3}\\-([A-Z]{2}|[0-9]{3})");
	// ^$ empty string
	//qDebug() << mpLineEditLanguageTagWav->text();
	// TO-DO: Disable QWizard::FinishButton if string doesn't match regexp
	//rx_lang.exactMatch(mpLineEditLanguageTagTT->text())
}
//WR


//hide/show Generate Empty context, when "Generate Empty Timed Text Resource" button is clicked
void WizardResourceGeneratorPage::hideGroupBox() {

	if (mGroupBoxCheck == 0) {
		mpGroupBox->show();
		mGroupBoxCheck = 1;
	}
	else {
		mpGroupBox->hide();
		mGroupBoxCheck = 0;
	}
}

void WizardResourceGeneratorPage::GenerateEmptyTimedText(){

	QStringList filePath(tr("%1/%2.xml").arg(mpLineEditFileDir->text()).arg(mpLineEditFileName->text()));
	QFileInfo *file = new QFileInfo(mpLineEditFileDir->text());
	QString dur(tr("%1f").arg(mpLineEditDuration->text()));

	//check if filename already exists in directory
	if (QFile::exists(filePath.at(0))) {
		mpMsgBox->setText(tr("Error"));
		mpMsgBox->setInformativeText(tr("Filename %1.xml exists in directory %2").arg(mpLineEditFileName->text()).arg(mpLineEditFileDir->text()));
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->exec();
		mpLineEditFileName->clear();
	}

	//check for writing permissions
	else if (file->isWritable() == false) {
		mpMsgBox->setText(tr("Permission denied"));
		mpMsgBox->setInformativeText(tr("make sure you have write permission for folder %1").arg(mpLineEditFileDir->text()));
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->exec();
		mpLineEditFileDir->clear();
	}
	else {
		mpEmptyTt = new EmptyTimedTextGenerator(filePath.at(0), dur, GetCplEditRate());
		mpTimedTextModel->SetFile(filePath);
		mpTableViewTimedText->resizeRowsToContents();
		mpTableViewTimedText->resizeColumnsToContents();
		SwitchMode(WizardResourceGenerator::TTMLMode);
		emit FilesListChanged();
		mpGroupBox->hide();
		mGroupBoxCheck = 0;
		mpLineEditFileDir->clear();
		mpLineEditDuration->clear();
		mpLineEditFileName->clear();
	}
}

void WizardResourceGeneratorPage::SetSourceFiles(const QStringList &rFiles) {

	mpFileDialog->hide();
	mpComboBoxEditRate->setEnabled(true);
	if(rFiles.isEmpty() == false) {
		if(is_wav_file(rFiles.at(0))) {
			// Check sampling rate and bit depth consistence.
			Metadata metadata;
			mpAs02Wrapper->ReadMetadata(metadata, rFiles.at(0));
			for(int i = 0; i < rFiles.size(); i++) {
				Metadata other_metadata;
				mpAs02Wrapper->ReadMetadata(other_metadata, rFiles.at(0));
				if(other_metadata.editRate != EditRate::EditRate48000 && other_metadata.editRate != EditRate::EditRate96000) {
					mpMsgBox->setText(tr("Unsupported Sampling Rate"));
					mpMsgBox->setInformativeText(tr("%1 (%2 Hz). Only 48000 Hz and 96000 Hz are supported.").arg(other_metadata.fileName).arg(other_metadata.editRate.GetQuotient()));
					mpMsgBox->setStandardButtons(QMessageBox::Ok);
					mpMsgBox->setDefaultButton(QMessageBox::Ok);
					mpMsgBox->exec();
					return;
				}
				if(other_metadata.audioQuantization != 24) {
					mpMsgBox->setText(tr("Unsupported Bit Depth"));
					mpMsgBox->setInformativeText(tr("%1 (%2 bit). Only 24 bit are supported.").arg(other_metadata.fileName).arg(other_metadata.audioQuantization));
					mpMsgBox->setStandardButtons(QMessageBox::Ok);
					mpMsgBox->setDefaultButton(QMessageBox::Ok);
					mpMsgBox->exec();
					return;
				}
				if(other_metadata.editRate != metadata.editRate) {
					mpMsgBox->setText(tr("Sampling Rate mismatch"));
					mpMsgBox->setInformativeText(tr("Mismatch between %1 (%2 Hz) and %3 (%4 Hz)").arg(metadata.fileName).arg(metadata.editRate.GetQuotient()).arg(other_metadata.fileName).arg(other_metadata.editRate.GetQuotient()));
					mpMsgBox->setStandardButtons(QMessageBox::Ok);
					mpMsgBox->setDefaultButton(QMessageBox::Ok);
					mpMsgBox->exec();
					return;
				}
				if(other_metadata.duration != metadata.duration) {
					mpMsgBox->setText(tr("Duration mismatch"));
					mpMsgBox->setInformativeText(tr("Mismatch between %1 (%2) and %3 (%4)").arg(metadata.fileName).arg(metadata.duration.GetAsString(metadata.editRate)).arg(other_metadata.fileName).arg(other_metadata.duration.GetAsString(other_metadata.editRate)));
					mpMsgBox->setStandardButtons(QMessageBox::Ok);
					mpMsgBox->setDefaultButton(QMessageBox::Ok);
					mpMsgBox->exec();
					return;
				}
			}
			for(int i = 0; i < mpSoundFieldGroupModel->rowCount(); i++) mpTableViewWav->closePersistentEditor(mpSoundFieldGroupModel->index(i, SoundFieldGroupModel::ColumnDstChannel));
			mpSoundFieldGroupModel->SetFilesList(rFiles);
			for(int i = 0; i < mpSoundFieldGroupModel->rowCount(); i++) mpTableViewWav->openPersistentEditor(mpSoundFieldGroupModel->index(i, SoundFieldGroupModel::ColumnDstChannel));
			SwitchMode(WizardResourceGenerator::WavMode);
			emit FilesListChanged();
		}


			/* -----Denis Manthey----- */

		else if(is_ttml_file(rFiles.at(0))) {

			Metadata metadata;
			Error error;
			QDialog *pEditDur = new QDialog(this);

			mpLineEditDuration = new QLineEdit(this);
			mpLineEditDuration->setValidator(new QIntValidator(this));
			mpLineEditDuration->setAlignment(Qt::AlignRight);
			mpLineEditDuration->setPlaceholderText("Duration [frames]");
			QPushButton *pOk = new QPushButton("OK");
			QPushButton *pCancel = new QPushButton("Cancel");

			QGridLayout *pEditDurLay = new QGridLayout();
			pEditDurLay->addWidget(new QLabel(tr("Duration could not be resolved.\n\nEnter a higher duration than needed.\nDuration can be shortened in the timeline"), this), 0, 0, 1, 2);
			pEditDurLay->addWidget(mpLineEditDuration, 1, 0, 1, 2);
			pEditDurLay->addWidget(pOk, 2, 0, 1, 1);
			pEditDurLay->addWidget(pCancel, 2, 1, 1, 1);

			pEditDur->setLayout(pEditDurLay);

			connect(pCancel, SIGNAL(clicked(bool)), pEditDur, SLOT(close()));
			connect(pOk, SIGNAL(clicked(bool)), pEditDur, SLOT(accept()));

			mpAs02Wrapper->SetCplEditRate(GetCplEditRate());
			error = mpAs02Wrapper->ReadMetadata(metadata, rFiles.at(0));
			if(error.IsError()){
				mpMsgBox->setText(error.GetErrorDescription());
				mpMsgBox->setInformativeText(error.GetErrorMsg());
				mpMsgBox->setStandardButtons(QMessageBox::Ok);
				mpMsgBox->setDefaultButton(QMessageBox::Ok);
				mpMsgBox->exec();
				return;
			}
			while(metadata.duration.GetCount() == 0) {

				int ret = pEditDur->exec();
				switch (ret) {
					case 1:
						metadata.duration = Duration(mpLineEditDuration->text().toInt());
						break;
					case 0:
						return;
				}
			}
			SwitchMode(WizardResourceGenerator::TTMLMode);
			mpTimedTextModel->SetFile(rFiles);
			mpTableViewTimedText->resizeRowsToContents();
			mpTableViewTimedText->resizeColumnsToContents();
			emit FilesListChanged();
		}
		/* -----Denis Manthey----- */


	}
}


QStringList WizardResourceGeneratorPage::GetFilesList() const {

	if(mpStackedLayout->currentIndex() == WizardResourceGeneratorPage::WavIndex) {
		return mpSoundFieldGroupModel->GetSourceFiles();
	}



		/* -----Denis Manthey----- */

	else if(mpStackedLayout->currentIndex() == WizardResourceGeneratorPage::TTMLIndex) {
		return mpTimedTextModel->GetSourceFile();
	}
		/* -----Denis Manthey----- */


	return QStringList();
}

void WizardResourceGeneratorPage::ShowFileDialog() {

	mpFileDialog->show();
}

void WizardResourceGeneratorPage::ShowDirDialog() {

	mpDirDialog->show();
}

SoundfieldGroup WizardResourceGeneratorPage::GetSoundfieldGroup() const {

	if(mpStackedLayout->currentIndex() == WizardResourceGeneratorPage::WavIndex) {
		return mpSoundFieldGroupModel->GetSoundfieldGroup();
	}
	return SoundfieldGroup();
}

void WizardResourceGeneratorPage::SetSoundfieldGroup(const SoundfieldGroup &rSoundfieldGroup) {

	for(int i = 0; i < mpSoundFieldGroupModel->rowCount(); i++) mpTableViewWav->closePersistentEditor(mpSoundFieldGroupModel->index(i, SoundFieldGroupModel::ColumnDstChannel));
	mpComboBoxSoundfieldGroup->setCurrentText(rSoundfieldGroup.GetName());
	mpSoundFieldGroupModel->SetSoundfieldGroup(rSoundfieldGroup);
	for(int i = 0; i < mpSoundFieldGroupModel->rowCount(); i++) mpTableViewWav->openPersistentEditor(mpSoundFieldGroupModel->index(i, SoundFieldGroupModel::ColumnDstChannel));
	emit SoundfieldGroupChanged();
	emit completeChanged();
}

void WizardResourceGeneratorPage::ChangeSoundfieldGroup(const QString &rName) {

	for(int i = 0; i < mpSoundFieldGroupModel->rowCount(); i++) mpTableViewWav->closePersistentEditor(mpSoundFieldGroupModel->index(i, SoundFieldGroupModel::ColumnDstChannel));
	mpSoundFieldGroupModel->ChangeSoundfieldGroup(rName);
	for(int i = 0; i < mpSoundFieldGroupModel->rowCount(); i++) mpTableViewWav->openPersistentEditor(mpSoundFieldGroupModel->index(i, SoundFieldGroupModel::ColumnDstChannel));
	emit completeChanged();
}

void WizardResourceGeneratorPage::SetEditRate(const EditRate &rEditRate) {

	mpComboBoxEditRate->setCurrentText(rEditRate.GetName());
	emit EditRateChanged();
	emit completeChanged();
}

void WizardResourceGeneratorPage::SetDuration(const Duration &rDuration) {

	emit DurationChanged();
	emit completeChanged();
}

//WR
void WizardResourceGeneratorPage::SetLanguageTagWav(const QString &rLanguageTag) {
	mpLineEditLanguageTagWav->setText(rLanguageTag);
	emit LanguageTagWavChanged();
	emit completeChanged();
}

void WizardResourceGeneratorPage::SetLanguageTagTT(const QString &rLanguageTag) {
	mpLineEditLanguageTagTT->setText(rLanguageTag);
	emit LanguageTagTTChanged();
	emit completeChanged();
}

void WizardResourceGeneratorPage::SetMCATitle(const QString &text) {
	mpLineEditMCATitle->setText(text);
	emit MCATitleChanged();
	emit completeChanged();
}

void WizardResourceGeneratorPage::SetMCATitleVersion(const QString &text) {
	mpLineEditMCATitleVersion->setText(text);
	emit MCATitleVersionChanged();
	emit completeChanged();
}

void WizardResourceGeneratorPage::SetMCAAudioContentKind(const QString &text) {
	mpLineEditMCAAudioContentKind->setText(text);
	emit MCAAudioContentKindChanged();
	emit completeChanged();
}

void WizardResourceGeneratorPage::SetMCAAudioElementKind(const QString &text) {
	mpLineEditMCAAudioElementKind->setText(text);
	emit MCAAudioElementKindChanged();
	emit completeChanged();
}

void WizardResourceGeneratorPage::SetCplEditRate(const EditRate &rEditRate) {
	mpComboBoxCplEditRate->setCurrentText(rEditRate.GetName());
	emit CplEditRateChanged();
	emit completeChanged();
}
//WR


EditRate WizardResourceGeneratorPage::GetEditRate() const {

	return EditRate::GetEditRate(mpComboBoxEditRate->currentText());
}

Duration WizardResourceGeneratorPage::GetDuration() const {

	return Duration(mpLineEditDuration->text().toInt());
}

//WR
QString WizardResourceGeneratorPage::GetLanguageTagWav() const {

	return mpLineEditLanguageTagWav->text();
}

QString WizardResourceGeneratorPage::GetLanguageTagTT() const {

	return mpLineEditLanguageTagTT->text();
}

QString WizardResourceGeneratorPage::GetMCATitle() const {

	return mpLineEditMCATitle->text();
}

QString WizardResourceGeneratorPage::GetMCATitleVersion() const {

	return mpLineEditMCATitleVersion->text();
}

QString WizardResourceGeneratorPage::GetMCAAudioContentKind() const {

	return mpLineEditMCAAudioContentKind->text();
}

QString WizardResourceGeneratorPage::GetMCAAudioElementKind() const {

	return mpLineEditMCAAudioElementKind->text();
}
//WR

EditRate WizardResourceGeneratorPage::GetCplEditRate() const {

	return EditRate::GetEditRate(mpComboBoxCplEditRate->currentText());
}



bool WizardResourceGeneratorPage::isComplete() const {

	bool are_mandatory_fields_filled = QWizardPage::isComplete();
	if(mpStackedLayout->currentIndex() == WizardResourceGeneratorPage::ExrIndex) {
		return are_mandatory_fields_filled && GetEditRate().IsValid();
	}
	else if(mpStackedLayout->currentIndex() == WizardResourceGeneratorPage::WavIndex) {
		return are_mandatory_fields_filled && GetSoundfieldGroup().IsComplete();
	}
	else if(mpStackedLayout->currentIndex() == WizardResourceGeneratorPage::TTMLIndex) {
		return are_mandatory_fields_filled;
	}
	return false;
}

void WizardResourceGeneratorPage::SwitchMode(WizardResourceGenerator::eMode mode) {

	switch(mode) {
		case WizardResourceGenerator::ExrMode:
			mpStackedLayout->setCurrentIndex(ExrIndex);
			break;
		case WizardResourceGenerator::WavMode:
			mpStackedLayout->setCurrentIndex(WavIndex);
			mpFileDialog->setNameFilters(QStringList() << "*.wav" );
			mpFileDialog->setFileMode(QFileDialog::ExistingFiles);
			break;

				/* -----Denis Manthey----- */
		case WizardResourceGenerator::TTMLMode:
			mpStackedLayout->setCurrentIndex(TTMLIndex);
			mpFileDialog->setNameFilters(QStringList() << "*.ttml *.xml");
			mpFileDialog->setFileMode(QFileDialog::ExistingFile);
			break;
				/* -----Denis Manthey----- */

		case WizardResourceGenerator::Jpeg2000Mode:
			mpStackedLayout->setCurrentIndex(Jpeg2000Index);
			break;

		default:
			break;
	}
}

WidgetProxyImage::WidgetProxyImage(QWidget *pParent /*= NULL*/) :
QWidget(NULL), mIndex(QPersistentModelIndex()), mpSpinner(NULL), mpTimer(NULL), mpImageLabel(NULL) {

	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowTransparentForInput);
	mpTimer = new QTimer(this);
	mpTimer->setSingleShot(true);
	setGeometry(0, 0, 100, 100);
	InitLayout();
}

void WidgetProxyImage::rTimeout() {

	show();
	QRect rect = geometry();
	rect.moveTopLeft(QCursor::pos());
	setGeometry(rect);
	mpSpinner->move(QRect(QPoint(0, 0), geometry().size()).center() - mpSpinner->rect().center());
}

void WidgetProxyImage::InitLayout() {

	setObjectName("ProxyImage");
	setStyleSheet(QString(
		"QWidget#%1 {"
		"background-color: black;"
		"}").arg(objectName()));
	mpSpinner = new QtWaitingSpinner(this);
	mpSpinner->setMinimumTrailOpacity(15.0);
	mpSpinner->setTrailFadePercentage(70.0);
	mpSpinner->setNumberOfLines(15);
	mpSpinner->setLineLength(15);
	mpSpinner->setLineWidth(5);
	mpSpinner->setInnerRadius(15);
	mpSpinner->setRevolutionsPerSecond(2);
	mpSpinner->setColor(QColor(255, 255, 255));

	mpImageLabel = new QLabel(this);

	QHBoxLayout *p_layout = new QHBoxLayout();
	p_layout->setSpacing(0);
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->addWidget(mpImageLabel);
	setLayout(p_layout);
}


void WidgetProxyImage::rTransformationFinished(const QImage &rImage, const QVariant &rIdentifier) {

	if(mpImageLabel->pixmap() == NULL && rIdentifier.toModelIndex() == mIndex) {
		mpSpinner->stop();
		mpImageLabel->setPixmap(QPixmap::fromImage(rImage));
		QRect rect = geometry();
		rect.setWidth(rImage.width());
		rect.setHeight(rImage.height());
		setGeometry(rect);
	}
}

void WidgetProxyImage::hide() {

	disconnect(mpTimer, NULL, NULL, NULL);
	mpImageLabel->clear();
	repaint();
	QWidget::hide();
}


SoundFieldGroupModel::SoundFieldGroupModel(QObject *pParent /*= NULL*/) :
QAbstractTableModel(pParent), mSourceFilesChannels(QList<QPair<QString, unsigned int> >()), mSoundfieldGroup(SoundfieldGroup::SoundFieldGroupNone), mpAs02Wrapper(NULL) {

	mpAs02Wrapper = new MetadataExtractor(this);
}

void SoundFieldGroupModel::SetFilesList(const QStringList &rSourceFile) {

	beginResetModel();
	mSourceFilesChannels.clear();
	for(int i = 0; i < rSourceFile.size(); i++) {
		Metadata metadata;
		Error error = mpAs02Wrapper->ReadMetadata(metadata, rSourceFile.at(i));
		if(error.IsError() == false) {
			for(unsigned int ii = 0; ii < metadata.audioChannelCount; ii++) {
				mSourceFilesChannels.push_back(QPair<QString, unsigned int>(rSourceFile.at(i), ii)); // Add entry for every channel.
			}
		}
		else {
			qWarning() << error;
		}
	}
	endResetModel();
}

QStringList SoundFieldGroupModel::GetSourceFiles() const {

	QStringList ret;
	for(int i = 0; i < mSourceFilesChannels.size(); i++) {
		if(mSourceFilesChannels.at(i).second == 0)ret.push_back(mSourceFilesChannels.at(i).first);
	}
	return ret;
}

Qt::ItemFlags SoundFieldGroupModel::flags(const QModelIndex &rIndex) const {

	const int row = rIndex.row();
	const int column = rIndex.column();

	if(column == SoundFieldGroupModel::ColumnDstChannel) {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int SoundFieldGroupModel::rowCount(const QModelIndex &rParent /*= QModelIndex()*/) const {

	if(rParent.isValid() == false) {
		return mSourceFilesChannels.size();
	}
	return 0;
}

int SoundFieldGroupModel::columnCount(const QModelIndex &rParent /*= QModelIndex()*/) const {

	if(rParent.isValid() == false) {
		return ColumnMax;
	}
	return 0;
}

QVariant SoundFieldGroupModel::data(const QModelIndex &rIndex, int role /*= Qt::DisplayRole*/) const {

	const int row = rIndex.row();
	const int column = rIndex.column();

	if(row < mSourceFilesChannels.size()) {
		if(column == SoundFieldGroupModel::ColumnIcon) {
			// icon
			if(role == Qt::DecorationRole) {
				return QVariant(QPixmap(":/sound_small.png"));
			}
		}
		else if(column == SoundFieldGroupModel::ColumnSourceFile) {
			if(role == Qt::DisplayRole) {
				return QVariant(QString("[%1]").arg(get_file_name(mSourceFilesChannels.at(row).first)));
			}
			else if(role == Qt::ToolTipRole) {
				Metadata metadata;
				Error error = mpAs02Wrapper->ReadMetadata(metadata, mSourceFilesChannels.at(row).first);
				if(error.IsError() == true) {
					return QVariant(error.GetErrorMsg());
				}
				return QVariant(metadata.GetAsString());
			}
		}
		else if(column == SoundFieldGroupModel::ColumnSrcChannel) {
			if(role == Qt::DisplayRole) {
				return QVariant(tr("Channel %1 maps to: ").arg(mSourceFilesChannels.at(row).second));
			}
		}
		else if(column == SoundFieldGroupModel::ColumnDstChannel) {
			if(role == UserRoleComboBox) {
				return QVariant(mSoundfieldGroup.GetAdmittedChannelNames());
			}
			else if(role == Qt::DisplayRole) {
				return QVariant(mSoundfieldGroup.GetChannelName(row));
			}
		}
	}
	return QVariant();
}

bool SoundFieldGroupModel::setData(const QModelIndex &rIndex, const QVariant &rValue, int role /*= Qt::EditRole*/) {

	const int row = rIndex.row();
	const int column = rIndex.column();

	if(row < mSourceFilesChannels.size()) {
		if(column == SoundFieldGroupModel::ColumnDstChannel) {
			if(role == Qt::EditRole) {
				bool success = mSoundfieldGroup.AddChannel(row, rValue.toString());
				if(success) {
					emit dataChanged(rIndex, rIndex);
					return true;
				}
			}
		}
	}
	return false;
}

void SoundFieldGroupModel::ChangeSoundfieldGroup(const QString &rName) {

	beginResetModel();
	mSoundfieldGroup = SoundfieldGroup::GetSoundFieldGroup(rName);
	endResetModel();
}

void SoundFieldGroupModel::SetSoundfieldGroup(const SoundfieldGroup &rSoundfieldGroup) {

	beginResetModel();
	mSoundfieldGroup = rSoundfieldGroup;
	endResetModel();
}



			/* -----Denis Manthey----- */

TimedTextModel::TimedTextModel(QObject *pParent /*= NULL*/) :
QAbstractTableModel(pParent), mpAs02Wrapper(NULL) {

	mpAs02Wrapper = new MetadataExtractor(this);
}

void TimedTextModel::SetFile(const QStringList &rSourceFile) {

	beginResetModel();
	mSelectedFile.clear();
	mSelectedFile = rSourceFile;
	endResetModel();
}
int TimedTextModel::rowCount(const QModelIndex &rParent /*= QModelIndex()*/) const {

	if(rParent.isValid() == false) {
		return mSelectedFile.size();
	}
	return 0;
}
int TimedTextModel::columnCount(const QModelIndex &rParent /*= QModelIndex()*/) const {

	if(rParent.isValid() == false) {
		return ColumnMax;
	}
	return 0;
}
QVariant TimedTextModel::data(const QModelIndex &rIndex, int role /*= Qt::DisplayRole*/) const {

	const int row = rIndex.row();
	const int column = rIndex.column();
	if(row < mSelectedFile.size()) {
		if(column == TimedTextModel::ColumnIcon) {
			if(role == Qt::DecorationRole) {
				return QVariant(QPixmap(":/text_small.png"));
			}
		}
		else if(column == TimedTextModel::ColumnFilePath) {
			if(role == Qt::DisplayRole) {
				return QVariant(mSelectedFile.at(row));
			}
			else if(role == Qt::ToolTipRole) {
				Metadata metadata;
				Error error = mpAs02Wrapper->ReadMetadata(metadata, mSelectedFile.at(row));
				if(error.IsError() == true) {
					return QVariant(error.GetErrorMsg());
				}
				return QVariant(metadata.GetAsString());
			}
		}
	}
	return QVariant();
}
			/* -----Denis Manthey----- */
