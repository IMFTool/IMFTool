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
#include "ImfPackageCommon.h"
#include <fstream>


XmlSerializationError::XmlSerializationError(const xml_schema::Serialization &rError) {

	for(unsigned int i = 0; i < rError.diagnostics().size(); i++) {
		if(rError.diagnostics().at(i).severity() == xml_schema::Severity::error) {
			AppendErrorDescription(QString("\tXML-DOM (error): line %1 %2").arg(rError.diagnostics().at(i).line()).arg(rError.diagnostics().at(i).message().c_str()));
		}
		else if(rError.diagnostics().at(i).severity() == xml_schema::Severity::warning) {
			AppendErrorDescription(QString("\tXML-DOM (warning): line %1 %2").arg(rError.diagnostics().at(i).line()).arg(rError.diagnostics().at(i).message().c_str()));
		}
	}
}

XmlSerializationError::XmlSerializationError(const xml_schema::UnexpectedElement &rError) {

	AppendErrorDescription(QString("\tXML - tree : name %1 expected name %2").arg(rError.encountered_name().c_str()).arg(rError.expected_name().c_str()));
}

XmlSerializationError::XmlSerializationError(const xml_schema::NoTypeInfo &rError) {

	AppendErrorDescription(QString("\tXML - tree : type %1").arg(rError.type_name().c_str()));
}

XmlParsingError::XmlParsingError(const xml_schema::Parsing &rError) : mErrorType(XmlParsingError::Parsing), mErrorDescription(rError.what()) {

	for(unsigned int i = 0; i < rError.diagnostics().size(); i++) {
		if(rError.diagnostics().at(i).severity() == xml_schema::Severity::error) {
			AppendErrorDescription(QString("\tXML-DOM (error): line %1 %2").arg(rError.diagnostics().at(i).line()).arg(rError.diagnostics().at(i).message().c_str()));
		}
		else if(rError.diagnostics().at(i).severity() == xml_schema::Severity::warning) {
			AppendErrorDescription(QString("\tXML-DOM (warning): line %1 %2").arg(rError.diagnostics().at(i).line()).arg(rError.diagnostics().at(i).message().c_str()));
		}
	}
}

XmlParsingError::XmlParsingError(const xml_schema::ExpectedElement &rError) : mErrorType(XmlParsingError::ExpectedElement), mErrorDescription(rError.what()) {

	AppendErrorDescription(QString("\tXML-tree : name %1").arg(rError.name().c_str()));
}

XmlParsingError::XmlParsingError(const xml_schema::UnexpectedElement &rError) : mErrorType(XmlParsingError::UnexpectedElement), mErrorDescription(rError.what()) {

	AppendErrorDescription(QString("\tXML - tree : name %1 expected name %2").arg(rError.encountered_name().c_str()).arg(rError.expected_name().c_str()));
}

XmlParsingError::XmlParsingError(const xml_schema::ExpectedAttribute &rError) : mErrorType(XmlParsingError::ExpectedAttribute), mErrorDescription(rError.what()) {

	AppendErrorDescription(QString("\tXML - tree : attribute %1").arg(rError.name().c_str()));
}

XmlParsingError::XmlParsingError(const xml_schema::UnexpectedEnumerator &rError) : mErrorType(XmlParsingError::UnexpectedEnumerator), mErrorDescription(rError.what()) {

	AppendErrorDescription(QString("\tXML - tree : enumerator %1").arg(rError.enumerator().c_str()));
}

XmlParsingError::XmlParsingError(const xml_schema::ExpectedTextContent &rError) : mErrorType(XmlParsingError::ExpectedTextContent), mErrorDescription(rError.what()) {

}

XmlParsingError::XmlParsingError(const xml_schema::NoTypeInfo &rError) : mErrorType(XmlParsingError::NoTypeInfo), mErrorDescription(rError.what()) {

	AppendErrorDescription(QString("\tXML - tree : type %1").arg(rError.type_name().c_str()));
}

