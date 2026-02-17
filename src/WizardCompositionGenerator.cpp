/* Copyright(C) 2019 Björn Stresing, Denis Manthey, Wolfgang Ruppel, Krispin Weiss
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
#include "WizardCompositionGenerator.h"
#include "WizardResourceGenerator.h"
#include "QHBoxLayout"
#include <QComboBox>
#include <QLabel>
#include <QStringListModel>
#include <QLineEdit>
#include <QStandardItemModel>


WizardCompositionGenerator::WizardCompositionGenerator(QWidget *pParent /*= NULL*/, EditRate rEditRate /* = EditRate::EditRate23_98 */, QStringList rApplicationIdentificationList /*= QStringList() */) :
QWizard(pParent), mEditRate(rEditRate), mApplicationIdentificationList(rApplicationIdentificationList) {

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setWindowModality(Qt::WindowModal);
	setWindowTitle(tr("Composition Generator"));
	setWizardStyle(QWizard::ModernStyle);
	setStyleSheet("QWizard QPushButton {min-width: 60 px;}");
	InitLayout();
}


QSize WizardCompositionGenerator::sizeHint() const {

	return QSize(600, 300);
}


void WizardCompositionGenerator::InitLayout() {

	WizardCompositionGeneratorPage *p_wizard_page = new WizardCompositionGeneratorPage(this);
	p_wizard_page->SetEditRate(mEditRate);
	mPageId = addPage(p_wizard_page);
	QList<QWizard::WizardButton> layout;
	layout << QWizard::Stretch << QWizard::FinishButton << QWizard::CancelButton;
	setButtonLayout(layout);
	if (!mApplicationIdentificationList.isEmpty()) p_wizard_page->SetApp(mApplicationIdentificationList.first());
}

WizardCompositionGeneratorPage::WizardCompositionGeneratorPage(QWidget *pParent /*= NULL*/) :
QWizardPage(pParent), mpComboBoxEditRate(NULL) {

	setTitle(tr("Add new Composition Playlist"));
	setSubTitle(tr("Fill in details. The edit rate can not be changed later. Everything else can."));
	InitLayout();
}

void WizardCompositionGeneratorPage::InitLayout() {

	mpComboBoxEditRate = new QComboBox(this);
	mpComboBoxEditRate->setWhatsThis(tr("The edit rate can not be changed later."));
	QStringListModel *p_edit_rates_model = new QStringListModel(this);
	p_edit_rates_model->setStringList(EditRate::GetFrameRateNames());
	mpComboBoxEditRate->setModel(p_edit_rates_model);

	QLineEdit *p_content_title = new QLineEdit(this);
	p_content_title->setPlaceholderText(tr("--The title for the composition--"));
	QLineEdit *p_issuer = new QLineEdit(this);
	p_issuer->setPlaceholderText(tr("--The entity that created the Composition Playlist--"));
	QLineEdit *p_content_originator = new QLineEdit(this);
	p_content_originator->setPlaceholderText(tr("--The originator of the content underlying the composition--"));

	QGridLayout *p_layout = new QGridLayout();
	p_layout->addWidget(new QLabel(tr("Select the edit rate (mandatory):"), this), 0, 0, 1, 1);
	p_layout->addWidget(mpComboBoxEditRate, 0, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Title (mandatory):"), this), 1, 0, 1, 1);
	p_layout->addWidget(p_content_title, 1, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Issuer (optional):"), this), 2, 0, 1, 1);
	p_layout->addWidget(p_issuer, 2, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Content Originator (optional):"), this), 3, 0, 1, 1);
	p_layout->addWidget(p_content_originator, 3, 1, 1, 1);

	mpComboBoxApp = new QComboBox(this);
	mAppString = "-- Select Application --";
	mpComboBoxApp->addItem(mAppString);
	for (QMap<QString, QString>::const_iterator i = mApplicationIdentificationMap.cbegin(); i != mApplicationIdentificationMap.cend(); i++) {
		mpComboBoxApp->addItem(i.key());
	}
	// From https://stackoverflow.com/questions/7632645/how-to-set-non-selectable-default-text-on-qcombobox/7633081#7633081
	QStandardItemModel* model = qobject_cast<QStandardItemModel*>(mpComboBoxApp->model());
	QModelIndex firstIndex = model->index(0, mpComboBoxApp->modelColumn(), mpComboBoxApp->rootModelIndex());
	QStandardItem* firstItem = model->itemFromIndex(firstIndex);
	firstItem->setSelectable(false);
	mpComboBoxApp->setEditable(true);
	connect(mpComboBoxApp, SIGNAL(currentTextChanged(const QString)), this, SLOT(AppTextChanged(const QString)));

	p_layout->addWidget(new QLabel(tr("Application (mandatory):"), this), 4, 0, 1, 1);
	p_layout->addWidget(mpComboBoxApp, 4, 1, 1, 1);
	setLayout(p_layout);

	registerField(FIELD_NAME_EDIT_RATE, this, "EditRateSelected", SIGNAL(EditRateChanged()));
	registerField(FIELD_NAME_TITLE"*", p_content_title);
	registerField(FIELD_NAME_ISSUER, p_issuer);
	registerField(FIELD_NAME_CONTENT_ORIGINATOR, p_content_originator);
	registerField(FIELD_NAME_APP, this, "AppSelected", SIGNAL(AppChanged()));
}

void WizardCompositionGeneratorPage::SetEditRate(const EditRate &rEditRate) {

	mpComboBoxEditRate->setCurrentText(rEditRate.GetName());
	emit EditRateChanged();
}

EditRate WizardCompositionGeneratorPage::GetEditRate() const {

	return EditRate::GetEditRate(mpComboBoxEditRate->currentText());
}

QString WizardCompositionGeneratorPage::GetApp() const {

	return mpComboBoxApp->currentText();
}

void WizardCompositionGeneratorPage::SetApp(const QString &rApplicationIdentification) {

	mpComboBoxApp->setCurrentText(rApplicationIdentification);
	mAppString = rApplicationIdentification;
	emit AppChanged();
}

void WizardCompositionGeneratorPage::AppTextChanged(const QString rAppString) {
	mAppString = rAppString;
	emit completeChanged();
}

bool WizardCompositionGeneratorPage::isComplete() const {
	bool complete = QWizardPage::isComplete();
	if(mAppString.contains("-- Select Application --")) {
		return false;
	} else {
		return complete;
	}
}

