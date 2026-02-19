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
#pragma once
#include "ImfCommon.h"
#include <QWizard>
#include <QWizardPage>


class QComboBox;

class WizardCompositionGenerator : public QWizard {

	Q_OBJECT

public:
	WizardCompositionGenerator(QWidget *pParent = NULL, EditRate rEditRate = EditRate::EditRate23_98, QStringList rApplicationIdentificationList = QStringList());
	virtual ~WizardCompositionGenerator() {}
	virtual QSize sizeHint() const;

private:
	Q_DISABLE_COPY(WizardCompositionGenerator);
	void	InitLayout();
	int		mPageId;
	EditRate mEditRate;
	QStringList mApplicationIdentificationList;
};


class WizardCompositionGeneratorPage : public QWizardPage {

	Q_OBJECT
		Q_PROPERTY(EditRate EditRateSelected READ GetEditRate WRITE SetEditRate NOTIFY EditRateChanged)
		Q_PROPERTY(QString AppSelected READ GetApp WRITE SetApp NOTIFY AppChanged)

public:
	WizardCompositionGeneratorPage(QWidget *pParent = NULL);
	virtual ~WizardCompositionGeneratorPage() {}
	EditRate GetEditRate() const;
	void SetEditRate(const EditRate &rEditRate);
	QString GetApp() const;
	void SetApp(const QString &rApplicationIdentification);

public slots:
	virtual bool isComplete() const;

signals:
	void EditRateChanged();
	void AppChanged();

private:
	Q_DISABLE_COPY(WizardCompositionGeneratorPage);
	void InitLayout();

	QComboBox *mpComboBoxEditRate;
	QComboBox *mpComboBoxApp;
	const QMap<QString, QString> mApplicationIdentificationMap {
//			{"http://www.smpte-ra.org/schemas/2067-20/2016", "App #2"},
		//Enable once published:
//			{"http://www.smpte-ra.org/ns/2067-21/5ED", "App 2E"},
			{"http://www.smpte-ra.org/ns/2067-21/2021", "App #2E"},
			{"http://www.smpte-ra.org/ns/2067-21/2020", "App #2E"},
//			{"http://www.smpte-ra.org/schemas/2067-20/2013", "App #2 2013"},
//			{"http://www.smpte-ra.org/schemas/2067-21/2014", "App #2E 2014"},
			{"http://www.smpte-ra.org/ns/2067-50/2017", "App 5 ACES"},
			{"http://www.smpte-ra.org/ns/2067-40-DCDM/2020", "App 4 DCDM 2020"},
	};
	QString mAppString;
private slots:
	void AppTextChanged(const QString);
};
