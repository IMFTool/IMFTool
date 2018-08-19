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
#include "info.h"
#include <QObject>
#include <QDebug>
#include <QString>
#include <QUuid>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <QMainWindow>
#include <QFileIconProvider>
#include <QMutex>
#include <QMutexLocker>


#define DEBUG_FILE_NAME PROJECT_NAME".log"
#define MAX_DEBUG_FILE_SIZE 1000 // [Byte]
#define ASSET_SEARCH_NAME "ASSETMAP.xml" // never ever change this
#define VOLINDEX_SEARCH_NAME "VOLINDEX.xml" // never ever change this
#define CREATOR_STRING PROJECT_NAME " " VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH
#define MIME_TYPE_MXF "application/mxf"
#define MIME_TYPE_XML "text/xml"
#define WELL_KNOWN_MARKER_LABEL_SCOPE_2013 "http://www.smpte-ra.org/schemas/2067-3/2013#standard-markers"
#define WELL_KNOWN_MARKER_LABEL_SCOPE_2016 "http://www.smpte-ra.org/schemas/2067-3/2016#standard-markers"
#define CUSTOM_MARKER_LABEL_SCOPE "http://www.hsrm.de/schemas/2015#markers"
#define IMFTOOL

#define XML_NAMESPACE_CPL "http://www.smpte-ra.org/schemas/2067-3/2016"
#define XML_NAMESPACE_AM "http://www.smpte-ra.org/schemas/429-9/2007/AM"
#define XML_NAMESPACE_PKL "http://www.smpte-ra.org/schemas/2067-2/2016/PKL"
#define XML_NAMESPACE_DCML "http://www.smpte-ra.org/schemas/433/2008/dcmlTypes/"
#define XML_NAMESPACE_CC "http://www.smpte-ra.org/schemas/2067-2/2016"
#define XML_NAMESPACE_DS "http://www.w3.org/2000/09/xmldsig#"
#define XML_NAMESPACE_XS "http://www.w3.org/2001/XMLSchema"
#define XML_NAMESPACE_NS "http://www.w3.org/2000/xmlns/"

#define SETTINGS_AUDIO_DEVICE "audio/audioDevice"
#define SETTINGS_AUDIO_CHANNEL_CONFIGURATION "audio/audioChannelConfiguration"
#define SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL "audio/audioChannels" // Without number for runtime string creation.
#define SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL0 "audio/audioChannels0"
#define SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL1 "audio/audioChannels1"
#define SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL2 "audio/audioChannels2"
#define SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL3 "audio/audioChannels3"
#define SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL4 "audio/audioChannels4"
#define SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL5 "audio/audioChannels5"
#define SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL6 "audio/audioChannels6"
#define SETTINGS_AUDIO_SELECTED_CHANNELS_FOR_TARGET_CHANNEL7 "audio/audioChannels7"
#define SETTINGS_CL_PLATFORM "cl/Platform"
#define SETTINGS_CL_DEVICE "cl/Device"

#define FIELD_NAME_ISSUER "Issuer"
#define FIELD_NAME_ANNOTATION "Annotation"
#define FIELD_NAME_TITLE "Title"
#define FIELD_NAME_CONTENT_ORIGINATOR "ContentOriginator"
#define FIELD_NAME_SELECTED_FILES "SelectedFiles"
#define FIELD_NAME_RESOURCE_NAME "ResourceName"
#define FIELD_NAME_EDIT_RATE "EditRate"
#define FIELD_NAME_DURATION "Duration"
#define FIELD_NAME_SOUNDFIELD_GROUP "SoundfiledGourpName"
#define FIELD_NAME_WORKING_DIR "WorkingDir"


			/* -----Denis Manthey Beg----- */
#define FIELD_NAME_PARTIAL_DIR "PartialDir"
#define FIELD_NAME_PARTIAL_NAME "PartialName"
#define FIELD_NAME_PARTIAL_ISSUER "PartialIssuer"
#define FIELD_NAME_PARTIAL_ANNOTATION "PartialAnnotation"
			/* -----Denis Manthey End----- */
//WR
#define FIELD_NAME_LANGUAGETAG_WAV "LanguageTagWav"
#define FIELD_NAME_LANGUAGETAG_TT "LanguageTagTT"
#define FIELD_NAME_MCA_TITLE "MCATitle"
#define FIELD_NAME_MCA_TITLE_VERSION "MCATitleVersion"
#define FIELD_NAME_MCA_AUDIO_CONTENT_KIND "MCAAudioContentKind"
#define FIELD_NAME_MCA_AUDIO_ELEMENT_KIND "MCAAudioElementKind"
#define FIELD_NAME_CPL_EDIT_RATE "CplEditRate"

#define FIELD_NAME_SCM_CPL_LIST "ScmCplList"

#define IMSC1_NS_TT 		XMLString::transcode("http://www.w3.org/ns/ttml")
#define IMSC1_NS_TTM 		XMLString::transcode("http://www.w3.org/ns/ttml#metadata")
#define IMSC1_NS_TTP 		XMLString::transcode("http://www.w3.org/ns/ttml#parameter")
#define IMSC1_NS_TTS 		XMLString::transcode("http://www.w3.org/ns/ttml#styling")
#define IMSC1_NS_SMPTE 		XMLString::transcode("http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt")
#define XML_NAMESPACE 		XMLString::transcode("http://www.w3.org/XML/1998/namespace")

#define XML_NAMESPACE_SCM "http://www.smpte-ra.org/ns/2067-9/2018"
//WR

enum eUserEventType {

	UserEventCplEditRateChange = QEvent::User + 1,
};


enum eUserItemDataRole {

	UserRoleComboBox = Qt::UserRole + 1,
	UserRoleMetadata
};


