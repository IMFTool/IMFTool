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
#include "WidgetSettings.h"
#include "ImfCommon.h"
#include <global.h>
#include <QGridLayout>
#include <QTableWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QSettings>
#include <QAudioDeviceInfo>
#include <QLabel>
#include <QListWidget>
#include <QStackedLayout>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QHeaderView>


#define CHANNELS_LIST_PROPERTY "ChannelsList"

WidgetSettings::WidgetSettings(QWidget *pParent /*= NULL*/) :
QWidget(pParent), mpTabWidget(NULL), mpButtonGroup(NULL), mpAppRestartRequest(NULL) {

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setWindowModality(Qt::WindowModal);
	setWindowFlags(Qt::Dialog);
	setWindowTitle(tr("Settings"));
	InitLayout();
}

void WidgetSettings::InitLayout() {

	mpButtonGroup = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
	mpTabWidget = new QTabWidget(this);

	mpAppRestartRequest = new QLabel(tr("Changes require exiting and restarting in order for the changes to take effect!"), this);
	mpAppRestartRequest->setStyleSheet("QLabel { color: red; }");
	mpAppRestartRequest->hide();

	QGridLayout *p_layout = new QGridLayout();
	p_layout->addWidget(mpTabWidget, 0, 0, 1, 2);
	p_layout->addWidget(mpAppRestartRequest, 1, 0, 1, 1);
	p_layout->addWidget(mpButtonGroup, 1, 1, 1, 1);
	setLayout(p_layout);

	connect(mpButtonGroup, SIGNAL(accepted()), this, SLOT(rAccepted()));
	connect(mpButtonGroup, SIGNAL(rejected()), this, SLOT(rRejected()));
}

void WidgetSettings::AddSettingsPage(AbstractWidgetSettingsPage *pSettingsPage) {

	if(pSettingsPage) {
		mpTabWidget->addTab(pSettingsPage, pSettingsPage->Text());
		connect(pSettingsPage, SIGNAL(NeedsRestart()), this, SLOT(rNeedsRestart()));
		pSettingsPage->Read();
	}
}

void WidgetSettings::rAccepted() {

	for(int i = 0; i < mpTabWidget->count(); i++) {
		AbstractWidgetSettingsPage *p_settings_page = qobject_cast<AbstractWidgetSettingsPage*>(mpTabWidget->widget(i));
		if(p_settings_page) p_settings_page->Write();
	}
	emit SettingsSaved();
	close();
}

void WidgetSettings::rRejected() {

	close();
}

QSize WidgetSettings::sizeHint() const {

	return QSize(700, 500);
}

void WidgetSettings::rNeedsRestart() {

	mpAppRestartRequest->show();
}

WidgetAudioSettingsPage::WidgetAudioSettingsPage(QWidget *pParent /*= NULL*/) :
AbstractWidgetSettingsPage(pParent) {

	InitLayout();
}

