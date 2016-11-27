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
#pragma once
#include <QWizard>


class QLineEdit;
class QDialogButtonBox;
class QFileDialog;

class WizardWorkspaceLauncher : public QWizard {

	Q_OBJECT

public:
	WizardWorkspaceLauncher(QWidget *pParent = NULL);
	virtual ~WizardWorkspaceLauncher() {}
	virtual QSize sizeHint() const;

private:
	Q_DISABLE_COPY(WizardWorkspaceLauncher);
};


class WizardWorkspaceLauncherPage : public QWizardPage {

	Q_OBJECT

public:
	WizardWorkspaceLauncherPage(QWidget *pParent = NULL);
	virtual ~WizardWorkspaceLauncherPage() {}

	private slots:
	void rFileSelected(const QString &rFile);

private:
	Q_DISABLE_COPY(WizardWorkspaceLauncherPage);
	void InitLayout();

	QFileDialog *mpFileDialog;
	QLineEdit		*mpLineEdit;
};


class WizardWorkspaceLauncherNewImpPage : public QWizardPage {

	Q_OBJECT

public:
	WizardWorkspaceLauncherNewImpPage(QWidget *pParent = NULL);
	virtual ~WizardWorkspaceLauncherNewImpPage() {}

	private slots:
	void rFileSelected(const QString &rFile);

private:
	Q_DISABLE_COPY(WizardWorkspaceLauncherNewImpPage);
	void InitLayout();

	QFileDialog *mpFileDialog;
	QLineEdit		*mpLineEdit;
};
