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
#pragma once
#include <QWizard>


class QLineEdit;
class QDialogButtonBox;
class QFileDialog;

class WizardPartialImpGenerator : public QWizard {

	Q_OBJECT

public:
	WizardPartialImpGenerator(QWidget *pParent = NULL);
	virtual ~WizardPartialImpGenerator() {}
	virtual QSize sizeHint() const;

private:
	Q_DISABLE_COPY(WizardPartialImpGenerator);
};


class WizardPartialImpGeneratorPage : public QWizardPage {

	Q_OBJECT

public:
	WizardPartialImpGeneratorPage(QWidget *pParent = NULL);
	virtual ~WizardPartialImpGeneratorPage() {}

	private slots:
	void rFileSelected(const QString &rFile);

private:
	Q_DISABLE_COPY(WizardPartialImpGeneratorPage);
	void InitLayout();

	QFileDialog *mpFileDialog;
	QLineEdit *mpLineEditRootDir;
	QLineEdit *mpLineEditDirName;
	QLineEdit *mpLineEditIssuer;
	QLineEdit *mpLineEditAnnotation;
};
