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
#include "SMPTE-429-8-2014-AM.h"
#include "SMPTE-429-8-2006-PKL.h"
#include "SMPTE-2067-3-2013-CPL.h"
#include "st2067-2b-2016-PKL.h"
#include "st2067-3a-2016-CPL.h"
#include "SafeBool.h"
#include "global.h"
#include "ImfCommon.h"
#include <QFileInfo>
#include <QString>
#include <QDateTime>
#include <QUuid>
#include <QStringList>
#include <QPair>
#include <QVector>





class ImfPackage;

class UserText : public QPair<QString, QString> {

public:
	UserText() : QPair<QString, QString>(QString(), QString("en")) {};
	UserText(const QString &rText, const QString &rLanguage = "en") : QPair<QString, QString>(rText, rLanguage) {};
	bool IsEmpty() const { return first.isEmpty(); }
	bool operator== (const UserText &rOther) { return first == rOther.first && second == rOther.second; }
	bool operator!= (const UserText &rOther) { return first != rOther.first || second != rOther.second; }
};

//WR
class ContentVersion : public QPair<QString, UserText> {

public:
	ContentVersion() : QPair<QString, UserText>(QString(), UserText()) {};
	ContentVersion(const QString &rUuid, const UserText &rUserText) : QPair<QString, UserText>(rUuid, rUserText) {};
	bool IsEmpty() const { return second.IsEmpty(); }
	bool operator== (const ContentVersion &rOther) { return first == rOther.first && second == rOther.second; }
	bool operator!= (const ContentVersion &rOther) { return first != rOther.first || second != rOther.second; }
};

class ContentVersionList : public QList<ContentVersion> {
//	ContentVersionList() : QList<ContentVersion>() {};
};

class ContentMaturityRating {

public:
	ContentMaturityRating() {};
	ContentMaturityRating(const QString &rAgency, const QString &rRating, const QPair<QString, QString> &rAudience = (QPair<QString, QString>()) ) { mAgency = rAgency; mRating = rRating; mAudience = rAudience; };
	UserText getAgency() { return mAgency; };
	UserText getRating() { return mRating; };
	QPair<QString, QString> getAudience() { return mAudience; };
	void setAgency(const UserText &rAgency) { mAgency = rAgency; };
	void setRating(const UserText &rRating) { mRating = rRating; };
	void setAudience(const QPair<QString, QString> &rAudience) { mAudience = rAudience; };

private:
	UserText mAgency;
	UserText mRating;
	QPair<QString, QString> mAudience; // (anyURI, String)
};

class Locale {

public:
	Locale() {};
	Locale(const UserText &rAnnotation, const QList<QString> &rLanguageList, const QList<QString> &rRegionList, const QList<ContentMaturityRating> rContentMaturityRatingList) : mAnnotation(rAnnotation), mLanguageList(rLanguageList), mRegionList(rRegionList), mContentMaturityRatingList(rContentMaturityRatingList){};
	bool IsEmpty() const { return mAnnotation.first.isEmpty() && mLanguageList.isEmpty() && mRegionList.isEmpty() && mContentMaturityRatingList.isEmpty(); }
	//bool operator== (const ContentVersion &rOther) { return first == rOther.first && second == rOther.second; }
	//bool operator!= (const ContentVersion &rOther) { return first != rOther.first || second != rOther.second; }
	UserText getAnnotation() { return mAnnotation; };
	QList<QString> getLanguageList() { return mLanguageList; };
	QList<QString> getRegionList() { return mRegionList; };
	QList<ContentMaturityRating> getContentMaturityRating() { return mContentMaturityRatingList; };
	void setAnnotation(UserText rUserText) {mAnnotation = rUserText; };
	void setLanguageList(QList<QString> rLanguageList) { mLanguageList = rLanguageList; };
	void setRegionList(QList<QString> rRegionList) { mRegionList = rRegionList; };
	void setContentMaturityRating(QList<ContentMaturityRating> rContentMaturityRatingList) { mContentMaturityRatingList = rContentMaturityRatingList;} ;

private:
	UserText mAnnotation;
	QList<QString> mLanguageList;
	QList<QString> mRegionList;
	QList<ContentMaturityRating> mContentMaturityRatingList;

};

