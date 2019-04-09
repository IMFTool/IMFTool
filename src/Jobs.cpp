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
#include "Jobs.h"
#include "AS_02.h"
#include "Metadata.h"
#include <vector>
#include "PCMParserList.h"
#include "AS_DCP_internal.h"
#include <QFileInfo>
#include <QCryptographicHash>
#include <QFile>
#include <QProcess>
#include <QDir>
#include <QTemporaryDir>
#include "SMPTE_Labels.h"
//regxmllibc
#include <com/sandflow/smpte/regxml/dict/MetaDictionaryCollection.h>
#include <com/sandflow/smpte/regxml/dict/importers/XMLImporter.h>
#include "com/sandflow/smpte/regxml/MXFFragmentBuilder.h"
#include <fstream>

XERCES_CPP_NAMESPACE_USE

using namespace rxml;
//regxmllibc

JobCalculateHash::JobCalculateHash(const QString &rSourceFile) :
AbstractJob(tr("Calculating Hash: %1").arg(QFileInfo(rSourceFile).fileName())), mSourceFile(rSourceFile) {

}

Error JobCalculateHash::Execute() {

	QFile file(mSourceFile);
	if(file.open(QIODevice::ReadOnly) == false) {
		return Error(Error::SourceFileOpenError, file.fileName());
	}

	QCryptographicHash hasher(QCryptographicHash::Sha1);

	Error error;
	char buffer[16 * 1024];
	qint64 count;
	qint64 file_size = file.size();
	qint64 bytes_read = 0;
	int progress = 0;
	int last_progress = 0;
	do {
		if(QThread::currentThread()->isInterruptionRequested()) {
			error = Error(Error::WorkerInterruptionRequest);
			break;
		}
		count = file.read(buffer, sizeof(buffer));
		if(count == -1) {
			error = Error(Error::HashCalculation, tr("Couldn't read file for Hash calculation."));
			break;
		}
		bytes_read += count;
		progress = bytes_read * 100 / file_size;
		if (progress >= last_progress + 10) {
			emit Progress(progress);
			last_progress = progress;
		}
		hasher.addData(buffer, count);
	} while(!file.atEnd());

	if(error.IsError() == false) {
		emit Result(hasher.result(), GetIdentifier());
	}
	file.close();
	return error;
}

JobWrapWav::JobWrapWav(const QStringList &rSourceFiles, const QString &rOutputFile, const SoundfieldGroup &rSoundFieldGroup, const QUuid &rAssetId, const QString &rLanguageTag, const QString &rMCATitle, const QString &rMCATitleVersion, const QString &rMCAAudioContentKind, const QString &rMCAAudioElementKind) :
AbstractJob(tr("Wrapping %1").arg(QFileInfo(rOutputFile).fileName())), mOutputFile(rOutputFile), mSourceFiles(rSourceFiles), mSoundFieldGoup(rSoundFieldGroup), mLanguageTag(rLanguageTag), mMCATitle(rMCATitle), mMCATitleVersion(rMCATitleVersion), mMCAAudioContentKind(rMCAAudioContentKind), mMCAAudioElementKind(rMCAAudioElementKind), mWriterInfo() {

	convert_uuid(rAssetId, (unsigned char*)mWriterInfo.AssetUUID);
}

Error JobWrapWav::Execute() {

	Error error;
	if(mSourceFiles.isEmpty() == false) {
		if(mSoundFieldGoup.IsComplete() == true) {
			// An AS - 02 PCM file is clip - wrapped, but the interface defined below mimics that used
			// for frame-wrapped essence elsewhere in this library.  The concept of frame rate
			// therefore is only relevant to these classes and is not reflected in or affected by
			// the contents of the MXF file.
			ASDCP::PCM::FrameBuffer buffer;
			ASDCP::PCMParserList parser;
			ASDCP::PCM::AudioDescriptor audio_descriptor;
			AS_02::PCM::MXFWriter writer;
			const ASDCP::Dictionary *dict = &ASDCP::DefaultSMPTEDict();
			ASDCP::MXF::WaveAudioDescriptor *essence_descriptor = NULL;
			ASDCP::MXF::AS02_MCAConfigParser mca_config(dict);
			int progress = 0;
			int last_progress = 0;
			//ASDCP::Rational edit_rate;
			QFileInfo output_file(mOutputFile);

			if(output_file.exists() && output_file.isFile() && !output_file.isSymLink()) QFile::remove(output_file.absoluteFilePath());
			QFileInfo file_info(mSourceFiles.first());
			Kumu::PathList_t source_files;
			for(int i = 0; i < mSourceFiles.size(); i++) {
				source_files.push_back(mSourceFiles.at(i).toStdString());
			}
			// set up MXF writer
			Kumu::Result_t result = parser.OpenRead(source_files, ASDCP::Rational(24, 1)); // Dirty hack? We need the duration in multiples of samples.
			if(ASDCP_SUCCESS(result)) {
				result = parser.FillAudioDescriptor(audio_descriptor);
				audio_descriptor.EditRate = ASDCP::Rational(24, 1);
				buffer.Capacity(ASDCP::PCM::CalcFrameBufferSize(audio_descriptor));
				essence_descriptor = new ASDCP::MXF::WaveAudioDescriptor(dict);
				result = ASDCP::PCM_ADesc_to_MD(audio_descriptor, essence_descriptor);
				if (mLanguageTag.isEmpty()) {
					if(mca_config.DecodeString(mSoundFieldGoup.GetAsString().toStdString()) == false) {
						error = Error(Error::MCAStringDecoding);
						return error;
					}
				} else {
					if(mca_config.DecodeString(mSoundFieldGoup.GetAsString().toStdString(), mLanguageTag.toStdString()) == false) {
						error = Error(Error::MCAStringDecoding);
						return error;
					}
				}
				if(mca_config.ChannelCount() != essence_descriptor->ChannelCount) {
					error = Error(Error::ChannelCountMismatch);
					return error;
				}
				ASDCP::MXF::InterchangeObject_list_t::iterator i;
				for ( i = mca_config.begin(); i != mca_config.end(); ++i ) {
				  if ( (*i)->GetUL() == ASDCP::UL(dict->ul(MDD_SoundfieldGroupLabelSubDescriptor))) {
					  ASDCP::MXF::SoundfieldGroupLabelSubDescriptor *current_soundfield;
					  current_soundfield = reinterpret_cast<ASDCP::MXF::SoundfieldGroupLabelSubDescriptor*>(*i);
					  if (current_soundfield) {
						  // Set SoundfieldGroupLabelSubDescriptor items which are mandatory per ST 2067-2
						  if (!mMCATitle.isEmpty()) current_soundfield->MCATitle = mMCATitle.toStdString();
						  if (!mMCATitleVersion.isEmpty()) current_soundfield->MCATitleVersion = mMCATitleVersion.toStdString();
						  if (!mMCAAudioContentKind.isEmpty()) current_soundfield->MCAAudioContentKind = mMCAAudioContentKind.toStdString();
						  if (!mMCAAudioElementKind.isEmpty()) current_soundfield->MCAAudioElementKind = mMCAAudioElementKind.toStdString();
						  //current_soundfield->Dump();
					  }
				  }
				}
				essence_descriptor->ChannelAssignment = dict->ul(MDD_IMFAudioChannelCfg_MCA);
			}

			if(ASDCP_SUCCESS(result)) {
				result = writer.OpenWrite(output_file.absoluteFilePath().toStdString(), mWriterInfo, essence_descriptor, mca_config, audio_descriptor.EditRate);
			}

			if(ASDCP_SUCCESS(result)) {
				result = parser.Reset();
				unsigned int duration = 0;
				for(ui32_t frame_num = 0; ASDCP_SUCCESS(result); frame_num++) {
/*					if(frame_num >= audio_descriptor.ContainerDuration) {
						result = RESULT_ENDOFFILE; // We mustn't wrap the WAV footer.
						break;
					*/
					if(QThread::currentThread()->isInterruptionRequested()) {
						error = Error(Error::WorkerInterruptionRequest);
						writer.Finalize();
						QFile::remove(output_file.absoluteFilePath());
						return error;
					}
					result = parser.ReadFrame(buffer);
					if(ASDCP_SUCCESS(result)) {
						result = writer.WriteFrame(buffer);
						progress = frame_num * 100 / audio_descriptor.ContainerDuration;
						if(progress != last_progress) emit Progress(progress);
						last_progress = progress;
					}
				}
				if(result == RESULT_ENDOFFILE) result = RESULT_OK;
			}
			if(ASDCP_SUCCESS(result)) result = writer.Finalize();
			else {
				error = Error(result);
				writer.Finalize();
				QFile::remove(output_file.absoluteFilePath());
			}
		}
		else error = Error(Error::SoundfieldGroupIncomplete, mSoundFieldGoup.GetAsString());
	}
	else error = Error(Error::SourceFilesMissing);
	return error;
}




JobWrapTimedText::JobWrapTimedText(const QStringList &rSourceFiles, const QString &rOutputFile, const EditRate &rEditRate, const Duration &rDuration, const QUuid &rAssetId, const QString &rProfile, const QString &rLanguageTag) :
AbstractJob(tr("Wrapping %1").arg(QFileInfo(rOutputFile).fileName())), mOutputFile(rOutputFile), mSourceFiles(rSourceFiles), mEditRate(rEditRate), mDuration(rDuration), mProfile(rProfile), mLanguageTag(rLanguageTag) {

	convert_uuid(rAssetId, (unsigned char*)mWriterInfo.AssetUUID);

}