XmlParsingError::XmlParsingError(const xml_schema::NotDerived &rError) : mErrorType(XmlParsingError::NotDerived), mErrorDescription(rError.what()) {

	AppendErrorDescription(QString("\tXML - tree : base type %1 derived type %2").arg(rError.base_type_name().c_str()).arg(rError.derived_type_name().c_str()));
}

XmlParsingError::XmlParsingError(const xml_schema::NoPrefixMapping &rError) : mErrorType(XmlParsingError::NoPrefixMapping), mErrorDescription(rError.what()) {

	AppendErrorDescription(QString("\tXML - tree : prefix %1").arg(rError.prefix().c_str()));
}

QDebug operator<<(QDebug dbg, const XmlParsingError &rError) {

	dbg.nospace() << "XML Parsing Error: " << rError.GetErrorMsg() << " Detail: " << rError.GetErrorDescription();
	return dbg.space();
}

QDebug operator<<(QDebug dbg, const XmlSerializationError &rError) {

	dbg.nospace() << "XML Serialization Error: " << rError.GetErrorMsg() << " Detail: " << rError.GetErrorDescription();
	return dbg.space();
}
//WR
std::auto_ptr<pkl2016::PackingListType> ImfXmlHelper::Convert(std::auto_ptr<pkl::PackingListType> rPackingList2013) {

	pkl2016::PackingListType::AssetListType rAssetList;
	pkl2016::PackingListType_AssetListType::AssetSequence seq;
	pkl::PackingListType_AssetListType::AssetSequence::iterator i;

	for (i = rPackingList2013->getAssetList().getAsset().begin(); i < rPackingList2013->getAssetList().getAsset().end(); i++) {
		pkl::AssetType asset = *i;
		const pkl2016::AssetType asset2016(asset.getId(), asset.getHash(), asset.getSize(), asset.getType(),
				pkl2016::AssetType::HashAlgorithmType(ds::CanonicalizationMethodType::AlgorithmType("http://www.w3.org/2000/09/xmldsig#sha1")));
		seq.push_back(asset2016);
	}
	rAssetList.setAsset(seq);
	//const pkl2016::PackingListType::IdType uuid = ImfXmlHelper::Convert(QUuid::createUuid());
	const pkl2016::PackingListType::IdType uuid = rPackingList2013->getId();
	const pkl2016::PackingListType::IssueDateType date(0,0,0,0,0,0);
	std::auto_ptr<pkl2016::PackingListType> rPackingList2016(new pkl2016::PackingListType(rPackingList2013->getId(), date,
			(const pkl2016::PackingListType::IssuerType&)"Issuer", (const pkl2016::PackingListType::CreatorType&)"Creator",
			(const pkl2016::PackingListType::AssetListType&)rAssetList
			));
	return rPackingList2016;
}

 ::ContentVersionList ImfXmlHelper::Convert(cpl2016::CompositionPlaylistType_ContentVersionListType rContentVersionList) {
	::ContentVersionList content_version_list;
	cpl2016::CompositionPlaylistType_ContentVersionListType::ContentVersionSequence &content_version_sequence = rContentVersionList.getContentVersion();
	cpl2016::CompositionPlaylistType_ContentVersionListType::ContentVersionSequence::iterator i;

	for (i = rContentVersionList.getContentVersion().begin(); i < rContentVersionList.getContentVersion().end(); i++) {
		cpl2016::ContentVersionType content_version = *i;
		content_version_list.append(Convert(content_version));
	}

	return content_version_list;

}
std::auto_ptr<cpl2016::CompositionPlaylistType_ContentVersionListType> ImfXmlHelper::Convert(const ::ContentVersionList &rContentVersionList) {
	std::auto_ptr<cpl2016::CompositionPlaylistType_ContentVersionListType> content_version_list(new cpl2016::CompositionPlaylistType_ContentVersionListType());
	cpl2016::CompositionPlaylistType_ContentVersionListType::ContentVersionSequence &content_version_sequence = content_version_list->getContentVersion();
	content_version_sequence.clear();
	for (int i = 0; i < rContentVersionList.count(); i++) {
		content_version_sequence.push_back(Convert(rContentVersionList.at(i)));
	}
	return content_version_list;
}


