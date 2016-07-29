/*  is a GUI application for interactive IMF Master Package creation.
 * Copyright(C) 2016 Björn Stresing
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
#include <QWidget>


class QTabWidget;
class QGroupBox;
class QComboBox;
class QListWidget;
class QStackedLayout;
class QDialogButtonBox;
class QTableWidget;
class QLabel;

class AbstractWidgetSettingsPage : public QWidget {

	Q_OBJECT

public:
	AbstractWidgetSettingsPage(QWidget *pParent = NULL) : QWidget(pParent) {}
	virtual ~AbstractWidgetSettingsPage() {}

signals:
	void NeedsRestart();

public:
	virtual void Read() = 0;
	virtual void Write() = 0;
	virtual QString Text() = 0;
};

class WidgetAudioSettingsPage : public AbstractWidgetSettingsPage {

	Q_OBJECT

public:
	WidgetAudioSettingsPage(QWidget *pParent = NULL);
	virtual ~WidgetAudioSettingsPage() {}
	virtual void Read();
	virtual void Write();
	virtual QString Text();

	private slots:
	void rCurrentIndexChanged(int index);

private:
	Q_DISABLE_COPY(WidgetAudioSettingsPage);
	void InitLayout();
	void ReadChannelsTable(QListWidget *pList);
	void WriteChannelsTable(QListWidget *pList);

	void FillAudioDeviceComboBox();
	//! Fills Combobox dependent on mpAudioDeviceSpinBox selection.
	void FillAudioChannelConfigurationComboBox();
	//! Fills Combobox dependent on mpAudioChannelConfigurationSpinBox selection.
	void FillAudioTargetChannelComboBox();

	QTabWidget *mpTabWidget;
	QGroupBox *mpGroupBox;
	QComboBox *mpAudioDeviceSpinBox;
	QComboBox *mpAudioChannelConfigurationSpinBox;
	QComboBox *mpAudioTargetChannel;
	QStackedLayout *mpStackedLayout;
	QListWidget *mpChannelsList0;
	QListWidget *mpChannelsList1;
	QListWidget *mpChannelsList2;
	QListWidget *mpChannelsList3;
	QListWidget *mpChannelsList4;
	QListWidget *mpChannelsList5;
	QListWidget *mpChannelsList6;
	QListWidget *mpChannelsList7;
};


class WidgetSettings : public QWidget {

	Q_OBJECT

public:
	WidgetSettings(QWidget *pParent = NULL);
	virtual ~WidgetSettings() {}
	//! Takes ownership.
	void AddSettingsPage(AbstractWidgetSettingsPage *pSettingsPage);
	virtual QSize sizeHint() const;

signals:
	void SettingsSaved();

private slots:
void rAccepted();
void rRejected();
void rNeedsRestart();

private:
	Q_DISABLE_COPY(WidgetSettings);
	void InitLayout();

	QTabWidget *mpTabWidget;
	QDialogButtonBox *mpButtonGroup;
	QLabel *mpAppRestartRequest;
};