Error JobWrapTimedText::Execute() {

	Error error;

	AS_02::TimedText::ST2052_TextParser  Parser;
	AS_02::TimedText::MXFWriter Writer;
	TimedText::FrameBuffer buffer;
	AS_02::TimedText::TimedTextDescriptor TDesc;
	ASDCP::MXF::TimedTextDescriptor *essence_descriptor = NULL;

	QFileInfo output_file(mOutputFile);
	QFileInfo file_info(mSourceFiles.first());

	//We need to change the working directory to resolve the ancillary resources!
	QString dirCopyPath = QDir::currentPath();
	QDir::setCurrent(file_info.absolutePath());

	Result_t result = Parser.OpenRead(file_info.absoluteFilePath().toStdString());
	result = Parser.FillTimedTextDescriptor(TDesc);

	TDesc.EditRate = ASDCP::Rational(mEditRate.GetNumerator(), mEditRate.GetDenominator());
	//WR
	TDesc.ContainerDuration = mDuration.GetCount();
	//WR
	TDesc.NamespaceName = mProfile.toStdString();
	Kumu::GenRandomUUID(TDesc.AssetID);
	buffer.Capacity(4*Kumu::Megabyte);

	ASDCP::MXF::InterchangeObject* tmp_obj = NULL;

	result = Writer.OpenWrite(output_file.absoluteFilePath().toStdString(), mWriterInfo, TDesc);

	if(ASDCP_SUCCESS(result)){
		//WR RFC5646LanguageTagList is available only in ASDCP::MXF::TimedTextDescriptor, not in AS_02::TimedText::TimedTextDescriptor
		result = Writer.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_TimedTextDescriptor), &tmp_obj);

		if(KM_SUCCESS(result)) {
			essence_descriptor = dynamic_cast<ASDCP::MXF::TimedTextDescriptor*>(tmp_obj);
			if(essence_descriptor == NULL) {
				error = Error(Error::UnsupportedEssence);
				return error;
			}
		}
		if (!mLanguageTag.isEmpty()) {
			essence_descriptor->RFC5646LanguageTagList.set(mLanguageTag.toStdString());
		}
		//WR
		std::string XMLDoc;
		AS_02::TimedText::ResourceList_t::const_iterator ri;

		result = Parser.ReadTimedTextResource(XMLDoc);
		if(ASDCP_SUCCESS(result)){
			result = Writer.WriteTimedTextResource(XMLDoc);

			if(ASDCP_SUCCESS(result)){

				for (ri = TDesc.ResourceList.begin(); ri != TDesc.ResourceList.end() && ASDCP_SUCCESS(result); ri++){

					result = Parser.ReadAncillaryResource((*ri).ResourceID, buffer);

					if(ASDCP_SUCCESS(result)){

						result = Writer.WriteAncillaryResource(buffer);
						if(ASDCP_FAILURE(result)) error = Error(result);
					}
					else { error = Error(result); QFile::remove(output_file.absoluteFilePath()); }
				}
			}
			else { error = Error(result); QFile::remove(output_file.absoluteFilePath()); }
		}
		else { error = Error(result); QFile::remove(output_file.absoluteFilePath()); }
	}
	else error = Error(result);

	result = Writer.Finalize();
	if(ASDCP_FAILURE(result)) { error = Error(result); QFile::remove(output_file.absoluteFilePath()); }

	//return to default working directory
	QDir::setCurrent(dirCopyPath);

	return error;
}
//WR

JobExtractEssenceDescriptor::JobExtractEssenceDescriptor(const QString &rSourceFile) :
AbstractJob(tr("Extracting Essence Descriptor from: %1").arg(QFileInfo(rSourceFile).fileName())), mSourceFile(rSourceFile) {

}


Error JobExtractEssenceDescriptor::Execute() {

	QFile file(mSourceFile);
	if(file.open(QIODevice::ReadOnly) == false) {
		return Error(Error::SourceFileOpenError, file.fileName());
	}
	Error error;

	QList<QString> dicts_fname = QList<QString>()
/*		<< "www-smpte-ra-org-reg-335-2012-13-1-amwa-as12.xml"
		<< "www-smpte-ra-org-reg-335-2012-13-1-amwa-rules.xml"
		<< "www-smpte-ra-org-reg-335-2012-13-4-archive.xml"
		<< "www-smpte-ra-org-reg-335-2012-13-12-as11.xml"
		<< "www-smpte-ra-org-reg-335-2012-13-13.xml"
		<< "www-smpte-ra-org-reg-395-2014.xml"*/
		<< "www-smpte-ra-org-reg-395-2014-13-1-aaf.xml"
/*		<< "www-smpte-ra-org-reg-395-2014-13-1-amwa-as10.xml"
		<< "www-smpte-ra-org-reg-395-2014-13-1-amwa-as11.xml"
		<< "www-smpte-ra-org-reg-395-2014-13-1-amwa-as12.xml"
		<< "www-smpte-ra-org-reg-395-2014-13-1-amwa-as-common.xml"
		<< "www-smpte-ra-org-reg-395-2014-13-4-archive.xml"
		<< "www-smpte-ra-org-reg-395-2014-13-12-as11.xml"
		<< "www-smpte-ra-org-reg-395-2014-13-13.xml"*/
		<< "www-smpte-ra-org-reg-2003-2012.xml"
/*		<< "www-smpte-ra-org-reg-2003-2012-13-1-amwa-as11.xml"
		<< "www-smpte-ra-org-reg-2003-2012-13-1-amwa-as12.xml"
		<< "www-smpte-ra-org-reg-2003-2012-13-4-archive.xml"
		<< "www-smpte-ra-org-reg-2003-2012-13-12-as11.xml"
		<< "www-ebu-ch-metadata-schemas-ebucore-smpte-class13-element.xml"
		<< "www-ebu-ch-metadata-schemas-ebucore-smpte-class13-group.xml"
		<< "www-ebu-ch-metadata-schemas-ebucore-smpte-class13-type.xml" */
		<< "www-smpte-ra-org-reg-335-2012.xml"
		<< "www-smpte-ra-org-reg-335-2012-13-1-aaf.xml"
//		<< "www-smpte-ra-org-reg-335-2012-13-1-amwa-as10.xml"
//		<< "www-smpte-ra-org-reg-335-2012-13-1-amwa-as11.xml"
		;

	XMLPlatformUtils::Initialize();

	XercesDOMParser *parser = new XercesDOMParser();

	parser->setDoNamespaces(true);

	MetaDictionaryCollection mds;

	for (int i = 0; i < dicts_fname.size(); i++) {

		QString dict_path = QApplication::applicationDirPath() + QString("/regxmllib/") + dicts_fname[i];
		parser->parse(dict_path.toStdString().c_str());
		DOMDocument *doc = parser->getDocument();
		if (doc) {
			MetaDictionary *md = new MetaDictionary();
			XMLImporter::fromDOM(*doc, *md);
			mds.addDictionary(md);
		} else {
			qDebug() << "Meta Dictionary " << dict_path << " not found!";
			return Error(Error::MetaDictionaryOpenError, QString("Couldn't open file %1").arg(dict_path));
		}
	}

	XMLCh tempStr[3] = { chLatin_L, chLatin_S, chNull };
	DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(tempStr);
	DOMLSOutput       *output = ((DOMImplementationLS*)impl)->createLSOutput();

	DOMDocument *doc = impl->createDocument();

	std::ifstream f(mSourceFile.toStdString().c_str(), std::ifstream::in | std::ifstream::binary);

	if (!f.good()) {
		qDebug() << "Can't read file:" << mSourceFile;
		error = Error(Error::EssenceDescriptorExtraction);

	}
	static const rxml::UL ESSENCE_DESCRIPTOR_KEY = "urn:smpte:ul:060e2b34.02010101.0d010101.01012400"; // Generic Descriptor per SMPTE ST 377-1 Table 19
	const rxml::AUID* ed_auid = new rxml::AUID(ESSENCE_DESCRIPTOR_KEY);
	DOMDocumentFragment* frag = MXFFragmentBuilder::fromInputStream(f, mds, NULL, ed_auid, *doc);

	if (frag)
		doc->appendChild(frag);
	else {
		qDebug() << "Can't find ESSENCE_DESCRIPTOR_KEY" << "urn:smpte:ul:060e2b34.02010101.0d010101.01012400";
		error = Error(Error::EssenceDescriptorExtraction);
	}

	if(error.IsError() == false) {
		emit Result(doc, GetIdentifier());
	}

	//doc->release();


	/* free heap */

	for (std::map<std::string, MetaDictionary*>::const_iterator it = mds.getDictionatries().begin();
		it != mds.getDictionatries().end();
		it++) {
		delete it->second;
	}
	XMLPlatformUtils::Terminate();
	return error;
}
//WR


JobCallPhoton::JobCallPhoton(const QString &rWorkingDirectory, WidgetImpBrowser* &rWidgetImpBrowser) :
AbstractJob("Generating Photon IMP QC Report"), mWorkingDirectory(rWorkingDirectory),  mWidgetImpBrowser(rWidgetImpBrowser){

}

Error JobCallPhoton::Execute() {

	Error error;
	QString qresult;

	//Figure out IMF App
	QString appString = "app2or2E";
	if (mWidgetImpBrowser && mWidgetImpBrowser->GetImfPackage()) {
		QVector<QString> appList = mWidgetImpBrowser->GetImfPackage().data()->GetApplicationIdentificationList();
		if ( appList.contains("http://www.smpte-ra.org/ns/2067-50/2017") ) appString = "app5";
	}
	QProcess *myProcess = new QProcess();
	const QString program = "java";
	QStringList arg;
	arg << "-cp";
	QString lib_dir = QString("/photon/build/libs/*");
	if (appString == "app5") {
		lib_dir = QString("/photon/build/libs-app5/*");
	}
#ifdef WIN32
	arg << QApplication::applicationDirPath() + lib_dir + QString(";");
#else
	arg << QApplication::applicationDirPath() + lib_dir + QString(":");
#endif
	arg << "com.netflix.imflibrary.app.IMPAnalyzer";
	arg << mWorkingDirectory;
	if (appString == "app5") {
		arg << "--application";
		arg << appString;
	}
	emit Progress(20);
	myProcess->start(program, arg);
	emit Progress(40);
	myProcess->waitForFinished(-1);
	if (myProcess->exitStatus() == QProcess::NormalExit) {
		if (myProcess->exitCode() != 0) { return error = Error(Error::ExitCodeNotZero); }
	} else {
		return error = Error(Error::ExitStatusError);
	}
	emit Progress(80);
	try {
		qresult = myProcess->readAllStandardOutput();
	}
	catch (...) {
		return error = Error(Error::PhotonQcReport);
	}

	if(error.IsError() == false) {
		emit Result(qresult, GetIdentifier());
	}

	return error;
}

JobCreateScm::JobCreateScm(const QSharedPointer<AssetScm> rAssetScm) :
AbstractJob("Generating Sidecar Composition Map"), mAssetScm(rAssetScm) {

}