::LocaleList ImfXmlHelper::Convert(cpl2016::CompositionPlaylistType_LocaleListType rLocaleList) {
	::LocaleList locale_list;
	cpl2016::CompositionPlaylistType_LocaleListType::LocaleSequence &locale_sequence = rLocaleList.getLocale();
	cpl2016::CompositionPlaylistType_LocaleListType::LocaleSequence::iterator i;

	for (i = rLocaleList.getLocale().begin(); i < rLocaleList.getLocale().end(); i++) {
		cpl2016::LocaleType locale = *i;
		locale_list.append(Convert(locale));
	}

	return locale_list;

}


std::auto_ptr<cpl2016::CompositionPlaylistType_LocaleListType> ImfXmlHelper::Convert(const ::LocaleList &rLocaleList) {
	std::auto_ptr<cpl2016::CompositionPlaylistType_LocaleListType> locale_list(new cpl2016::CompositionPlaylistType_LocaleListType());
	cpl2016::CompositionPlaylistType_LocaleListType::LocaleSequence &locale_sequence = locale_list->getLocale();
	locale_sequence.clear();
	for (int i = 0; i < rLocaleList.count(); i++) {
		locale_sequence.push_back(Convert(rLocaleList.at(i)));
	}
	return locale_list;
}

::Locale ImfXmlHelper::Convert(const cpl2016::LocaleType &rLocale) {
	return ::Locale(
			(rLocale.getAnnotation().present() ? Convert(rLocale.getAnnotation().get()) : UserText()),
			(rLocale.getLanguageList().present() ? Convert(rLocale.getLanguageList().get().getLanguage()) : QList<QString>()),
			(rLocale.getRegionList().present() ? Convert(rLocale.getRegionList().get().getRegion()) : QList<QString>()),
			(rLocale.getContentMaturityRatingList().present() ? Convert(rLocale.getContentMaturityRatingList().get().getContentMaturityRating()) : QList<ContentMaturityRating>())
			);
}

// Also accepts cpl2016::LocaleType_RegionListType::RegionSequence argument type
QList<QString> ImfXmlHelper::Convert(cpl2016::LocaleType_LanguageListType::LanguageSequence rLanguageSequence) {
	cpl2016::LocaleType_LanguageListType::LanguageSequence::iterator i;
	QList<QString> language_list;
	for (i = rLanguageSequence.begin(); i < rLanguageSequence.end(); i++) {
		const cpl2016::LocaleType_LanguageListType::LanguageType language = *i;
		language_list.append(QString(language.c_str()));
	}
	return language_list;
}