class Id {

public:
	static Id& Instance() {
		static Id _instance;
		return _instance;
	}

	int value() {
		QMutexLocker locker(&mutex);
		int id = counter++;
		return id;
	}

private:
	Id() : mutex(), counter(0) {}
	Q_DISABLE_COPY(Id);
	QMutex mutex;
	int counter;
};


inline QString strip_uuid(const QUuid &rUuid) {

	QString uuid = rUuid.toString();
	return uuid.mid(1, uuid.size() - 2);
}

inline QUuid convert_uuid(const unsigned char *pByteArray) {

	uint   l(uint(0) | pByteArray[0] << 24 | pByteArray[1] << 16 | pByteArray[2] << 8 | pByteArray[3] << 0);
	ushort w1(ushort(0) | pByteArray[4] << 8 | pByteArray[5] << 0);
	ushort w2(ushort(0) | pByteArray[6] << 8 | pByteArray[7] << 0);
	uchar  b1(uchar(0) | pByteArray[8] << 0);
	uchar  b2(uchar(0) | pByteArray[9] << 0);
	uchar  b3(uchar(0) | pByteArray[10] << 0);
	uchar  b4(uchar(0) | pByteArray[11] << 0);
	uchar  b5(uchar(0) | pByteArray[12] << 0);
	uchar  b6(uchar(0) | pByteArray[13] << 0);
	uchar  b7(uchar(0) | pByteArray[14] << 0);
	uchar  b8(uchar(0) | pByteArray[15] << 0);

	return QUuid(l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8);
}

inline void convert_uuid(const QUuid &rUuid, unsigned char *pByteArray) {

	pByteArray[0] = rUuid.data1 >> 24;
	pByteArray[1] = rUuid.data1 >> 16;
	pByteArray[2] = rUuid.data1 >> 8;
	pByteArray[3] = rUuid.data1 >> 0;

	pByteArray[4] = rUuid.data2 >> 8;
	pByteArray[5] = rUuid.data2 >> 0;

	pByteArray[6] = rUuid.data3 >> 8;
	pByteArray[7] = rUuid.data3 >> 0;

	pByteArray[8] = rUuid.data4[0];
	pByteArray[9] = rUuid.data4[1];
	pByteArray[10] = rUuid.data4[2];
	pByteArray[11] = rUuid.data4[3];
	pByteArray[12] = rUuid.data4[4];
	pByteArray[13] = rUuid.data4[5];
	pByteArray[14] = rUuid.data4[6];
	pByteArray[15] = rUuid.data4[7];
}

inline QString get_file_name(const QString &rFilePath) {

	return QString(rFilePath.split('/').last());
}


inline bool is_wav_file(const QString &rFilePath) {

	if(QFileInfo(rFilePath).suffix().compare("wav", Qt::CaseInsensitive) == 0) return true;
	return false;
}


inline bool is_ttml_file(const QString &rFilePath) {

	if(QFileInfo(rFilePath).suffix().compare("ttml", Qt::CaseInsensitive) == 0 || QFileInfo(rFilePath).suffix().compare("xml", Qt::CaseInsensitive) == 0) return true;
	return false;
}


inline bool is_mxf_file(const QString &rFilePath) {

	if(QFileInfo(rFilePath).suffix().compare("mxf", Qt::CaseInsensitive) == 0) return true;
	return false;
}


inline bool is_wav_file(const QFileInfo &rFilePath) {

	if(rFilePath.suffix().compare("wav", Qt::CaseInsensitive) == 0) return true;
	return false;
}


inline bool is_ttml_file(const QFileInfo &rFilePath) {

	if(rFilePath.suffix().compare("ttml", Qt::CaseInsensitive) == 0 || rFilePath.suffix().compare("xml", Qt::CaseInsensitive) == 0) return true;
	return false;
}


inline bool is_mxf_file(const QFileInfo &rFilePath) {

	if(rFilePath.suffix().compare("mxf", Qt::CaseInsensitive) == 0) return true;
	return false;
}


inline QDir get_app_data_location() {

#ifdef NDEBUG
	QString writeable_location(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
	QDir dir(writeable_location);
	if(!dir.exists(PROJECT_NAME)) {
		if(!dir.mkpath(PROJECT_NAME)) {
			qCritical() << "Couldn't create writable folder. Fallback to current path.";
			return QDir::current();
		}
	}
	dir.cd(PROJECT_NAME);
	return dir;
#else
	return QDir::current();
#endif // NDEBUG
}


inline QDir get_app_data_ctl_location() {

	QDir dir = get_app_data_location();
	if(!dir.exists("ctl")) {
		if(!dir.mkpath("ctl")) {
			qCritical() << "Couldn't create writable folder. Fallback to current path.";
			return QDir::current();
		}
	}
	dir.cd("ctl");
	return dir;
}


inline QMainWindow* get_main_window() {

	for(int i = 0; i < qApp->topLevelWidgets().size(); i++) {
		if(qApp->topLevelWidgets().at(i)->inherits("QMainWindow")) {
			return qobject_cast<QMainWindow *>(qApp->topLevelWidgets().at(i));
		}
	}
	qCritical() << "Couldn't extract main window.";
	return NULL;
}


class IconProviderExrWav : public QObject, public QFileIconProvider {

public:
	IconProviderExrWav(QObject *pParent = NULL) : QObject(pParent), QFileIconProvider() {}
	virtual ~IconProviderExrWav() {}

private:
	Q_DISABLE_COPY(IconProviderExrWav);
	virtual QIcon icon(const QFileInfo &info) const {
		if(info.suffix() == "wav") return QIcon(":/sound.png");
		else if(info.suffix() == "ttml" || info.suffix() == "xml") return QIcon(":/text.png");
		return QFileIconProvider::icon(info);
	}
};