class LocaleList : public QList<Locale> {
public:
//	LocaleList() : QList<Locale>() {};
};


//WR


class XmlSerializationError {

public:
	enum eError {
		None = 0,
		Serialization,
		UnexpectedElement,
		NoTypeInfo,
		Unknown
	};
	//! Constructs empty error (IsError returns false).
	XmlSerializationError() : mErrorType(XmlSerializationError::None) {}
	XmlSerializationError(const xml_schema::Serialization &rError);
	XmlSerializationError(const xml_schema::UnexpectedElement &rError);
	XmlSerializationError(const xml_schema::NoTypeInfo &rError);
	XmlSerializationError(XmlSerializationError::eError error, const QString &rDescription = QString()) : mErrorType(error), mErrorDescription(rDescription) {}
	~XmlSerializationError() {}
	bool IsError() const { return mErrorType; }
	void AppendErrorDescription(const QString &rAppend) { mErrorDescription.append(rAppend); }
	QString GetErrorMsg() const {
		QString ret;
		switch(mErrorType) {
			case XmlSerializationError::None:
				ret = QObject::tr("No Error"); break;
			case XmlSerializationError::Serialization:
				ret = QObject::tr("Serialization or validation error during the XML-DOM stage."); break;
			case XmlSerializationError::UnexpectedElement:
				ret = QObject::tr("Unexpected Element is encountered by the DOM-Tree stage."); break;
			case XmlSerializationError::NoTypeInfo:
				ret = QObject::tr("No type information associated with a type specified by the xsi:type attribute."); break;
			default:
				ret = QObject::tr("Unknown XML parsing error"); break;
		}
		return ret;
	}
	QString GetErrorDescription() const { return mErrorDescription; }

private:
	eError		mErrorType;
	QString		mErrorDescription;
};


QDebug operator<< (QDebug dbg, const XmlSerializationError &rError);


class XmlParsingError {

public:
	enum eError {
		None = 0,
		Parsing,
		ExpectedElement,
		UnexpectedElement,
		ExpectedAttribute,
		UnexpectedEnumerator,
		ExpectedTextContent,
		NoTypeInfo,
		NotDerived,
		NoPrefixMapping,
		Unknown
	};
	//! Constructs empty error (IsError returns false).
	XmlParsingError() : mErrorType(XmlParsingError::None) {}
	XmlParsingError(const xml_schema::Parsing &rError);
	XmlParsingError(const xml_schema::ExpectedElement &rError);
	XmlParsingError(const xml_schema::UnexpectedElement &rError);
	XmlParsingError(const xml_schema::ExpectedAttribute &rError);
	XmlParsingError(const xml_schema::UnexpectedEnumerator &rError);
	XmlParsingError(const xml_schema::ExpectedTextContent &rError);
	XmlParsingError(const xml_schema::NoTypeInfo &rError);
	XmlParsingError(const xml_schema::NotDerived &rError);
	XmlParsingError(const xml_schema::NoPrefixMapping &rError);
	XmlParsingError(XmlParsingError::eError error, const QString &rDescription = QString()) : mErrorType(error), mErrorDescription(rDescription) {}
	~XmlParsingError() {}
	bool IsError() const { return mErrorType; }
	void AppendErrorDescription(const QString &rAppend) { mErrorDescription.append(rAppend); }
	QString GetErrorMsg() const {
		QString ret;
		switch(mErrorType) {
			case XmlParsingError::None:
				ret = QObject::tr("No Error"); break;
			case XmlParsingError::Parsing:
				ret = QObject::tr("Parsing or validation error during the XML-DOM stage."); break;
			case XmlParsingError::ExpectedElement:
				ret = QObject::tr("Expected Element is not encountered by the DOM-Tree stage."); break;
			case XmlParsingError::UnexpectedElement:
				ret = QObject::tr("Unexpected Element is encountered by the DOM-Tree stage."); break;
			case XmlParsingError::ExpectedAttribute:
				ret = QObject::tr("Expected Attribute is not encountered by the DOM-Tree stage."); break;
			case XmlParsingError::UnexpectedEnumerator:
				ret = QObject::tr("Unexpected Enumerator is encountered by the DOM-Tree stage."); break;
			case XmlParsingError::ExpectedTextContent:
				ret = QObject::tr("Content other than text is encountered and the text content was expected by the DOM-Tree stage."); break;
			case XmlParsingError::NoTypeInfo:
				ret = QObject::tr("No type information associated with a type specified by the xsi:type attribute."); break;
			case XmlParsingError::NotDerived:
				ret = QObject::tr("A type specified by the xsi:type attribute is not derived from the expected base type."); break;
			case XmlParsingError::NoPrefixMapping:
				ret = QObject::tr("A namespace prefix is encountered for which a prefix-namespace mapping hasn't been provided."); break;
			default:
				ret = QObject::tr("Unknown XML parsing error"); break;
		}
		return ret;
	}
	QString GetErrorDescription() const { return mErrorDescription; }

private:
	eError		mErrorType;
	QString		mErrorDescription;
};

