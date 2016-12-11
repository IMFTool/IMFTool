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

//WR
