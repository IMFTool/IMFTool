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
#include "ImfCommon.h"
#include <QWizard>
#include <QWizardPage>


class QComboBox;

class WizardCompositionGenerator : public QWizard {

	Q_OBJECT

public:
	WizardCompositionGenerator(QWidget *pParent = NULL);
	virtual ~WizardCompositionGenerator() {}
	virtual QSize sizeHint() const;

private:
	Q_DISABLE_COPY(WizardCompositionGenerator);
	void	InitLayout();
	int		mPageId;
};


class WizardCompositionGeneratorPage : public QWizardPage {

	Q_OBJECT
		Q_PROPERTY(EditRate EditRateSelected READ GetEditRate WRITE SetEditRate NOTIFY EditRateChanged)

public:
	WizardCompositionGeneratorPage(QWidget *pParent = NULL);
	virtual ~WizardCompositionGeneratorPage() {}
	EditRate GetEditRate() const;
	void SetEditRate(const EditRate &rEditRate);

signals:
	void EditRateChanged();

private:
	Q_DISABLE_COPY(WizardCompositionGeneratorPage);
	void InitLayout();

	QComboBox *mpComboBoxEditRate;
};
