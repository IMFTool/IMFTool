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
#include "WizardWorkspaceLauncher.h"
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


WizardWorkspaceLauncher::WizardWorkspaceLauncher(QWidget *pParent /*= NULL*/) :
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

QSize WizardWorkspaceLauncher::sizeHint() const {

	return QSize(600, 300);
}

WizardWorkspaceLauncherPage::WizardWorkspaceLauncherPage(QWidget *pParent /*= NULL*/) :
QWizardPage(pParent), mpFileDialog(NULL), mpLineEdit(NULL) {

	setTitle(tr("Open IMF package"));
	setSubTitle(tr("Please select the root directory of an existing IMF package. The root directory is the directory where the ASSETMAP.xml file is located."));
	InitLayout();
}

void WizardWorkspaceLauncherPage::InitLayout() {

	mpFileDialog = new QFileDialog(this, QString(), QDir::homePath());
	mpFileDialog->setFileMode(QFileDialog::Directory);
	mpFileDialog->setViewMode(QFileDialog::Detail);
	mpLineEdit = new QLineEdit(this);
	mpLineEdit->setWhatsThis("You can select the root dir using the directory browser or enter the absolute path directly.");
	mpLineEdit->setPlaceholderText(tr("--Select IMF package root directory--"));
	QCompleter *p_completer = new QCompleter(this);
	p_completer->setModel(new QDirModel(QStringList(), QDir::AllDirs | QDir::NoDotAndDotDot, QDir::NoSort, p_completer));
	p_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	mpLineEdit->setCompleter(p_completer);
	QPushButton *p_button_browse = new QPushButton(tr("Browse"), this);
	p_button_browse->setAutoDefault(false);
	QGridLayout *p_layout = new QGridLayout();
	p_layout->addWidget(new QLabel(tr("Workspace:"), this), 0, 0, 1, 1);
	p_layout->addWidget(mpLineEdit, 0, 1, 1, 1);
	p_layout->addWidget(p_button_browse, 0, 2, 1, 1);
	setLayout(p_layout);

	registerField(FIELD_NAME_WORKING_DIR"*", mpLineEdit);

	connect(p_button_browse, SIGNAL(clicked()), mpFileDialog, SLOT(show()));
	connect(mpFileDialog, SIGNAL(fileSelected(const QString &)), this, SLOT(rFileSelected(const QString &)));
}

void WizardWorkspaceLauncherPage::rFileSelected(const QString &rFile) {

	mpLineEdit->setText(rFile);
}

WizardWorkspaceLauncherNewImpPage::WizardWorkspaceLauncherNewImpPage(QWidget *pParent /*= NULL*/) :
	QWizardPage(pParent), mpFileDialog(NULL), mpLineEdit(NULL) {

	setTitle(tr("Create new IMF package"));
	setSubTitle(tr("Please select the root directory. The root directory is the directory where the ASSETMAP.xml and all MXF Assets will be saved."));
	InitLayout();
}

void WizardWorkspaceLauncherNewImpPage::InitLayout() {

	mpFileDialog = new QFileDialog(this, QString(), QDir::homePath());
	mpFileDialog->setFileMode(QFileDialog::Directory);
	mpFileDialog->setViewMode(QFileDialog::Detail);
	mpLineEdit = new QLineEdit(this);
	mpLineEdit->setWhatsThis("You can select the root dir using the directory browser or enter the absolute path directly.");
	mpLineEdit->setPlaceholderText(tr("--Select IMF package root directory--"));
	QCompleter *p_completer = new QCompleter(this);
	p_completer->setModel(new QDirModel(QStringList(), QDir::AllDirs | QDir::NoDotAndDotDot, QDir::NoSort, p_completer));
	p_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	mpLineEdit->setCompleter(p_completer);
	QPushButton *p_button_browse = new QPushButton(tr("Browse"), this);
	p_button_browse->setAutoDefault(false);
	QLineEdit *p_issuer = new QLineEdit(this);
	p_issuer->setPlaceholderText(tr("--The entity that created the Composition Playlist--"));
	QLineEdit *p_annotation_text = new QLineEdit(this);
	p_annotation_text->setPlaceholderText(tr("--Annotation describing the composition--"));

	QGridLayout *p_layout = new QGridLayout();
	p_layout->addWidget(new QLabel(tr("Workspace:"), this), 0, 0, 1, 1);
	p_layout->addWidget(mpLineEdit, 0, 1, 1, 1);
	p_layout->addWidget(p_button_browse, 0, 2, 1, 1);
	p_layout->addWidget(new QLabel(tr("Issuer (mandatory):"), this), 1, 0, 1, 1);
	p_layout->addWidget(p_issuer, 1, 1, 1, 2);
	p_layout->addWidget(new QLabel(tr("Annotation (optional):"), this), 2, 0, 1, 1);
	p_layout->addWidget(p_annotation_text, 2, 1, 1, 2);

	setLayout(p_layout);

	registerField(FIELD_NAME_WORKING_DIR"*", mpLineEdit);
	registerField(FIELD_NAME_ISSUER"*", p_issuer);
	registerField(FIELD_NAME_ANNOTATION"", p_annotation_text);

	connect(p_button_browse, SIGNAL(clicked()), mpFileDialog, SLOT(show()));
	connect(mpFileDialog, SIGNAL(fileSelected(const QString &)), this, SLOT(rFileSelected(const QString &)));
}

void WizardWorkspaceLauncherNewImpPage::rFileSelected(const QString &rFile) {

	mpLineEdit->setText(rFile);
}