QDebug operator<< (QDebug dbg, const XmlParsingError &rError);


class ImfError : public SafeBool<ImfError> {

public:
	enum eError {

		None = 0,
		WorkingDirNotFound,
		MultipleAssetMapsFound,
		NoAssetMapFound,
		NoPackingListFound,
		MultipleChunks,
		AssetMapSplit,
		UnknownAsset,
		AssetFileMissing,
		DestinationFileUnspecified,
		UnknownInheritance,
		XMLParsing,
		XMLSerialization,
		Unknown
	};
	//! Constructs empty error (IsError returns false).
	ImfError() : mErrorType(None), mErrorDescription(""), mRecoverable(true) {}
	//! Imports Xml parsing error
	ImfError(const XmlParsingError &rXmlError) : mErrorType(XMLParsing), mErrorDescription(rXmlError.GetErrorMsg().append(rXmlError.GetErrorDescription())), mRecoverable(false) {}
	//! Imports Xml serialization error
	ImfError(const XmlSerializationError &rXmlError) : mErrorType(XMLSerialization), mErrorDescription(rXmlError.GetErrorMsg().append(rXmlError.GetErrorDescription())), mRecoverable(false) {}
	ImfError(eError error, const QString &rDescription = QString(), bool recoverable = false) : mErrorType(error), mErrorDescription(rDescription), mRecoverable(recoverable) {}
	~ImfError() {}
	bool IsError() const { return (mErrorType != None && mRecoverable == false); }
	bool IsRecoverableError() const { return (mErrorType != None && mRecoverable == true); }
	void AppendErrorDescription(const QString &rAppend) { mErrorDescription.append(rAppend); }
	QString GetErrorMsg() const {
		QString ret;
		switch(mErrorType) {
			case None:
				ret = QObject::tr("No Error"); break;
			case WorkingDirNotFound:
				ret = QObject::tr("Working directory does not exist."); break;
			case MultipleAssetMapsFound:
				ret = QObject::tr("There are multiple Asset Maps in subdirectories. This Feature is not supported yet."); break;
			case NoAssetMapFound:
				ret = QObject::tr("There are no Asset Maps in this working directory."); break;
			case NoPackingListFound:
				ret = QObject::tr("There are no Packing Lists in this working directory."); break;
			case MultipleChunks:
				ret = QObject::tr("The Asset consists of multiple chunks. This Feature is deprecated (SMPTE ST 429-9:2014)."); break;
			case AssetMapSplit:
				ret = QObject::tr("The Asset Map is split across multiple storage volumes. This Feature is deprecated (SMPTE ST 429-9:2014)."); break;
			case  UnknownAsset:
				ret = QObject::tr("Unknown Asset found."); break;
			case AssetFileMissing:
				ret = QObject::tr("The Asset doesn't exist on the file system."); break;
			case DestinationFileUnspecified:
				ret = QObject::tr("The destination file is unspecified."); break;
			case UnknownInheritance:
				ret = QObject::tr("Unknown inheritance in XML DOM."); break;
			case XMLParsing:
				ret = QObject::tr("The XML parsing failed."); break;
			case XMLSerialization:
				ret = QObject::tr("The XML serialization failed."); break;
			case Unknown:
				ret = QObject::tr("Unknown error"); break;
			default:
				ret = QObject::tr("Unknown error"); break;
		}
		return ret;
	}
	QString GetErrorDescription() const { return mErrorDescription; }
	bool BooleanTest() const { return IsError(); }

private:
	eError	mErrorType;
	QString mErrorDescription;
	bool		mRecoverable;
};