void WidgetAudioSettingsPage::InitLayout() {

	mpAudioDeviceSpinBox = new QComboBox(this);
	mpAudioChannelConfigurationSpinBox = new QComboBox(this);
	mpGroupBox = new QGroupBox(tr("Channel Mapping"), this);
	mpChannelsList0 = new QListWidget(this);
	mpChannelsList0->setProperty(CHANNELS_LIST_PROPERTY, QVariant(0));
	mpChannelsList1 = new QListWidget(this);
	mpChannelsList1->setProperty(CHANNELS_LIST_PROPERTY, QVariant(1));
	mpChannelsList2 = new QListWidget(this);
	mpChannelsList2->setProperty(CHANNELS_LIST_PROPERTY, QVariant(2));
	mpChannelsList3 = new QListWidget(this);
	mpChannelsList3->setProperty(CHANNELS_LIST_PROPERTY, QVariant(3));
	mpChannelsList4 = new QListWidget(this);
	mpChannelsList4->setProperty(CHANNELS_LIST_PROPERTY, QVariant(4));
	mpChannelsList5 = new QListWidget(this);
	mpChannelsList5->setProperty(CHANNELS_LIST_PROPERTY, QVariant(5));
	mpChannelsList6 = new QListWidget(this);
	mpChannelsList6->setProperty(CHANNELS_LIST_PROPERTY, QVariant(6));
	mpChannelsList7 = new QListWidget(this);
	mpChannelsList7->setProperty(CHANNELS_LIST_PROPERTY, QVariant(7));

	mpAudioTargetChannel = new QComboBox(this);

	mpStackedLayout = new QStackedLayout();
	mpStackedLayout->addWidget(mpChannelsList0);
	mpStackedLayout->addWidget(mpChannelsList1);
	mpStackedLayout->addWidget(mpChannelsList2);
	mpStackedLayout->addWidget(mpChannelsList3);
	mpStackedLayout->addWidget(mpChannelsList4);
	mpStackedLayout->addWidget(mpChannelsList5);
	mpStackedLayout->addWidget(mpChannelsList6);
	mpStackedLayout->addWidget(mpChannelsList7);

	QGridLayout *p_group_box_layout = new QGridLayout();
	p_group_box_layout->addWidget(new QLabel(tr("Target Channel:")), 0, 0, 1, 1);
	p_group_box_layout->addWidget(mpAudioTargetChannel, 0, 1, 1, 1);
	p_group_box_layout->addLayout(mpStackedLayout, 1, 1, 1, 1);
	mpGroupBox->setLayout(p_group_box_layout);

	QGridLayout *p_layout = new QGridLayout();
	p_layout->addWidget(new QLabel(tr("Audio Device:"), this), 0, 0, 1, 1);
	p_layout->addWidget(mpAudioDeviceSpinBox, 0, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Supported Channels:"), this), 0, 2, 1, 1);
	p_layout->addWidget(mpAudioChannelConfigurationSpinBox, 0, 3, 1, 1);
	p_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 4, 1, 1);
	p_layout->addWidget(mpGroupBox, 1, 0, 1, 5);
	setLayout(p_layout);

	connect(mpAudioDeviceSpinBox, SIGNAL(currentIndexChanged(int)), this, SLOT(rCurrentIndexChanged(int)));
	connect(mpAudioChannelConfigurationSpinBox, SIGNAL(currentIndexChanged(int)), this, SLOT(rCurrentIndexChanged(int)));
	connect(mpAudioTargetChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(rCurrentIndexChanged(int)));
}

void WidgetAudioSettingsPage::Read() {

	QSettings settings;
	FillAudioDeviceComboBox();
	mpAudioDeviceSpinBox->setCurrentIndex(mpAudioDeviceSpinBox->findText(settings.value(SETTINGS_AUDIO_DEVICE, QVariant(QAudioDeviceInfo::defaultOutputDevice().deviceName())).toString()));
	FillAudioChannelConfigurationComboBox();
	mpAudioChannelConfigurationSpinBox->setCurrentIndex(mpAudioChannelConfigurationSpinBox->findData(settings.value(SETTINGS_AUDIO_CHANNEL_CONFIGURATION, 2).toInt()));
	ReadChannelsTable(mpChannelsList0);
	ReadChannelsTable(mpChannelsList1);
	ReadChannelsTable(mpChannelsList2);
	ReadChannelsTable(mpChannelsList3);
	ReadChannelsTable(mpChannelsList4);
	ReadChannelsTable(mpChannelsList5);
	ReadChannelsTable(mpChannelsList6);
	ReadChannelsTable(mpChannelsList7);

}

void WidgetAudioSettingsPage::Write() {

	QSettings settings;
	settings.setValue(SETTINGS_AUDIO_DEVICE, QVariant(mpAudioDeviceSpinBox->currentText()));
	settings.setValue(SETTINGS_AUDIO_CHANNEL_CONFIGURATION, QVariant(mpAudioChannelConfigurationSpinBox->currentData().toInt()));
	WriteChannelsTable(mpChannelsList0);
	WriteChannelsTable(mpChannelsList1);
	WriteChannelsTable(mpChannelsList2);
	WriteChannelsTable(mpChannelsList3);
	WriteChannelsTable(mpChannelsList4);
	WriteChannelsTable(mpChannelsList5);
	WriteChannelsTable(mpChannelsList6);
	WriteChannelsTable(mpChannelsList7);
}