Error JobCreateScm::Execute() {

	Error error = Error::None;
	xml_schema::NamespaceInfomap scm_namespace;
	scm_namespace[""].name = XML_NAMESPACE_SCM;
	scm_namespace["scm"].name = XML_NAMESPACE_SCM;
	scm_namespace["dcml"].name = XML_NAMESPACE_DCML;
	scm_namespace["ds"].name = XML_NAMESPACE_DS;
	scm_namespace["xs"].name = XML_NAMESPACE_XS;

	scm::SidecarCompositionMapType scm = mAssetScm->WriteScm();
	// Create a scm::SidecarCompositionMapType instance
	std::auto_ptr< scm::SidecarCompositionMapType> scm_data;
	std::auto_ptr< scm::SidecarCompositionMapType_PropertiesType> scm_properties;

	QString destination(mAssetScm->GetPath().absoluteFilePath());
	if(destination.isEmpty() == false) {
		XmlSerializationError serialization_error;
		std::ofstream scm_ofs(destination.toStdString().c_str(), std::ofstream::out);
		try {
			scm::serializeSidecarCompositionMap(scm_ofs, scm, scm_namespace, "UTF-8", xml_schema::Flags::dont_initialize );
		}
		catch(xml_schema::Serialization &e) { serialization_error = XmlSerializationError(e); }
		catch(xml_schema::UnexpectedElement &e) { serialization_error = XmlSerializationError(e); }
		catch(xml_schema::NoTypeInfo &e) { serialization_error = XmlSerializationError(e); }
		catch(...) { serialization_error = XmlSerializationError(XmlSerializationError::Unknown); }
		scm_ofs.close();
		if(serialization_error.IsError() == true) {
			qDebug() << serialization_error;
			error = Error::Unknown;
		} else {
		if (ImfXmlHelper::RemoveWhiteSpaces(destination))
			qDebug() << "Error removing XML whitespaces from " << destination;
		}
	}
	else {
		error = Error::Unknown;
	}
	if(!error.IsError()) {
		//if(mAssetCpl) mAssetCpl->FileModified();
		//WR begin
		//if(mAssetCpl) mAssetCpl->SetIsNewOrModified(true);
		//WR end
	}
	return error;
}

#ifdef APP5_ACES
JobExtractTargetFrames::JobExtractTargetFrames(const QSharedPointer<AssetMxfTrack> rAssetMxf) :
AbstractJob("Extracting Target Frames"), mAssetMxf(rAssetMxf) {

}