class ImfXmlHelper {

public:

	static QUuid Convert(const am::UUID &rUuid) {

		QString uuid(rUuid.c_str());
		return QUuid(uuid.split(':').last());
	}

	static QString Convert(const cpl2016::ContentVersionType::IdType &rUuid) {

		QString uuid(rUuid.c_str());
		return uuid;
	}

	static cpl2016::ContentVersionType::IdType Convert(const QString &rUuid) {

		cpl2016::ContentVersionType::IdType rId(rUuid.toStdString().c_str());
		//QString uuid(rUuid.c_str());
		return rId;
	}

	static am::UUID Convert(const QUuid &rUuid) {

		xml_schema::Uri uri(QString("urn:uuid:").append(strip_uuid(rUuid)).toStdString());
		return am::UUID(uri);
	}

	static xml_schema::DateTime Convert(const QDateTime &rDateTime) {

		QDate date = rDateTime.date();
		QTime time = rDateTime.time();
		int offset = rDateTime.offsetFromUtc();
		xml_schema::DateTime date_time = xml_schema::DateTime(date.year(), date.month(), date.day(), time.hour(), time.minute(), (double)time.second());
		date_time.zone_hours(offset / 3600);
		date_time.zone_minutes((offset % 3600) / 60);
		return date_time;
	}

	static QDateTime Convert(const xml_schema::DateTime &rDateTime) {

		QDateTime date_time = QDateTime(QDate(rDateTime.year(), rDateTime.month(), rDateTime.day()), QTime(rDateTime.hours(), rDateTime.minutes(), rDateTime.seconds())); // TODO does this work?
		if(rDateTime.zone_present() == true) {
			date_time.setOffsetFromUtc(rDateTime.zone_hours() * 3600 + rDateTime.zone_minutes() * 60);
		}
		return date_time;
	}

	static am::UserText Convert(const ::UserText &rText) {

		am::UserText text(rText.first.toStdString());
		if(rText.second != "en") text.setLanguage(am::UserText::LanguageType(rText.second.toStdString()));
		return text;
	}

	static ::UserText Convert(const am::UserText &rText) {

		return ::UserText(rText.c_str(), rText.getLanguage().c_str());
	}

	static QUuid Convert(const pkl2016::UUID &rUuid) {

		QString uuid(rUuid.c_str());
		return QUuid(uuid.split(':').last());
	}

	static ::UserText Convert(const pkl2016::UserText &rText) {

		return ::UserText(rText.c_str(), rText.getLanguage().c_str());
	}

	static xml_schema::Base64Binary Convert(const QByteArray &rHash) {

		return xml_schema::Base64Binary(rHash.constData(), rHash.size());
	}

	static QByteArray Convert(const xml_schema::Base64Binary &rHash) {

		return QByteArray(rHash.data(), rHash.size());
	}