QList<ContentMaturityRating> ImfXmlHelper::Convert(cpl2016::LocaleType_ContentMaturityRatingListType::ContentMaturityRatingSequence rContentMaturityRatingSequence) {
	cpl2016::LocaleType_ContentMaturityRatingListType::ContentMaturityRatingSequence::iterator i;
	QList<ContentMaturityRating> maturity_sequence;
	for (i = rContentMaturityRatingSequence.begin(); i < rContentMaturityRatingSequence.end(); i++) {
		UserText agency, rating;
		QPair<QString, QString> audience;
		ContentMaturityRating ratingElement;
		cpl2016::ContentMaturityRatingType cpl_ratingELement = *i;
		agency = Convert(am::UserText(cpl_ratingELement.getAgency()));
		cpl2016::ContentMaturityRatingType::RatingType rat;
		//rat.string();
		rating = Convert(am::UserText(cpl_ratingELement.getRating()));
		if (cpl_ratingELement.getAudience().present()) {
			audience = QPair<QString, QString>(Convert(cpl_ratingELement.getAudience().get().getScope()), QString(cpl_ratingELement.getAudience().get().c_str()));
		}
		ratingElement.setAgency(agency);
		ratingElement.setRating(rating);
		ratingElement.setAudience(audience);
		maturity_sequence.append(ratingElement);
	}
	return maturity_sequence;
}

 cpl2016::LocaleType ImfXmlHelper::Convert(const ::Locale &rLocale) {
	cpl2016::LocaleType locale;
	::Locale myLocale = rLocale;

	//Add Annotation
	locale.setAnnotation(dcml::UserTextType(Convert(myLocale.getAnnotation())));

	//Add LanguageList
	cpl2016::LocaleType_LanguageListType::LanguageType language;
	cpl2016::LocaleType_LanguageListType::LanguageSequence languageSequence;
	cpl2016::LocaleType_LanguageListType languageList;
	bool languagePresent = false;
	foreach (QString lang, myLocale.getLanguageList()) {
		language = cpl2016::LocaleType_LanguageListType::LanguageType(lang.toStdString());
		if (!lang.isEmpty()) {
			languageSequence.push_back(language);
			languagePresent = true;
		}

	}
	if (languagePresent) {
		languageList.setLanguage(languageSequence);
		locale.setLanguageList(languageList);
	}

	//Add RegionList
	cpl2016::LocaleType_RegionListType::RegionType region;
	cpl2016::LocaleType_RegionListType::RegionSequence regionSequence;
	cpl2016::LocaleType_RegionListType regionList;
	bool regionPresent = false;
	foreach (QString reg, myLocale.getRegionList()) {
		region = cpl2016::LocaleType_RegionListType::RegionType(reg.toStdString());
		if (!reg.isEmpty()) {
			regionSequence.push_back(region);
			regionPresent = true;
		}
	}
	if (regionPresent) {
		regionList.setRegion(regionSequence);
		locale.setRegionList(regionList);
	}

	//Add ContentMaturityRatingList
	cpl2016::LocaleType_ContentMaturityRatingListType::ContentMaturityRatingSequence cmrSequence;
	cpl2016::LocaleType_ContentMaturityRatingListType cmrList;
	bool cmrPresent = false;
	foreach (ContentMaturityRating cmr_model, myLocale.getContentMaturityRating()) {
		cpl2016::LocaleType_ContentMaturityRatingListType::ContentMaturityRatingType
			cmr(xml_schema::Uri(cmr_model.getAgency().first.toStdString().c_str()), cmr_model.getRating().first.toStdString());
		//If cmr_model.getAudience().second.toStdString().isEmpty() == true: AudienceType will not be created
		cmr.setAudience(
				cpl2016::ContentMaturityRatingType_AudienceType(cmr_model.getAudience().second.toStdString().c_str(), xml_schema::Uri(cmr_model.getAudience().first.toStdString().c_str()))
		);
		if (!cmr_model.getRating().first.isEmpty()) {
			cmrSequence.push_back(cmr);
			cmrPresent = true;
		}

	}
	if (cmrPresent) {
		cmrList.setContentMaturityRating(cmrSequence);
		locale.setContentMaturityRatingList(cmrList);
	}

	return locale;
}

int ImfXmlHelper::RemoveWhiteSpaces(const QString &rPathName) {
	// Remove trailing CR and CRLF from base64 encoded hashes
	// The XSD Code Synthesis serializer inserts CR (mac) and CRLF (Win) for type base64Binary
	// Trailing CR/CRLF are allowed in base64, nevertheless some QC systems are complaining about them.
	int result = 0;
#ifdef WIN32
	QByteArray before("\r\n</Hash>");
#else
	QByteArray before("\n</Hash>");
#endif
	QByteArray after("</Hash>");
	QFile file(rPathName);
	QByteArray blob;
	if (file.open(QIODevice::ReadOnly)) {
		blob = file.readAll();
		blob.replace(before, after);
		file.remove();
		file.close();
		if (file.open(QIODevice::WriteOnly)) {
			file.write(blob);
			file.close();
		} else result = -1;
	} else result = -1;
	return result;
}



//WR