void WidgetAudioSettingsPage::ReadChannelsTable(QListWidget *pList) {

	if(pList) {
		QSettings settings;
		int list_number = pList->property(CHANNELS_LIST_PROPERTY).toInt();
		QString list_number_string(SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL);
		list_number_string.append(QString::number(list_number));
		Channels selected_channels = settings.value(list_number_string, 0x00).toUInt();
		pList->clear();
		const QList<QPair<QString, QString> > channels = SoundfieldGroup::mChannelNamesSymbolsMap;
		for(int i = 0; i < channels.size(); i++) {
			QListWidgetItem *newItem = new QListWidgetItem();
			newItem->setText(channels.at(i).first);
			if(selected_channels & (1U << i)) newItem->setCheckState(Qt::Checked);
			else newItem->setCheckState(Qt::Unchecked);
			pList->insertItem(i, newItem);
		}
	}
}

void WidgetAudioSettingsPage::rCurrentIndexChanged(int index) {

	QComboBox *p_combo_box = qobject_cast<QComboBox*>(sender());
	if(p_combo_box) {
		if(p_combo_box == mpAudioTargetChannel) {
			if(index != -1) {
				mpStackedLayout->setCurrentIndex(index);
				QListWidget *p_list_widget = qobject_cast<QListWidget*>(mpStackedLayout->widget(index));
				if(p_list_widget) p_list_widget->scrollToTop();
			}
		}
		else if(p_combo_box == mpAudioDeviceSpinBox) {
			if(index != -1) {
				FillAudioChannelConfigurationComboBox();
			}
			else {
				mpGroupBox->setDisabled(true);
			}
		}
		else if(p_combo_box == mpAudioChannelConfigurationSpinBox) {
			if(index != -1) {
				FillAudioTargetChannelComboBox();
				mpGroupBox->setEnabled(true);
			}
			else {
				mpGroupBox->setDisabled(true);
			}
		}
	}
}

void WidgetAudioSettingsPage::WriteChannelsTable(QListWidget *pList) {

	if(pList) {
		QSettings settings;
		int list_number = pList->property(CHANNELS_LIST_PROPERTY).toInt();
		QString list_number_string(SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL);
		list_number_string.append(QString::number(list_number));
		Channels selected_channels = 0x00;
		for(int i = 0; i < pList->count(); i++) {
			QListWidgetItem *p_item = pList->item(i);
			if(p_item) {
				if(p_item->checkState() == Qt::Checked) selected_channels |= (1U << i);
			}
		}
		settings.setValue(list_number_string, selected_channels);
	}
}

QString WidgetAudioSettingsPage::Text() {

	return tr("Audio");
}

void WidgetAudioSettingsPage::FillAudioChannelConfigurationComboBox() {

	if(mpAudioChannelConfigurationSpinBox) {
		mpAudioChannelConfigurationSpinBox->clear();
		QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
		QAudioDeviceInfo audio_device;
		for(int i = 0; i < devices.size(); i++) {
			if(devices.at(i).deviceName().compare(mpAudioDeviceSpinBox->currentText()) == 0) {
				audio_device = devices.at(i);
				break;
			}
		}
		if(audio_device.isNull() == false) {
			mpAudioChannelConfigurationSpinBox->setEnabled(true);
			QList<int> supported_channels = audio_device.supportedChannelCounts();
			for(int i = 0; i < supported_channels.size(); i++) {
				mpAudioChannelConfigurationSpinBox->addItem(QString::number(supported_channels.at(i)), supported_channels.at(i));
			}
		}
		else {
			mpAudioChannelConfigurationSpinBox->setDisabled(true);
		}
	}
}

void WidgetAudioSettingsPage::FillAudioTargetChannelComboBox() {

	if(mpAudioTargetChannel) {
		mpAudioTargetChannel->clear();
		int target_configuration = mpAudioChannelConfigurationSpinBox->currentData().toInt();
		if(target_configuration > 8) qWarning() << "Max 8 channels are supported.";
		for(int i = 0; i < target_configuration && i < 8; i++) {
			mpAudioTargetChannel->addItem(tr("Channel %1").arg(i + 1));
		}
	}
}

void WidgetAudioSettingsPage::FillAudioDeviceComboBox() {

	if(mpAudioDeviceSpinBox) {
		mpAudioDeviceSpinBox->clear();
		QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
		for(int i = 0; i < devices.size(); i++) {
			mpAudioDeviceSpinBox->addItem(devices.at(i).deviceName());
		}
	}
}