	static QUuid Convert(const dcml::UUIDType &rUuid) {

		QString uuid(rUuid.c_str());
		return QUuid(uuid.split(':').last());
	}

	static ::UserText Convert(const dcml::UserTextType &rText) {

		return ::UserText(rText.c_str(), rText.getLanguage().c_str());
	}

	static ::EditRate Convert(const dcml::RationalType &rEditRate) {

		if(rEditRate.size() == 2) {
			return EditRate(rEditRate.at(0), rEditRate.at(1));
		}
		return EditRate();
	}

	static dcml::RationalType Convert(const ::EditRate &rEditRate) {

		dcml::RationalType edit_rate;
		edit_rate.push_back(rEditRate.GetNumerator());
		edit_rate.push_back(rEditRate.GetDenominator());
		return edit_rate;
	}

	static cpl2016::MarkerType_LabelType Convert(const ::MarkerLabel &rMarkerLabel) {

		cpl2016::MarkerType_LabelType marker(rMarkerLabel.GetLabel().toStdString());
		marker.setScope(rMarkerLabel.GetScope().toStdString());
		return marker;
	}

	static ::MarkerLabel Convert(const cpl2016::MarkerType_LabelType &rMarkerLabel) {

		MarkerLabel ret;
		const cpl2016::MarkerType_LabelType::ScopeType markerScope2016(WELL_KNOWN_MARKER_LABEL_SCOPE_2016);
		if(rMarkerLabel.getScope().compare(rMarkerLabel.getScopeDefaultValue()) == 0) {
			ret = MarkerLabel::GetMarker(QString(rMarkerLabel.c_str()));
		}
		else if(rMarkerLabel.getScope().compare(markerScope2016) == 0) {
			ret = MarkerLabel::GetMarker(QString(rMarkerLabel.c_str()));
		}
		else {
			ret = MarkerLabel(QString(rMarkerLabel.c_str()), "Unknown Marker", rMarkerLabel.getScope().c_str());
		}
		return ret;
	}
	//WR
	static ::UserText Convert(const cpl2016::ContentKindType &rContentKindType) {

		return ::UserText(rContentKindType.c_str(), "");
	}
	static std::auto_ptr<pkl2016::PackingListType> Convert(std::auto_ptr<pkl::PackingListType> rPackingList2013);

	static ::ContentVersion Convert(const cpl2016::ContentVersionType &rContentVersion) {
		return ::ContentVersion( Convert(rContentVersion.getId()), Convert(rContentVersion.getLabelText()) );
		//tbd
		//rContentVersion->getAny();
	}

	static cpl2016::ContentVersionType Convert(const ::ContentVersion &rContentVersion) {
		cpl2016::ContentVersionType content_version(Convert(rContentVersion.first), Convert(rContentVersion.second));
		return content_version;
	}

	static ::ContentVersionList Convert(cpl2016::CompositionPlaylistType_ContentVersionListType rContentVersionList);

	static std::auto_ptr<cpl2016::CompositionPlaylistType_ContentVersionListType> Convert(const ::ContentVersionList &rContentVersionList);

	static ::LocaleList Convert(cpl2016::CompositionPlaylistType_LocaleListType rLocaleList);

	static std::auto_ptr<cpl2016::CompositionPlaylistType_LocaleListType> Convert(const ::LocaleList &rLocaleList);

	static ::Locale Convert(const cpl2016::LocaleType &rLocale);

	static QList<QString> Convert(cpl2016::LocaleType_LanguageListType::LanguageSequence rLanguageSequence);

	//static QList<QString> Convert(cpl2016::LocaleType_RegionListType::RegionSequence& rRegionSequence);

	static QList<ContentMaturityRating> Convert(cpl2016::LocaleType_ContentMaturityRatingListType::ContentMaturityRatingSequence rContentMaturityRatingSequence);

	static cpl2016::LocaleType Convert(const ::Locale &rLocale);
};
