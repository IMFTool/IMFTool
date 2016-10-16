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
#include "WizardPartialImpGenerator.h"
#include "global.h"
#include <QTextBrowser>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QWizardPage>
#include <QFileDialog>
#include <QCompleter>
#include <QDirModel>
#include <QRegExpValidator>

WizardPartialImpGenerator::WizardPartialImpGenerator(QWidget *pParent /*= NULL*/) :
QWizard(pParent) {

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setFixedSize(sizeHint());
	setWindowModality(Qt::WindowModal);
	setWindowTitle(tr("Workspace Launcher"));
	setWizardStyle(QWizard::ModernStyle);
	setStyleSheet("QWizard QPushButton {min-width: 60 px;}");
	setOption(QWizard::NoBackButtonOnLastPage);
	QList<QWizard::WizardButton> layout;
	layout << QWizard::Stretch << QWizard::CancelButton << QWizard::FinishButton;
	setButtonLayout(layout);
}

QSize WizardPartialImpGenerator::sizeHint() const {

	return QSize(600, 350);
}

WizardPartialImpGeneratorPage::WizardPartialImpGeneratorPage(QWidget *pParent /*= NULL*/) :
QWizardPage(pParent), mpFileDialog(NULL), mpLineEditRootDir(NULL), mpLineEditDirName(NULL), mpLineEditIssuer(NULL), mpLineEditAnnotation(NULL) {

	setTitle(tr("Write Partial IMF Package"));
	setSubTitle(tr("Please enter a name and select the root directory of the Partial IMF package. Every asset added in the current project will be written to this Folder"));
	InitLayout();
}

void WizardPartialImpGeneratorPage::InitLayout() {

	mpFileDialog = new QFileDialog(this, QString(), QDir::homePath());
	mpFileDialog->setFileMode(QFileDialog::Directory);
	mpFileDialog->setViewMode(QFileDialog::Detail);
	mpFileDialog->setOption(QFileDialog::ShowDirsOnly);
	mpLineEditRootDir = new QLineEdit(this);
	mpLineEditRootDir->setWhatsThis("You can select the root dir using the directory browser or enter the absolute path directly.");
	mpLineEditRootDir->setPlaceholderText(tr("--Select Partial IMF package root directory--"));
	QCompleter *p_completer = new QCompleter(this);
	p_completer->setModel(new QDirModel(QStringList(), QDir::AllDirs | QDir::NoDotAndDotDot, QDir::NoSort, p_completer));
	p_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	mpLineEditRootDir->setCompleter(p_completer);
	QPushButton *p_button_browse = new QPushButton(tr("Browse"), this);
	p_button_browse->setAutoDefault(false);
	mpLineEditDirName = new QLineEdit(this);
	mpLineEditDirName->setWhatsThis("Enter the name of the partial IMP you want to create.");
	mpLineEditDirName->setPlaceholderText(tr("--Enter the name of the partial IMP--"));
	QRegExp rx("[A-Za-z0-9-_]+");
	QRegExpValidator *v = new QRegExpValidator(rx, this);
	mpLineEditDirName->setValidator(v);
	mpLineEditIssuer = new QLineEdit(this);
	mpLineEditIssuer->setPlaceholderText(tr("--Issuer--"));
	mpLineEditAnnotation = new QLineEdit(this);
	mpLineEditAnnotation->setPlaceholderText(tr("--optional--"));
	QGridLayout *p_layout = new QGridLayout();
	p_layout->addWidget(new QLabel(tr("Name:"), this), 0, 0, 1, 1);
	p_layout->addWidget(mpLineEditDirName, 0, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Directory:"), this), 1, 0, 1, 1);
	p_layout->addWidget(mpLineEditRootDir, 1, 1, 1, 1);
	p_layout->addWidget(p_button_browse, 1, 2, 1, 1);
	p_layout->addWidget(new QLabel(tr("Issuer:"), this), 2, 0, 1, 1);
	p_layout->addWidget(mpLineEditIssuer, 2, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Annotation Text:"), this), 3, 0, 1, 1);
	p_layout->addWidget(mpLineEditAnnotation, 3, 1, 1, 1);
	setLayout(p_layout);

	registerField(FIELD_NAME_PARTIAL_DIR"*", mpLineEditRootDir);
	registerField(FIELD_NAME_PARTIAL_NAME"*", mpLineEditDirName);
	registerField(FIELD_NAME_PARTIAL_ISSUER"*", mpLineEditIssuer);
	registerField(FIELD_NAME_PARTIAL_ANNOTATION"", mpLineEditAnnotation);

	connect(p_button_browse, SIGNAL(clicked()), mpFileDialog, SLOT(show()));
	connect(mpFileDialog, SIGNAL(fileSelected(const QString &)), this, SLOT(rFileSelected(const QString &)));
}

void WizardPartialImpGeneratorPage::rFileSelected(const QString &rFile) {

	mpLineEditRootDir->setText(rFile);
}