Error JobExtractTargetFrames::Execute() {
	QStringList result_list;
	Error error = Error::None;

	try {
		QTemporaryDir dir;
		if (dir.isValid()) {
			//TODO Figure out size of ancillary resource from RIP
			AS_02::ACES::FrameBuffer FrameBuffer(30000000);
			AS_02::ACES::ResourceList_t resource_list_t;
			AS_02::ACES::MXFReader Reader;
			Result_t result = Reader.OpenRead(mAssetMxf->GetPath().absoluteFilePath().toStdString()); // open file for reading
			if (ASDCP_SUCCESS(result)) {
				result = Reader.FillAncillaryResourceList(resource_list_t);
				AS_02::ACES::ResourceList_t::iterator it;
				for (it = resource_list_t.begin(); it != resource_list_t.end(); it++) {
					//ASDCP::MXF::RIP& rip = Reader.RIP();
					ASDCP::UUID resource_id;
					resource_id.Set(it->ResourceID);
					result = Reader.ReadAncillaryResource(resource_id, FrameBuffer);
					if (!ASDCP_SUCCESS(result)) qDebug() << "ReadAncillaryResource failed!" << result.Value();

					QString filename;
					char buf[64];
					resource_id.EncodeString(buf, 64);
					QString extension;
					switch (it->Type) {
					case AS_02::ACES::MT_PNG:
						extension = "png";
						break;
					case AS_02::ACES::MT_TIFF:
						extension = "tif";
						break;
					default:
						break;
					}
					QFileInfo file_path = QFileInfo(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
					if (!file_path.isDir() || !file_path.isWritable()) {
					  error = Error::Unknown;
					  Reader.Close();
					  return error;
					}
					file_path = QFileInfo(file_path.absoluteFilePath() + "/" + "TargetFrame_" + QString(buf) + "." + extension);
					filename = file_path.absoluteFilePath();

					if (ASDCP_SUCCESS(result)) {
						Kumu::FileWriter OutFile;
						ui32_t write_count;
						result = OutFile.OpenWrite(filename.toStdString());

						if (ASDCP_SUCCESS(result)) {
							result = OutFile.Write(FrameBuffer.Data(), FrameBuffer.Size(), &write_count);
							if (ASDCP_SUCCESS(result))
								result_list << filename;
						}
					}
				}
				Reader.Close();
			}
		} else {
			error = Error::Unknown;
		}
	}
	catch(...) { return Error::XMLSchemeError; }

	emit Result(result_list, GetIdentifier());

	return error;
}
#endif

JobDeliverySpecificationCheck::JobDeliverySpecificationCheck(const QSharedPointer<AssetCpl> rAssetCpl, const dsl::DeliverableType_CompositionPlaylistConstraintsType* rDslConstraints) :
AbstractJob("Checking CPL against a delivery specification"), mAssetCpl(rAssetCpl), mDslConstraints(rDslConstraints) {

}

Error JobDeliverySpecificationCheck::Execute() {
	QList<QStringList> result_list;
	//This QMap contains all EssenceDescriptos as listed in mAssetCpl
	QMap<QUuid, cpl2016::EssenceDescriptorBaseType> essence_descriptors;
	// These QMaps contain the TrackID and all SourceEncoding UUIDs of a given Virtual Track
	QMap<QUuid, QSet<QUuid> > image_tracks_sourceencodings;
	QMap<QUuid, QSet<QUuid> > audio_tracks_sourceencodings;
	QMap<QUuid, QSet<QUuid> > timed_text_tracks_sourceencodings;
	QMap<QUuid, QString> timed_text_tracks_sequence_type;

	std::auto_ptr< cpl2016::CompositionPlaylistType> cpl_data;
	// Only 2067-3:2016 CPLs are supported
	try { cpl_data = cpl2016::parseCompositionPlaylist(mAssetCpl->GetPath().absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize ); }
	catch(...) { return Error::XMLSchemeError; }
	/*
	 * General constraints
	 */
	result_list << (QStringList() << "0" << "CPL Id: " + mAssetCpl.data()->GetId().toString() << "PASS");
	/*
	 * Application Identification
	 */
	QStringList application_identification_cpl, application_identification_dsl;
	bool is_exclusive = false;
	if (cpl_data->getExtensionProperties().present()) {
		cpl2016::CompositionPlaylistType_ExtensionPropertiesType sequence_list = cpl_data->getExtensionProperties().get();
		cpl2016::CompositionPlaylistType_ExtensionPropertiesType::AnySequence &r_any_sequence(sequence_list.getAny());
		for(cpl2016::CompositionPlaylistType_ExtensionPropertiesType::AnySequence::iterator sequence_iter(r_any_sequence.begin()); sequence_iter != r_any_sequence.end(); ++sequence_iter) {
			if (QString(XMLString::transcode(sequence_iter->getNodeName())).contains("ApplicationIdentification")) {
				application_identification_cpl << QString(XMLString::transcode(sequence_iter->getTextContent()));
			}
		}
	}
	if (mDslConstraints->getApplicationIdentificationList().present()) {
		dsl::DeliverableType_CompositionPlaylistConstraintsType_ApplicationIdentificationListType app_id_list = mDslConstraints->getApplicationIdentificationList().get();
		dsl::DeliverableType_CompositionPlaylistConstraintsType_ApplicationIdentificationListType_ValueListType::ApplicationIdentificationSequence app_id_sequence = app_id_list.getValueList().getApplicationIdentification();
		is_exclusive = QString(app_id_list.getMatchType().c_str()).compare("inclusive");
		for (dsl::DeliverableType_CompositionPlaylistConstraintsType_ApplicationIdentificationListType_ValueListType::ApplicationIdentificationIterator
				iter(app_id_sequence.begin());
				iter != app_id_sequence.end(); iter++) {
			application_identification_dsl << QString(iter->c_str());
		}
		bool app_id_result = true;
		if (is_exclusive && (application_identification_cpl.size() != application_identification_dsl.size()))
			app_id_result = false;
		else {
			for (int i = 0; i < application_identification_dsl.size(); i++) {
				if (!application_identification_cpl.contains(application_identification_dsl.at(i))) {
					app_id_result = false;
				}
			}
		}
		if (app_id_result) {
			result_list << (QStringList() << "1" << "Application Identification" << "PASS" << tr("%1").arg(application_identification_cpl.join(",")));
		} else {
			result_list << (QStringList() << "1" << "Application Identification" << "FAIL" << tr("Expected: %1").arg(application_identification_dsl.join(",")) << tr("Actual: %1").arg(application_identification_cpl.join(",")));
		}
		if (result_list.at(0).at(2).compare("PASS") == 0)
			result_list[0][2] = (app_id_result ? "PASS": "FAIL");
	}

	/*
	 * Get Essence Descriptor List
	 */
	if (cpl_data->getEssenceDescriptorList().present()) {
		cpl2016::CompositionPlaylistType_EssenceDescriptorListType ed_list = cpl_data->getEssenceDescriptorList().get();
		cpl2016::CompositionPlaylistType_EssenceDescriptorListType::EssenceDescriptorSequence &ed_sequence = ed_list.getEssenceDescriptor();
		for (cpl2016::CompositionPlaylistType_EssenceDescriptorListType::EssenceDescriptorSequence::iterator sequence_iter(ed_sequence.begin()); sequence_iter != ed_sequence.end(); sequence_iter++) {
			cpl2016::EssenceDescriptorBaseType& desc(*sequence_iter);
			essence_descriptors.insert(ImfXmlHelper::Convert(desc.getId()), cpl2016::EssenceDescriptorBaseType(desc));
		}
	}

	/*
	 * Get Virtual Tracks from CPL
	 */

	for(unsigned int i = 0; i < cpl_data->getSegmentList().getSegment().size(); i++) {

		cpl2016::CompositionPlaylistType_SegmentListType::SegmentType *r_segment = new cpl2016::CompositionPlaylistType_SegmentListType::SegmentType(cpl_data->getSegmentList().getSegment().at(i));
		cpl2016::SegmentType::SequenceListType &r_sequence_list = r_segment->getSequenceList();
		cpl2016::SegmentType_SequenceListType::AnySequence &r_any_sequence(r_sequence_list.getAny());
		for(cpl2016::SegmentType_SequenceListType::AnySequence::iterator sequence_iter(r_any_sequence.begin()); sequence_iter != r_any_sequence.end(); ++sequence_iter) {
			xercesc::DOMElement& element(*sequence_iter);
			cpl2016::SequenceType sequence(element);
			std::string name(xsd::cxx::xml::transcode<char>(element.getLocalName()));
			std::string name_space(xsd::cxx::xml::transcode<char>(element.getNamespaceURI()));
			QUuid track_id = ImfXmlHelper::Convert(sequence.getTrackId());
			QStringList timed_text_sequence_types;
			timed_text_sequence_types << "SubtitlesSequence" << "HearingImpairedCaptionsSequence" << "VisuallyImpairedTextSequence" << "CommentarySequence" << "KaraokeSequence" << "SubtitlesSequence";

			if(name == "MainImageSequence") {
				// Extract all SourceEncoding values
				QSet<QUuid> source_encoding_ids;
				cpl2016::SequenceType::ResourceListType::ResourceSequence &resource_sequence = sequence.getResourceList().getResource();
				for (cpl2016::SequenceType::ResourceListType::ResourceSequence::iterator it = resource_sequence.begin(); it != resource_sequence.end(); it++ ) {
					cpl2016::TrackFileResourceType *p_file_resource = dynamic_cast<cpl2016::TrackFileResourceType*>(&(*it));
					if (p_file_resource) source_encoding_ids << ImfXmlHelper::Convert(p_file_resource->getSourceEncoding());
				}
				// Add all SourceEncoding UUIds
				if (image_tracks_sourceencodings.contains(track_id)) {
					QSet<QUuid> temp_list = image_tracks_sourceencodings.values(track_id).first();
					temp_list = temp_list.unite(source_encoding_ids);
					image_tracks_sourceencodings.remove(track_id);
					image_tracks_sourceencodings.insert(track_id, temp_list);
				} else {
					image_tracks_sourceencodings.insert(track_id, source_encoding_ids);
				}
			}
			else if(name == "MainAudioSequence") {
				// Extract all SourceEncoding values
				QSet<QUuid> source_encoding_ids;
				cpl2016::SequenceType::ResourceListType::ResourceSequence &resource_sequence = sequence.getResourceList().getResource();
				for (cpl2016::SequenceType::ResourceListType::ResourceSequence::iterator it = resource_sequence.begin(); it != resource_sequence.end(); it++ ) {
					cpl2016::TrackFileResourceType *p_file_resource = dynamic_cast<cpl2016::TrackFileResourceType*>(&(*it));
					if (p_file_resource) source_encoding_ids << ImfXmlHelper::Convert(p_file_resource->getSourceEncoding());
				}
				if (audio_tracks_sourceencodings.contains(track_id)) {
					// Add all SourceEncoding UUIds
					QSet<QUuid> temp_list = audio_tracks_sourceencodings.values(track_id).first();
					temp_list = temp_list.unite(source_encoding_ids);
					audio_tracks_sourceencodings.remove(track_id);
					audio_tracks_sourceencodings.insert(track_id, temp_list);
				} else {
					audio_tracks_sourceencodings.insert(track_id, source_encoding_ids);
				}
			}
			else if(timed_text_sequence_types.contains(QString(name.c_str()))) {
				// Record Sequence Type
				if (!timed_text_tracks_sequence_type.contains(track_id)) timed_text_tracks_sequence_type.insert(track_id, QString(name.c_str()));
				// Extract all SourceEncoding values
				QSet<QUuid> source_encoding_ids;
				cpl2016::SequenceType::ResourceListType::ResourceSequence &resource_sequence = sequence.getResourceList().getResource();
				for (cpl2016::SequenceType::ResourceListType::ResourceSequence::iterator it = resource_sequence.begin(); it != resource_sequence.end(); it++ ) {
					cpl2016::TrackFileResourceType *p_file_resource = dynamic_cast<cpl2016::TrackFileResourceType*>(&(*it));
					if (p_file_resource) source_encoding_ids << ImfXmlHelper::Convert(p_file_resource->getSourceEncoding());
				}
				if (timed_text_tracks_sourceencodings.contains(track_id)) {
					// Add all SourceEncoding UUIds
					QSet<QUuid> temp_list = timed_text_tracks_sourceencodings.values(track_id).first();
					temp_list = temp_list.unite(source_encoding_ids);
					timed_text_tracks_sourceencodings.remove(track_id);
					timed_text_tracks_sourceencodings.insert(track_id, temp_list);
				} else {
					timed_text_tracks_sourceencodings.insert(track_id, source_encoding_ids);
				}
			}
		}
	}

	/*
	 * Check virtual tracks against delivery specification
	 */

	const dsl::DeliverableType_CompositionPlaylistConstraintsType_VirtualTrackListType& vtl = mDslConstraints->getVirtualTrackList();
	const dsl::DeliverableType_CompositionPlaylistConstraintsType_VirtualTrackListType::TimedTextVirtualTrackSequence& vt_timed_text_sequence = vtl.getTimedTextVirtualTrack();
	const dsl::DeliverableType_CompositionPlaylistConstraintsType_VirtualTrackListType::MarkerVirtualTrackType& vt_marker_sequence = vtl.getMarkerVirtualTrack().get();

	/* ###################
	 * IMAGE VIRTUAL TRACK
	 */
	if (vtl.getImageVirtualTrack().present()) {
		const dsl::DeliverableType_CompositionPlaylistConstraintsType_VirtualTrackListType::ImageVirtualTrackType& vt_image = vtl.getImageVirtualTrack().get();
		QList<QStringList> track_result_list;
		track_result_list << (QStringList() << "1" << "Main Image Virtual Track" << "");
		enum SMPTE::eJ2K_Profiles essence_encoding_enum_j2k = SMPTE::eJ2K_Profiles::J2K_Profiles_UNKNOWN;
		enum SMPTE::eACES_Profiles essence_encoding_enum_aces = SMPTE::eACES_Profiles::ACES_Profiles_UNKNOWN;
		enum SMPTE::eProRes_Profiles essence_encoding_enum_prores = SMPTE::eProRes_Profiles::ProRes_Profiles_UNKNOWN;
		QMap<enum SMPTE::eJ2K_Profiles, QString>::iterator profile_it_j2k = SMPTE::vJ2K_Profiles.begin();
		QString image_essence_encoding_string, image_essence_encoding_ul;
		bool essence_encoding_profile_found = false;
		while ( !essence_encoding_profile_found && (profile_it_j2k != SMPTE::vJ2K_Profiles.end())) {
			if (profile_it_j2k.value() == QString(vt_image.getEssenceEncoding().c_str())) {
				essence_encoding_enum_j2k = profile_it_j2k.key();
				image_essence_encoding_ul = SMPTE::J2K_ProfilesMapInverse.values(essence_encoding_enum_j2k).first();
				image_essence_encoding_string = SMPTE::vJ2K_Profiles.value(essence_encoding_enum_j2k);
				essence_encoding_profile_found = true;
			}
			profile_it_j2k++;
		}
		QMap<enum SMPTE::eACES_Profiles, QString>::iterator profile_it_aces = SMPTE::vACES_Profiles.begin();
		while ( !essence_encoding_profile_found && (profile_it_aces != SMPTE::vACES_Profiles.end())) {
			if (profile_it_aces.value() == QString(vt_image.getEssenceEncoding().c_str())) {
				essence_encoding_enum_aces = profile_it_aces.key();
				image_essence_encoding_ul = SMPTE::ACES_ProfilesMapInverse.values(essence_encoding_enum_aces).first();
				image_essence_encoding_string = SMPTE::vACES_Profiles.value(essence_encoding_enum_aces);
				essence_encoding_profile_found = true;
			}
			profile_it_aces++;
		}
		QMap<enum SMPTE::eProRes_Profiles, QString>::iterator profile_it_prores = SMPTE::vProRes_Profiles.begin();
		while ( !essence_encoding_profile_found && (profile_it_prores != SMPTE::vProRes_Profiles.end())) {
			if (profile_it_prores.value() == QString(vt_image.getEssenceEncoding().c_str())) {
				essence_encoding_enum_prores = profile_it_prores.key();
				image_essence_encoding_ul = SMPTE::ProRes_ProfilesMapInverse.values(essence_encoding_enum_prores).first();
				image_essence_encoding_string = SMPTE::vProRes_Profiles.value(essence_encoding_enum_prores);
				essence_encoding_profile_found = true;
			}
			profile_it_prores++;
		}
		if (!essence_encoding_profile_found) {
			track_result_list << (QStringList() << "2" << "EssenceEncoding" << "FAIL" <<  tr("Unknown: %1").arg(QString(vt_image.getEssenceEncoding().c_str())));
		} else {
		// EssenceCoding in DSL is valid
			// Get DSL requirements
			QString TransferCharacteristic_UL, CodingEquations_UL, ColorPrimaries_UL;
			if (vt_image.getColorimetry().present()) {
				QString colorimetry = vt_image.getColorimetry().get().c_str();
				if (colorimetry == "COLOR.1") {
					TransferCharacteristic_UL = SMPTE::TransferCharacteristicMapInverse.value(SMPTE::TransferCharacteristic_ITU1361);
					CodingEquations_UL = SMPTE::CodingEquationsMapInverse.value(SMPTE::CodingEquations_ITU601);
					ColorPrimaries_UL = SMPTE::ColorPrimariesMapInverse.value(SMPTE::ColorPrimaries_SMPTE170M);
				} else if (colorimetry == "COLOR.2") {
					TransferCharacteristic_UL = SMPTE::TransferCharacteristicMapInverse.value(SMPTE::TransferCharacteristic_ITU1361);
					CodingEquations_UL = SMPTE::CodingEquationsMapInverse.value(SMPTE::CodingEquations_ITU601);
					ColorPrimaries_UL = SMPTE::ColorPrimariesMapInverse.value(SMPTE::ColorPrimaries_ITU470_PAL);
				} else if (colorimetry == "COLOR.3") {
					TransferCharacteristic_UL = SMPTE::TransferCharacteristicMapInverse.value(SMPTE::TransferCharacteristic_ITU709);
					CodingEquations_UL = SMPTE::CodingEquationsMapInverse.value(SMPTE::CodingEquations_ITU709);
					ColorPrimaries_UL = SMPTE::ColorPrimariesMapInverse.value(SMPTE::ColorPrimaries_ITU709);
				} else if (colorimetry == "COLOR.4") {
					TransferCharacteristic_UL = SMPTE::TransferCharacteristicMapInverse.value(SMPTE::TransferCharacteristic_IEC6196624_xvYCC);
					CodingEquations_UL = SMPTE::CodingEquationsMapInverse.value(SMPTE::CodingEquations_ITU709);
					ColorPrimaries_UL = SMPTE::ColorPrimariesMapInverse.value(SMPTE::ColorPrimaries_ITU709);
				} else if (colorimetry == "COLOR.5") {
					TransferCharacteristic_UL = SMPTE::TransferCharacteristicMapInverse.value(SMPTE::TransferCharacteristic_ITU2020);
					CodingEquations_UL = SMPTE::CodingEquationsMapInverse.value(SMPTE::CodingEquations_ITU2020_NCL);
					ColorPrimaries_UL = SMPTE::ColorPrimariesMapInverse.value(SMPTE::ColorPrimaries_ITU2020);
				} else if (colorimetry == "COLOR.6") {
					TransferCharacteristic_UL = SMPTE::TransferCharacteristicMapInverse.value(SMPTE::TransferCharacteristic_SMPTEST2084);
					CodingEquations_UL = "CodingEquations_UNKNOWN";
					ColorPrimaries_UL = SMPTE::ColorPrimariesMapInverse.value(SMPTE::ColorPrimaries_P3D65);
				} else if (colorimetry == "COLOR.7") {
					TransferCharacteristic_UL = SMPTE::TransferCharacteristicMapInverse.value(SMPTE::TransferCharacteristic_SMPTEST2084);
					CodingEquations_UL = SMPTE::CodingEquationsMapInverse.value(SMPTE::CodingEquations_ITU2020_NCL);
					ColorPrimaries_UL = SMPTE::ColorPrimariesMapInverse.value(SMPTE::ColorPrimaries_ITU2020);
				} else if (colorimetry == "COLOR.APP5.AP0") {
					TransferCharacteristic_UL = SMPTE::TransferCharacteristicMapInverse.value(SMPTE::TransferCharacteristic_linear);
					CodingEquations_UL = SMPTE::CodingEquationsMapInverse.value(SMPTE::CodingEquations_UNKNOWN);
					ColorPrimaries_UL = SMPTE::ColorPrimariesMapInverse.value(SMPTE::ColorPrimaries_ACES);
				}
			}

			//QUuid image_track_id = image_track_ids.values().first();
			//Loop all source encodings and check values against delivery specification
			QSet<QUuid> source_encoding_ids = image_tracks_sourceencodings.first();
			for (QSet<QUuid>::iterator source_encoding_id = source_encoding_ids.begin(); source_encoding_id != source_encoding_ids.end(); source_encoding_id++) {
				cpl2016::EssenceDescriptorBaseType ed = essence_descriptors.values(*source_encoding_id).first();
				cpl2016::EssenceDescriptorBaseType::AnySequence any_sequence = ed.getAny();
				DOMElement& dom_element(any_sequence.front());
				int cpl_component_depth = 0;
				int cpl_component_max_ref = -1;
				bool cpl_component_max_ref_extracted = false; // This is to mitigate a bug in Xerces 3.1.3 that leads to a crash when getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ComponentMaxRef") is called twice

				/*
				 * EssenceEncoding
				 */
				DOMNodeList* node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("PictureCompression"));
				QStringList result;
				if (node_list->getLength() != 1)
					result << (QStringList() << "2" << "EssenceEncoding" << "FAIL" << "Ambiguous or missing PictureCompression element");
				else {
					QString actual_value = QString(XMLString::transcode(node_list->item(0)->getTextContent())).replace("urn:smpte:ul:","");
					if ( actual_value == image_essence_encoding_ul) {
						result << (QStringList() << "2" << "EssenceEncoding" << "PASS" << image_essence_encoding_string);
					} else {
						result << "2" << "EssenceEncoding" << "FAIL"
								<< tr("Expected: %1").arg(image_essence_encoding_string);
						if (SMPTE::J2K_ProfilesMap.contains(actual_value))
							result << tr("Actual: %1").arg(SMPTE::vJ2K_Profiles.value(SMPTE::J2K_ProfilesMap.values(actual_value).first()));
						else if (SMPTE::ACES_ProfilesMap.contains(actual_value))
							result << tr("Actual: %1").arg(SMPTE::vACES_Profiles.value(SMPTE::ACES_ProfilesMap.values(actual_value).first()));
						else if (SMPTE::ProRes_ProfilesMap.contains(actual_value))
							result << tr("Actual: %1").arg(SMPTE::vProRes_Profiles.value(SMPTE::ProRes_ProfilesMap.values(actual_value).first()));
						else
							result << tr("Actual: INVALID UL: %1").arg(actual_value);
					}
				}
				if (!track_result_list.contains(result)) track_result_list << result;
				result.clear();
				node_list->~DOMNodeList();
				/*
				 * Colorimetry
				 */
				bool colorimetry_pass = true;
				if (vt_image.getColorimetry().present()) {
				// Check Colorimetry - TransferCharacteristic
					DOMNodeList* node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("TransferCharacteristic"));
					if (node_list->getLength() != 1) {
						QStringList result;
						result << (QStringList() << "2" << "Colorimetry" << "FAIL" << "Ambiguous or missing TransferCharacteristic element");
						if (!track_result_list.contains(result)) track_result_list << result;
						colorimetry_pass = false;
					} else {
						QString actual = QString(XMLString::transcode(node_list->item(0)->getTextContent())).replace("urn:smpte:ul:", "");
						if ( actual != TransferCharacteristic_UL) {
							QStringList result;
							colorimetry_pass = false;
							result << (QStringList() << "2" << "Colorimetry" << "FAIL" << "Expected TransferCharacteristic: " + SMPTE::vTransferCharacteristic.value(SMPTE::TransferCharacteristicMap.value(TransferCharacteristic_UL))
							<< "Actual TransferCharacteristic: "+ SMPTE::vTransferCharacteristic.value(SMPTE::TransferCharacteristicMap.value(actual)) );
							if (!track_result_list.contains(result)) track_result_list << result;
						}
					}
					node_list->~DOMNodeList();
				// Check Colorimetry - CodingEquations
					node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("CodingEquations"));
					if (CodingEquations_UL != SMPTE::CodingEquationsMapInverse.value(SMPTE::CodingEquations_UNKNOWN)) { // Coding Equations don't matter for P3 or ACES
						QStringList result;
						if (node_list->getLength() != 1) {
							result << (QStringList() << "2" << "Colorimetry" << "FAIL" << "Ambiguous or missing CodingEquations element");
							colorimetry_pass = false;
							if (!track_result_list.contains(result)) track_result_list << result;
						} else {
							QString actual = QString(XMLString::transcode(node_list->item(0)->getTextContent())).replace("urn:smpte:ul:", "");
							if (actual != CodingEquations_UL) {
								colorimetry_pass = false;
								result << (QStringList() << "2" << "Colorimetry" << "FAIL"
										<< tr("Expected CodingEquations: %1").arg(SMPTE::vCodingEquations.value(SMPTE::CodingEquationsMap.value(CodingEquations_UL)))
										<< tr("ActualCodingEquations: %1").arg(SMPTE::vCodingEquations.value(SMPTE::CodingEquationsMap.value(actual))) );
								if (!track_result_list.contains(result)) track_result_list << result;
							}
						}
					}
					node_list->~DOMNodeList();
				// Check Colorimetry - ColorPrimaries_UL
					node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ColorPrimaries"));
					if (node_list->getLength() != 1) {
						result << (QStringList() << "2" << "Colorimetry" << "FAIL" << "Ambiguous or missing ColorPrimaries element");
						if (!track_result_list.contains(result)) track_result_list << result;
						colorimetry_pass = false;
					} else {
						QString actual = QString(XMLString::transcode(node_list->item(0)->getTextContent())).replace("urn:smpte:ul:", "");
						if (actual != ColorPrimaries_UL) {
							result << (QStringList() << "2" << "Colorimetry" << "FAIL" << "Expected ColorPrimaries: " + SMPTE::vColorPrimaries.value(SMPTE::ColorPrimariesMap.value(ColorPrimaries_UL))
									<< "Actual ColorPrimaries " + SMPTE::vColorPrimaries.value(SMPTE::ColorPrimariesMap.value(actual)) );
							colorimetry_pass = false;
							if (!track_result_list.contains(result)) track_result_list << result;
						}
					}
					node_list->~DOMNodeList();
					result.clear();
					if (colorimetry_pass) {
						result << (QStringList() << "2" << "Colorimetry" << "PASS" << SMPTE::vColorPrimaries.value(SMPTE::ColorPrimariesMap.value(ColorPrimaries_UL)));
						if (!track_result_list.contains(result)) track_result_list << result;
					}
				}
				/*
				 * Sampling
				 */
				QString sampling = QString(vt_image.getSampling().get().c_str());
				if (vt_image.getSampling().present()) {
					int cpl_horizontal_subsampling = 0;
					int cpl_vertical_subsampling = 0;
					bool sampling_pass = false;
					DOMNodeList* node_list;
					if (QString(XMLString::transcode(dom_element.getNodeName())).contains("CDCIDescriptor")) {
						node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("HorizontalSubsampling"));
						if (node_list->getLength() == 1) {
							cpl_horizontal_subsampling = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
						}
						node_list->~DOMNodeList();
						node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("VerticalSubsampling"));
						if (node_list->getLength() == 1) {
							cpl_vertical_subsampling = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
						}
						if (sampling == "4:2:2") {
							sampling_pass = ((cpl_horizontal_subsampling == 2) && (cpl_vertical_subsampling == 1));
						} else if (sampling == "4:4:4") {
							sampling_pass = ((cpl_horizontal_subsampling == 1) && (cpl_vertical_subsampling == 1));
						}
						node_list->~DOMNodeList();
					} else { // Assuming RGBADescriptor
						if (sampling == "4:4:4") sampling_pass = true;
					}
					QStringList result;
					if (sampling_pass) {
						result << (QStringList() << "2" << "Sampling" << "PASS" << sampling);
					} else {
						result << (QStringList() << "2" << "Sampling" << "FAIL" << "Expected Sampling "+ sampling);
					}
					if (!track_result_list.contains(result)) track_result_list << result;
											}
				/*
				 * Quantization
				 */
				if (vt_image.getQuantization().present()) {
				// Check Sampling
					QString quantization = QString(vt_image.getQuantization().get().c_str());
					bool quantization_pass = true;
					bool error = false;
					DOMNodeList* node_list;
					if (QString(XMLString::transcode(dom_element.getNodeName())).contains("CDCIDescriptor")) {
						int black_ref=0, white_ref=0, range=0;
						if (quantization == "QE.1") {
							node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ComponentDepth"));
							if (node_list->getLength() == 1) {
								cpl_component_depth = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
								black_ref = 16 << (cpl_component_depth - 8);
								white_ref = black_ref + (219 << (cpl_component_depth - 8));
								range =  (240 << (cpl_component_depth - 8)) - black_ref + 1;
								// Exception: COLOR.4 (xvYCC)
								if (vt_image.getColorimetry().present())
									if (QString(vt_image.getColorimetry().get().c_str()) == "COLOR.4")
										// No algorithm for these color range values ? Zero means unsupported bit depth
										range = (cpl_component_depth == 8 ? 254 : (cpl_component_depth == 10 ? 1013 : 0));
							}
							node_list->~DOMNodeList();
							node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("BlackRefLevel"));
							if (node_list->getLength() == 1) {
								int actual_value = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
								if (black_ref != actual_value) {
									QStringList result;
									result << (QStringList() << "2" << "Quantization" << "FAIL" << tr("Expected BlackRefLevel: %1").arg(black_ref) << tr("Actual BlackRefLevel: %1").arg(actual_value) );
									if (!track_result_list.contains(result)) track_result_list << result;
									quantization_pass = false;
								}
							}
							node_list->~DOMNodeList();
							node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("WhiteRefLevel"));
							if (node_list->getLength() == 1) {
								int actual_value = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
								if (white_ref != actual_value) {
									QStringList result;
									result << (QStringList() << "2" << "Quantization" << "FAIL" << tr("Expected WhiteRefLevel: %1").arg(white_ref) << tr("Actual WhiteRefLevel: %1").arg(actual_value) );
									if (!track_result_list.contains(result)) track_result_list << result;
									quantization_pass = false;
								}
							}
							node_list->~DOMNodeList();
							node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ColorRange"));
							if (node_list->getLength() == 1) {
								int actual_value = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
								if (range != actual_value) {
									QStringList result;
									result << (QStringList() << "2" << "Quantization" << "FAIL" << tr("Expected ColorRange: %1").arg(range) << tr("Actual ColorRange: %1").arg(actual_value) );
									if (!track_result_list.contains(result)) track_result_list << result;
									quantization_pass = false;
								}
							}
							node_list->~DOMNodeList();
						} else {
							quantization_pass = false;
							QStringList result;
							result << (QStringList() << "2" << "Quantization" << "FAIL" << "Quantization " + quantization + " is not allowed for YCbCr");
							if (!track_result_list.contains(result)) track_result_list << result;
						}
					} else if (QString(XMLString::transcode(dom_element.getNodeName())).contains("RGBADescriptor")) {
						int component_depth = 0, component_min_ref = -1, component_max_ref = -1;
						node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ComponentMaxRef"));
						if (node_list->getLength() == 1) {
							cpl_component_max_ref = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
							component_depth = round(log10(cpl_component_max_ref + 1) / log10(2.));
						}
						cpl_component_max_ref_extracted = true;
						node_list->~DOMNodeList();
						if (quantization == "QE.1") {
							component_min_ref = 16 << (component_depth - 8);
							component_max_ref = component_min_ref + (219 << (component_depth - 8));
						} else if (quantization == "QE.2") {
							component_min_ref = 0;
							component_max_ref = pow(2,component_depth);
						} else {
							QStringList result;
							result << (QStringList() << "2" << "Quantization" << "FAIL" << "Quantization " + quantization + " is not allowed for RGB components");
							if (!track_result_list.contains(result)) track_result_list << result;
							quantization_pass = false;
						}
						if (quantization_pass) {
							node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ComponentMinRef"));
							if (node_list->getLength() == 1) {
								int actual_value = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
								if (component_min_ref != actual_value) {
									QStringList result;
									result << (QStringList() << "2" << "ComponentMinRef" << "FAIL" << "Expected value:"+ QString::number(component_min_ref) << "Actual value:"+QString::number(actual_value));
									if (!track_result_list.contains(result)) track_result_list << result;
									quantization_pass = false;
								}
							} else {
								QStringList result;
								result << (QStringList() << "2" << "Quantization" << "FAIL" << "ComponentMinRef not present in Essence descriptor");
								if (!track_result_list.contains(result)) track_result_list << result;
								quantization_pass = false;
							}
							node_list->~DOMNodeList();
							if (!cpl_component_max_ref_extracted) {
								node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ComponentMaxRef"));
								if (node_list->getLength() == 1) {
									cpl_component_max_ref = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
									if (component_max_ref != cpl_component_max_ref) {
										QStringList result;
										result << (QStringList() << "2" << "ComponentMaxRef" << "FAIL" << "Expected value:" + QString::number(component_max_ref) << "Actual value:" + QString::number(cpl_component_max_ref));
										if (!track_result_list.contains(result)) track_result_list << result;
										quantization_pass = false;
									}
								}
								cpl_component_max_ref_extracted = true;
							}
							if (cpl_component_max_ref == -1) {
								QStringList result;
								result << (QStringList() << "2" << "Quantization" << "FAIL" << "ComponentMaxRef not present in Essence descriptor");
								if (!track_result_list.contains(result)) track_result_list << result;
								quantization_pass = false;
							}
							node_list->~DOMNodeList();
						}

					} else {
						QStringList result;
						result << (QStringList() << "2" << "Quantization" << "FAIL" << "No CDCI or RGBA Descriptor found");
						if (!track_result_list.contains(result)) track_result_list << result;
					}
					if (quantization_pass) {
						QStringList result;
						result << (QStringList() << "2" << "Quantization" << "PASS" << quantization);
						if (!track_result_list.contains(result)) track_result_list << result;
					}
				}
				/*
				 * FrameStructure
				 */
				if (vt_image.getFrameStructure().present()) {
					QString frame_structure = QString(vt_image.getFrameStructure().get().c_str());
					bool progressive = (frame_structure.toInt() == 0);
					bool frame_structure_pass = true;
					bool match = false;
					DOMNodeList* node_list;
					if (QString(XMLString::transcode(dom_element.getNodeName())).contains("CDCIDescriptor")) {
						node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("FrameLayout"));
						if (node_list->getLength() == 1) {
							int actual_value = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
							match = progressive ? (actual_value == 0) : (actual_value > 0);
						}
						node_list->~DOMNodeList();
					} else if (QString(XMLString::transcode(dom_element.getNodeName())).contains("RGBADescriptor")) {
						match = progressive;
					} else {
						QStringList result;
						result << (QStringList() << "2" << "FrameStructure" << "FAIL" << "No CDCI or RGBA Descriptor found");
						if (!track_result_list.contains(result)) track_result_list << result;
						match = false;
					}
					if (!match) {
						QStringList result;
						result << (QStringList() << "2" << "FrameStructure" << "FAIL" << "Expected value:"+ frame_structure << tr("Actual value: %1").arg(progressive ? QString("Progressive"):QString("Interlaced")));
						if (!track_result_list.contains(result)) track_result_list << result;
						frame_structure_pass = false;
					}
					if (frame_structure_pass) {
						QStringList result;
						result << (QStringList() << "2" << "FrameStructure" << "PASS" << frame_structure);
						if (!track_result_list.contains(result)) track_result_list << result;
					}
				}
				/*
				 * Stereoscopy
				 */
				if (vt_image.getStereoscopy().present()) {
					QString stereoscopy = QString(vt_image.getStereoscopy().get().c_str());
					bool stereoscopy_pass = true;
					QStringList result;
					result << (QStringList() << "2" << "Stereoscopy" << "WARN" << "Stereoscopy cannot be checked yet by IMF Tool");
					if (!track_result_list.contains(result)) track_result_list << result;
				}
				/*
				 * ColorComponents
				 */
				if (vt_image.getColorComponents().present()) {
					QString color_components = QString(vt_image.getColorComponents().get().c_str());
					bool color_components_pass = true;
					bool match = false;
					enum color_encodings {YCbCr, RGB, XYZ, Unknown} actual_color_components = Unknown;
					QStringList color_encoding_string_list;
					color_encoding_string_list << "YCbCr" << "RGB" << "XYZ" << "Unknown";
					if (QString(XMLString::transcode(dom_element.getNodeName())).contains("CDCIDescriptor")) {
						actual_color_components = YCbCr;
					} else if (QString(XMLString::transcode(dom_element.getNodeName())).contains("RGBADescriptor")) {
						actual_color_components = RGB;
						// App4 t.b.d.
					}
					match = (color_components.contains("YCbCr") && (actual_color_components == YCbCr)) ||
							(color_components.contains("RGB") && (actual_color_components == RGB)) ||
							(color_components.contains("XYZ") && (actual_color_components == XYZ));
					QStringList result;
					if (!match) {
						result << (QStringList() << "2" << "ColorComponents" << "FAIL" << tr("Expected value: %1").arg(color_components) << tr("Actual value: %1").arg(color_encoding_string_list.at(actual_color_components)));
					} else {
						result << (QStringList() << "2" << "ColorComponents" << "PASS" << color_components);
					}
					if (!track_result_list.contains(result)) track_result_list << result;
				}
				/*
				 * PixelBitDepthList
				 */
				if (vt_image.getPixelBitDepthList().present()) {
					bool pixel_bit_depth_pass = true;
					QList<int> pixel_bit_depth_list;
					dsl::ImageVirtualTrackType_PixelBitDepthListType::PixelBitDepthSequence bit_depth_sequence = vt_image.getPixelBitDepthList().get().getPixelBitDepth();
					for (dsl::ImageVirtualTrackType_PixelBitDepthListType::PixelBitDepthSequence::iterator it = bit_depth_sequence.begin(); it != bit_depth_sequence.end(); it++) {
						pixel_bit_depth_list << *it;
					}
					if (QString(XMLString::transcode(dom_element.getNodeName())).contains("CDCIDescriptor")) {
						if (cpl_component_depth == 0) {
							DOMNodeList* node_list;
							node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ComponentDepth"));
							if (node_list->getLength() == 1) {
								cpl_component_depth = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
							}
							node_list->~DOMNodeList();
						}
					} else if (QString(XMLString::transcode(dom_element.getNodeName())).contains("RGBADescriptor")) {
						DOMNodeList* node_list;
						if (!cpl_component_max_ref_extracted) {
							node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ComponentMaxRef"));
							if (node_list->getLength() == 1) {
								cpl_component_depth = round(log10(QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt() + 1) / log10(2.));
							}
							node_list->~DOMNodeList();
							cpl_component_max_ref_extracted = true;
						}
						else if (cpl_component_max_ref > 0) {
							cpl_component_depth = round(log10(cpl_component_max_ref + 1) / log10(2.));
						}
					}
					pixel_bit_depth_pass = pixel_bit_depth_list.contains(cpl_component_depth);
					QStringList result;
					QString list_string;
					foreach(int i, pixel_bit_depth_list) {
						list_string = list_string.append(QString::number(i)) + QString(",");
					}
					if (pixel_bit_depth_pass) {
						result << (QStringList() << "2" << "PixelBitDepth" << "PASS" << tr("%1, expected: %2").arg(cpl_component_depth).arg(list_string));
					} else {
						result << (QStringList() << "2" << "PixelBitDepth" << "FAIL" << tr("Expected value one of: %1").arg(list_string) << tr("Actual value: %1").arg(cpl_component_depth));
					}
					if (!track_result_list.contains(result)) track_result_list << result;
				}
				/*
				 * ImageFrameWidthList
				 */
				if (vt_image.getImageFrameWidthList().present()) {
					bool image_width_pass = true;
					QList<int> image_width_sequence_list;
					dsl::ImageVirtualTrackType_ImageFrameWidthListType::ImageFrameWidthSequence image_width_sequence = vt_image.getImageFrameWidthList().get().getImageFrameWidth();
					for (dsl::ImageVirtualTrackType_ImageFrameWidthListType::ImageFrameWidthSequence::iterator it = image_width_sequence.begin(); it != image_width_sequence.end(); it++) {
						image_width_sequence_list << *it;
					}
					DOMNodeList* node_list;
					int image_width = 0;
					node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("StoredWidth"));
					if (node_list->getLength() == 1) {
						image_width = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
					}
					node_list->~DOMNodeList();
					image_width_pass = image_width_sequence_list.contains(image_width);
					QStringList result;
					QString list_string;
					foreach(int i, image_width_sequence_list) {
						list_string = list_string.append(QString::number(i)) + QString(",");
					}
					if (image_width_pass) {
						result << (QStringList() << "2" << "ImageFrameWidth" << "PASS" << tr("Actual value: %1, expected one of: %2").arg(image_width).arg(list_string));
					} else {
						result << (QStringList() << "2" << "ImageFrameWidth" << "FAIL" << tr("Expected value one of: %1").arg(list_string) << tr("Actual value: %1").arg(image_width));
					}
					if (!track_result_list.contains(result)) track_result_list << result;
				}
				/*
				 * ImageFrameHeightList
				 */
				if (vt_image.getImageFrameHeightList().present()) {
					bool image_height_pass = true;
					QList<int> image_height_sequence_list;
					dsl::ImageVirtualTrackType_ImageFrameHeightListType::ImageFrameHeightSequence image_height_sequence = vt_image.getImageFrameHeightList().get().getImageFrameHeight();
					for (dsl::ImageVirtualTrackType_ImageFrameHeightListType::ImageFrameHeightSequence::iterator it = image_height_sequence.begin(); it != image_height_sequence.end(); it++) {
						image_height_sequence_list << *it;
					}
					DOMNodeList* node_list;
					int image_height = 0;
					node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("StoredHeight"));
					if (node_list->getLength() == 1) {
						image_height = QString(XMLString::transcode(node_list->item(0)->getTextContent())).toInt();
					}
					node_list->~DOMNodeList();
					image_height_pass = image_height_sequence_list.contains(image_height);
					QStringList result;
					QString list_string;
					foreach(int i, image_height_sequence_list) {
						list_string = list_string.append(QString::number(i)) + QString(",");
					}
					if (image_height_pass) {
						result << (QStringList() << "2" << "ImageFrameHeight" << "PASS" << tr("Actual value: %1, expected one of: %2").arg(image_height).arg(list_string));
					} else {
						result << (QStringList() << "2" << "ImageFrameHeight" << "FAIL" << tr("Expected value one of: %1").arg(list_string) << tr("Actual value: %1").arg(image_height));
					}
					if (!track_result_list.contains(result)) track_result_list << result;
				}
				/*
				 * FrameRateList
				 */
				if (vt_image.getFrameRateList().present()) {
					bool frame_rate_pass = true;
					QList<EditRate> frame_rate_list;
					dsl::ImageVirtualTrackType_FrameRateListType::FrameRateSequence frame_rate_sequence = vt_image.getFrameRateList().get().getFrameRate();
					for (dsl::ImageVirtualTrackType_FrameRateListType::FrameRateSequence::iterator it = frame_rate_sequence.begin(); it != frame_rate_sequence.end(); it++) {
						frame_rate_list << EditRate(it->front(), it->back());
					}
					DOMNodeList* node_list;
					EditRate actual_edit_rate = EditRate(0,0);
					node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("SampleRate"));
					if (node_list->getLength() == 1) {
						QStringList sample_rate;
						sample_rate = QString(XMLString::transcode(node_list->item(0)->getTextContent())).split('/');
						if (sample_rate.length() == 2) actual_edit_rate = EditRate(sample_rate.at(0).toInt(), sample_rate.at(1).toInt());
					}
					node_list->~DOMNodeList();
					frame_rate_pass = actual_edit_rate.IsValid() && frame_rate_list.contains(actual_edit_rate);
					QStringList result;
					QString list_string;
					foreach(EditRate e, frame_rate_list) {
						list_string = list_string.append(e.GetName() + QString("; "));
					}
					if (frame_rate_pass) {
						result << (QStringList() << "2" << "FrameRate" << "PASS" << tr("Actual value: %1 Expected value one of: %2").arg(actual_edit_rate.GetName()).arg(list_string));
					} else {
						result << (QStringList() << "2" << "FrameRate" << "FAIL" << tr("Expected value one of: %1").arg(list_string) << tr("Actual value: %1").arg(actual_edit_rate.GetName()));
					}
					if (!track_result_list.contains(result)) track_result_list << result;
				}
			}
		}
		bool pass = true;
		for (int i = 0; i < track_result_list.length(); i++) {
			if (track_result_list.at(i).at(2).contains("FAIL"))
				pass = false;
		}
		track_result_list[0][2] = pass ? "PASS" : "FAIL";
		result_list << track_result_list;
	}
	/* ####################
	 * AUDIO VIRTUAL TRACKS
	 */
	if (!vtl.getAudioVirtualTrack().empty()) {
		const dsl::DeliverableType_CompositionPlaylistConstraintsType_VirtualTrackListType::AudioVirtualTrackSequence& vt_audio_sequence = vtl.getAudioVirtualTrack();
		for (dsl::DeliverableType_CompositionPlaylistConstraintsType_VirtualTrackListType::AudioVirtualTrackConstIterator it(vt_audio_sequence.begin()); it != vt_audio_sequence.end(); it++) {
			//Loop all source encodings and check values against delivery specification
			QUuid matching_audio_track_found(0);
			QSet<QUuid> source_encoding_ids;
			QMap<QUuid, QSet<QUuid> >::iterator audio_iterator;
			QList<QStringList> track_result_list;
			if (audio_tracks_sourceencodings.size() == 0) {
				track_result_list << (QStringList() << "1" << "Main Audio Virtual Track" << "");
				track_result_list << (QStringList() << "2" << "Number of Virtual Track mismatch" << "");
			} else {
				audio_iterator = audio_tracks_sourceencodings.begin();
			}
			while ( (matching_audio_track_found == QUuid(0))  && (audio_tracks_sourceencodings.size() > 0) && (audio_iterator != audio_tracks_sourceencodings.end()) ) {
				track_result_list.clear();
				track_result_list << (QStringList() << "1" << "Main Audio Virtual Track" << "");
				source_encoding_ids = *audio_iterator;
				for (QSet<QUuid>::iterator source_encoding_id = source_encoding_ids.begin(); source_encoding_id != source_encoding_ids.end(); source_encoding_id++) {
					cpl2016::EssenceDescriptorBaseType ed = essence_descriptors.values(*source_encoding_id).first();
					cpl2016::EssenceDescriptorBaseType::AnySequence any_sequence = ed.getAny();
					DOMElement& dom_element(any_sequence.front());

					/*
					 * EssenceEncoding
					 */
					QString essence_encoding = QString(it->getEssenceEncoding().c_str());
					QStringList result;
					if (essence_encoding != "MXFGCClipWrappedBroadcastWaveAudioData") {
						result << "2" << "EssenceEncoding" << "FAIL" << "Invalid value in Delivery Specification: " + essence_encoding;
					} else {
						DOMNodeList* node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("ContainerFormat"));
						if (node_list->getLength() != 1)
							result << (QStringList() << "2" << "EssenceEncoding" << "FAIL" << "Ambiguous or missing ContainerFormat element");
						else {
							QString actual_value = QString(XMLString::transcode(node_list->item(0)->getTextContent())).replace("urn:smpte:ul:","");
							if ( actual_value == QString("060e2b34.04010101.0d010301.02060200")) {
								result << (QStringList() << "2" << "EssenceEncoding" << "PASS" << "MXFGCClipWrappedBroadcastWaveAudioData");
							} else {
								QStringList result;
								result << "2" << "EssenceEncoding" << "FAIL"
										<< tr("Expected: %1").arg(essence_encoding)
										<< tr("Actual: INVALID UL: %1").arg(actual_value);
							}
						}
						node_list->~DOMNodeList();
					}
					if (!track_result_list.contains(result)) track_result_list << result;
					/*
					 * SoundfieldGroupConfiguration
					 */
					if (it->getSoundfieldGroupConfiguration().present()) {
						QString sg = QString(it->getSoundfieldGroupConfiguration().get().getMCATagSymbol().c_str());
						bool sg_pass = true;
						QStringList result;
						DOMNodeList* node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R0).c_str(), XMLString::transcode("SoundfieldGroupLabelSubDescriptor"));
						QString actual_value;
						for (int n = 0; n < node_list->getLength(); n++) {
							DOMNodeList* child_nodes = node_list->item(n)->getChildNodes();
							for (int nn = 0; nn < child_nodes->getLength(); nn++) {
								if ( QString(XMLString::transcode(child_nodes->item(nn)->getNodeName())).contains("MCATagSymbol") ) {
									actual_value = QString(XMLString::transcode(child_nodes->item(nn)->getTextContent())).replace("sg","");
								}
							}
						}
						node_list->~DOMNodeList();
						if (!actual_value.isEmpty()) {
							if (actual_value == sg )
								result << "2" << "SoundfieldGroupConfiguration" << "PASS" << sg;
							else
								result << "2" << "SoundfieldGroupConfiguration" << "FAIL" << "Expected: " + sg << "Actual: " + actual_value;
						} else {
							result << "2" << "SoundfieldGroupConfiguration" << "FAIL" << "MCATagSymbol not present in SoundfieldGroupLabelSubDescriptor";
						}
						if (!track_result_list.contains(result)) track_result_list << result;
					}
					/*
					 * AudioChannelMapping
					 */
					if (it->getAudioChannelMapping().present()) {
						QStringList audio_channel_list;
						dsl::AudioVirtualTrackType_AudioChannelMappingType::AudioChannelSequence audio_channel_sequence = it->getAudioChannelMapping().get().getAudioChannel();
						bool ch_pass = true;
						QStringList result;
						dsl::AudioVirtualTrackType_AudioChannelMappingType::AudioChannelSequence::iterator ch_it;
						for (ch_it = audio_channel_sequence.begin(); ch_it < audio_channel_sequence.end(); ch_it++) {
							audio_channel_list << QString(ch_it->getMCATagSymbol().c_str());
						}
						DOMNodeList* node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R0).c_str(), XMLString::transcode("AudioChannelLabelSubDescriptor"));
						QStringList actual_value_list;
						for (int n = 0; n < node_list->getLength(); n++) {
							DOMNodeList* child_nodes = node_list->item(n)->getChildNodes();
							for (int nn = 0; nn < child_nodes->getLength(); nn++) {
								if ( QString(XMLString::transcode(child_nodes->item(nn)->getNodeName())).contains("MCATagSymbol") ) {
									actual_value_list << QString(XMLString::transcode(child_nodes->item(nn)->getTextContent())).replace("ch","");
								}
							}
						}
						node_list->~DOMNodeList();
						if (!actual_value_list.isEmpty()) {
							if (audio_channel_list == actual_value_list )
								result << "2" << "AudioChannelMapping" << "PASS" << actual_value_list.join(",");
							else
								result << "2" << "AudioChannelMapping" << "FAIL" << "Expected: " + audio_channel_list.join(",") << "Actual: " + actual_value_list.join(",");
						} else {
							result << "2" << "AudioChannelMapping" << "FAIL" << "MCATagSymbol not present in AudioChannelLabelSubDescriptor";
						}
						if (!track_result_list.contains(result)) track_result_list << result;
					}
					/*
					 * SampleRateList
					 */
					if (it->getSampleRateList().present()) {
						QStringList sample_rate_list;
						dsl::AudioVirtualTrackType_SampleRateListType::SampleRateSequence sample_rate_sequence = it->getSampleRateList().get().getSampleRate();
						QStringList result;
						dsl::AudioVirtualTrackType_SampleRateListType::SampleRateSequence::iterator sr_it;
						for (sr_it = sample_rate_sequence.begin(); sr_it < sample_rate_sequence.end(); sr_it++) {
							sample_rate_list << QString::number(sr_it->front())+"/"+QString::number(sr_it->back());
						}
						DOMNodeList* node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("SampleRate"));
						QString actual_sample_rate;
						if (node_list->getLength() == 1) {
							actual_sample_rate = QString(XMLString::transcode(node_list->item(0)->getTextContent()));
						}
						node_list->~DOMNodeList();
						if (sample_rate_list.contains(actual_sample_rate))
							result << "2" << "SampleRateList" << "PASS" << actual_sample_rate;
						else
							result << "2" << "SampleRateList" << "FAIL" << "Expected: " + sample_rate_list.join(",") << "Actual: " + actual_sample_rate;
						if (!track_result_list.contains(result)) track_result_list << result;
					}
				}

				bool pass = true;
				for (int i = 0; i < track_result_list.length(); i++) {
					if (track_result_list.at(i).at(2).contains("FAIL"))  pass = false;
				}
				if (pass) matching_audio_track_found = audio_iterator.key();
				audio_iterator++;
			} // while (matching_audio_track_found == QUuid(0))
			if (matching_audio_track_found != QUuid(0)) {
				track_result_list[0][2] = "PASS";
				audio_tracks_sourceencodings.remove(matching_audio_track_found);
			} else {
				track_result_list[0][2] = "FAIL";
			}

			result_list << track_result_list;
		}
		/* #########################
		 * TIMED TEXT VIRTUAL TRACKS
		 */
		if (!vtl.getTimedTextVirtualTrack().empty()) {
			const dsl::DeliverableType_CompositionPlaylistConstraintsType_VirtualTrackListType::TimedTextVirtualTrackSequence& vt_timed_text_sequence = vtl.getTimedTextVirtualTrack();
			for (dsl::DeliverableType_CompositionPlaylistConstraintsType_VirtualTrackListType::TimedTextVirtualTrackConstIterator it(vt_timed_text_sequence.begin()); it != vt_timed_text_sequence.end(); it++) {
				QUuid matching_timed_text_track_found(0);
				QSet<QUuid> source_encoding_ids;
				QMap<QUuid, QSet<QUuid> >::iterator timed_text_iterator;
				QList<QStringList> track_result_list;
				if (timed_text_tracks_sourceencodings.size() == 0) {
					track_result_list << (QStringList() << "1" << "Timed Text Virtual Track" << "");
					track_result_list << (QStringList() << "2" << "Number of Virtual Track mismatch" << "");
				} else {
					timed_text_iterator = timed_text_tracks_sourceencodings.begin();
				}
				while ( (matching_timed_text_track_found == QUuid(0))  && (timed_text_tracks_sourceencodings.size() > 0) && (timed_text_iterator != timed_text_tracks_sourceencodings.end())) {
					track_result_list.clear();
					track_result_list << (QStringList() << "1" << "Timed Text Virtual Track" << "");
					source_encoding_ids = *timed_text_iterator;
					for (QSet<QUuid>::iterator source_encoding_id = source_encoding_ids.begin(); source_encoding_id != source_encoding_ids.end(); source_encoding_id++) {
						cpl2016::EssenceDescriptorBaseType ed = essence_descriptors.values(*source_encoding_id).first();
						cpl2016::EssenceDescriptorBaseType::AnySequence any_sequence = ed.getAny();
						DOMElement& dom_element(any_sequence.front());
						/*
						 * Sequence Type
						 */
						QString sequence_type = QString(it->getTimedTextSequenceType().c_str());
						QStringList result;
						QString actual_value = timed_text_tracks_sequence_type.value(timed_text_iterator.key());
						if (!actual_value.isEmpty()) {
							if (actual_value == sequence_type )
								result << "2" << "SequenceType" << "PASS" << sequence_type;
							else
								result << "2" << "SequenceType" << "FAIL" << "Expected: " + sequence_type << "Actual: " + actual_value;
						} else {
							result << "2" << "SequenceType" << "FAIL" << "Type of Timed Text Sequence could not be determined";
						}
						if (!track_result_list.contains(result)) track_result_list << result;

						/*
						 * NamespaceURI
						 */
						if (it->getNamespaceURI().present()) {
							QStringList result;
							QString namespace_uri = QString(it->getNamespaceURI().get().c_str());
							DOMNodeList* node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R0).c_str(), XMLString::transcode("DCTimedTextDescriptor"));
							QString actual_value;
							node_list = dom_element.getElementsByTagNameNS(xsd::cxx::xml::string(XML_NAMESPACE_R1).c_str(), XMLString::transcode("NamespaceURI"));
							if (node_list->getLength() == 1) {
								actual_value = QString(XMLString::transcode(node_list->item(0)->getTextContent()));
							}
							if (!actual_value.isEmpty()) {
								if (actual_value == namespace_uri )
									result << "2" << "NamespaceURI" << "PASS" << namespace_uri;
								else
									result << "2" << "NamespaceURI" << "FAIL" << "Expected: " + namespace_uri << "Actual: " + actual_value;
							} else {
								result << "2" << "NamespaceURI" << "FAIL" << "NamespaceURI not present in DCTimedTextDescriptor";
							}
							if (!track_result_list.contains(result)) track_result_list << result;
						}
					}
					bool pass = true;
					for (int i = 0; i < track_result_list.length(); i++) {
						if (track_result_list.at(i).at(2).contains("FAIL"))  pass = false;
					}
					if (pass) matching_timed_text_track_found = timed_text_iterator.key();
					timed_text_iterator++;
				} // while (matching_timed_text_track_found == QUuid(0))
				if (matching_timed_text_track_found != QUuid(0)) {
					track_result_list[0][2] = "PASS";
					timed_text_tracks_sourceencodings.remove(matching_timed_text_track_found);
				} else {
					track_result_list[0][2] = "FAIL";
				}
				result_list << track_result_list;
			}
		}
	}
	bool pass = true;
	for (int i = 1; i < result_list.length(); i++) {
		if (result_list.at(i).at(2).contains("FAIL"))
			pass = false;
	}
	result_list[0][2] = pass ? "PASS" : "FAIL";

	emit Result(result_list);

	Error error = Error::None;
	return error;
}
