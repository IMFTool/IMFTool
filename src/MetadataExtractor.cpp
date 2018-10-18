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

#include "MetadataExtractor.h"
#include "AS_DCP_internal.h"
#include "AS_02.h"
#include "PCMParserList.h"
#include "SMPTE_Labels.h" // (k)
#include <KM_fileio.h>
#include <cmath>
#include <QtCore>
#include <QDebug>
#include <cstring>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/validators/common/Grammar.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>

#include <xercesc/framework/XMLFormatter.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMLSSerializer.hpp>
#include <xercesc/dom/DOMLSOutput.hpp>

#include <string>
#include <QCryptographicHash>
#include <QMessageBox>
#include "ImfPackageCommon.h"
#include "global.h"

using namespace xercesc;



MetadataExtractor::MetadataExtractor(QObject *pParent /*= NULL*/) :
QObject(pParent) {

}

Error MetadataExtractor::ReadMetadata(Metadata &rMetadata, const QString &rSourceFile) {

	Error error(Error::None);
	QFileInfo source_file(rSourceFile);
	if(source_file.exists() && source_file.isFile() && !source_file.isSymLink()) {
		EssenceType_t essence_type = ESS_UNKNOWN;
		if(is_mxf_file(rSourceFile)) {
			Result_t result = ASDCP::EssenceType(source_file.absoluteFilePath().toStdString(), essence_type);
			qDebug()<< QString(source_file.absoluteFilePath()) << essence_type;
			if(ASDCP_SUCCESS(result)) {
				switch(essence_type) {
#ifdef APP5_ACES
					case  ASDCP::ESS_AS02_ACES:
						error = ReadAcesMxfDescriptor(rMetadata, source_file);
						break;
#endif
					case ASDCP::ESS_AS02_JPEG_2000:
						error = ReadJP2KMxfDescriptor(rMetadata, source_file);
						break;
					case ASDCP::ESS_AS02_PCM_24b_48k:
					case ASDCP::ESS_AS02_PCM_24b_96k:
						error = ReadPcmMxfDescriptor(rMetadata, source_file);
						break;

					case ASDCP::ESS_AS02_TIMED_TEXT:
						error = ReadTimedTextMxfDescriptor(rMetadata, source_file);
						break;
					default:
						break;
				}
			}
		}
		else if(is_wav_file(rSourceFile)) {
			error = ReadWavHeader(rMetadata, source_file);
		}

		else if(is_ttml_file(rSourceFile)) {
			error = ReadTimedTextMetadata(rMetadata, source_file);
		}


		else error = Error(Error::UnsupportedEssence, source_file.fileName());
	}
	else error = Error(Error::SourceFilesMissing, tr("Expected file: %1").arg(source_file.absoluteFilePath()));
	return error;
}

#ifdef APP5_ACES
Error MetadataExtractor::ReadAcesMxfDescriptor(Metadata &rMetadata, const QFileInfo &rSourceFile) {

	Error error;
	Metadata metadata(Metadata::Aces);
	metadata.fileName = rSourceFile.fileName();
	metadata.filePath = rSourceFile.filePath();
	AESDecContext* p_context = NULL;
	HMACContext* p_hmac = NULL;
	AS_02::ACES::MXFReader reader;
	AS_02::ACES::FrameBuffer frame_buffer;
	quint32 frame_count = 0;

	Result_t result = reader.OpenRead(rSourceFile.absoluteFilePath().toStdString());

	if(ASDCP_SUCCESS(result)) {
		WriterInfo writerinfo;
		result = reader.FillWriterInfo(writerinfo);
		if(KM_SUCCESS(result)) {
			char str_buf[16], str_buf2[40];
			metadata.assetId = convert_uuid((unsigned char*)writerinfo.AssetUUID);
		}
		ASDCP::MXF::RGBAEssenceDescriptor *aces_descriptor = NULL;
		result = reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_RGBAEssenceDescriptor), reinterpret_cast<MXF::InterchangeObject**>(&aces_descriptor));

		if(KM_SUCCESS(result) && aces_descriptor) {
			if(aces_descriptor->ContainerDuration.empty() == false) frame_count = aces_descriptor->ContainerDuration.get();
			else 			frame_count = reader.AS02IndexReader().GetDuration();
		}
		if(frame_count == 0) {
			error = Error(Error::UnknownDuration, QString(), true);
		}
		metadata.duration = Duration(frame_count);
		if(aces_descriptor) {
			metadata.colorEncoding = Metadata::RGBA;
			metadata.editRate = EditRate(aces_descriptor->SampleRate.Numerator, aces_descriptor->SampleRate.Denominator);
			metadata.aspectRatio = aces_descriptor->AspectRatio;
			if(aces_descriptor->DisplayHeight.empty() == false) metadata.displayHeight = aces_descriptor->DisplayHeight;
			else if(aces_descriptor->SampledHeight.empty() == false) metadata.displayHeight = aces_descriptor->SampledHeight;
			else metadata.displayHeight = aces_descriptor->StoredHeight;
			if(aces_descriptor->DisplayWidth.empty() == false) metadata.displayWidth = aces_descriptor->DisplayWidth;
			else if(aces_descriptor->SampledWidth.empty() == false) metadata.displayWidth = aces_descriptor->SampledWidth;
			else metadata.displayWidth = aces_descriptor->StoredWidth;
			metadata.storedHeight = aces_descriptor->StoredHeight;
			metadata.storedWidth = aces_descriptor->StoredWidth;
			metadata.horizontalSubsampling = 1;
			metadata.componentDepth = 253;
			ASDCP::UL TransferCharacteristic; // (k)
			ASDCP::UL ColorPrimaries; // (k)
			ColorPrimaries = aces_descriptor->ColorPrimaries;
			TransferCharacteristic = aces_descriptor->TransferCharacteristic;
			// get ColorPrimaries
			char buf[64];
			QString CP;
			if (ColorPrimaries.HasValue()) {

				CP = ColorPrimaries.EncodeString(buf, 64);
				CP = CP.toLower(); //.replace(".", "");
				metadata.colorPrimaries = SMPTE::ColorPrimariesMap[CP];
			}

			// get TransferCharacteristic
			QString TC;
			if (TransferCharacteristic.HasValue()){
				TC = TransferCharacteristic.EncodeString(buf, 64);
				TC = TC.toLower(); //.replace(".", "");
				metadata.transferCharcteristics = SMPTE::TransferCharacteristicMap[TC];

			}
			if (aces_descriptor->PictureEssenceCoding.HasValue()) {
				char buf[64];
				metadata.pictureEssenceCoding = QString(aces_descriptor->PictureEssenceCoding.EncodeString(buf, 64));
			}
			result = reader.FillAncillaryResourceList(metadata.AncillaryResources);
			if(KM_FAILURE(result)) qWarning() << "Couldn't extract informations about anc. resources";
		}
	}
	else {
		error = Error(result);
		return error;
	}
	rMetadata = metadata;
	return error;
}
#endif

Error MetadataExtractor::ReadJP2KMxfDescriptor(Metadata &rMetadata, const QFileInfo &rSourceFile) {

	Error error;
	Metadata metadata(Metadata::Jpeg2000);
	metadata.fileName = rSourceFile.fileName();
	metadata.filePath = rSourceFile.filePath();
	AESDecContext*     p_context = NULL;
	HMACContext*       p_hmac = NULL;
	AS_02::JP2K::MXFReader    reader;
	JP2K::FrameBuffer  frame_buffer;
	ui32_t             frame_count = 0;

	Result_t result = reader.OpenRead(rSourceFile.absoluteFilePath().toStdString());

	if(ASDCP_SUCCESS(result)) {
		WriterInfo writerinfo;
		result = reader.FillWriterInfo(writerinfo);
		if(KM_SUCCESS(result)) {
			char str_buf[16], str_buf2[40];
			metadata.assetId = convert_uuid((unsigned char*)writerinfo.AssetUUID);
		}

		ASDCP::MXF::RGBAEssenceDescriptor *rgba_descriptor = NULL;
		ASDCP::MXF::CDCIEssenceDescriptor *cdci_descriptor = NULL;

		result = reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_RGBAEssenceDescriptor), reinterpret_cast<MXF::InterchangeObject**>(&rgba_descriptor));
		if(KM_SUCCESS(result) && rgba_descriptor) {
			if(rgba_descriptor->ContainerDuration.empty() == false) frame_count = rgba_descriptor->ContainerDuration.get();
			else frame_count = reader.AS02IndexReader().GetDuration();
		}
		else {
			result = reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_CDCIEssenceDescriptor), reinterpret_cast<MXF::InterchangeObject**>(&cdci_descriptor));
			if(KM_SUCCESS(result) && cdci_descriptor) {
				if(cdci_descriptor->ContainerDuration.empty() == false) frame_count = cdci_descriptor->ContainerDuration.get();
				else frame_count = reader.AS02IndexReader().GetDuration();
			}
		}
		if(frame_count == 0) {
			error = Error(Error::UnknownDuration, QString(), true);
		}
		metadata.duration = Duration(frame_count);
		ASDCP::UL TransferCharacteristic; // (k)
		ASDCP::UL ColorPrimaries; // (k)

		if(rgba_descriptor) {
			metadata.colorEncoding = Metadata::RGBA;
			metadata.editRate = EditRate(rgba_descriptor->SampleRate.Numerator, rgba_descriptor->SampleRate.Denominator);
			metadata.aspectRatio = rgba_descriptor->AspectRatio;
			if(rgba_descriptor->DisplayHeight.empty() == false) metadata.displayHeight = rgba_descriptor->DisplayHeight;
			else if(rgba_descriptor->SampledHeight.empty() == false) metadata.displayHeight = rgba_descriptor->SampledHeight;
			else metadata.displayHeight = rgba_descriptor->StoredHeight;
			if(rgba_descriptor->DisplayWidth.empty() == false) metadata.displayWidth = rgba_descriptor->DisplayWidth;
			else if(rgba_descriptor->SampledWidth.empty() == false) metadata.displayWidth = rgba_descriptor->SampledWidth;
			else metadata.displayWidth = rgba_descriptor->StoredWidth;
			metadata.storedHeight = rgba_descriptor->StoredHeight;
			metadata.storedWidth = rgba_descriptor->StoredWidth;
			metadata.horizontalSubsampling = 1;
			metadata.componentDepth = 0;
			if (rgba_descriptor->PixelLayout.HasValue()) {
				char buf[64];
				QString pixel_layout;
				pixel_layout = QString(rgba_descriptor->PixelLayout.EncodeString(buf, 64)); //WR
				if (pixel_layout.contains(QRegExp("\\([0-9]*\\)"))) {
					metadata.componentDepth = pixel_layout.split('(')[1].split(')').first().toInt();
				}
			}
			if (metadata.componentDepth == 0) {
				// Try deriving from ComponentMaxRef, if present.
				if(rgba_descriptor->ComponentMaxRef.empty() == false) {
					metadata.componentDepth = round (log10(rgba_descriptor->ComponentMaxRef.get() + 1) / log10(2.));
				} else {
					qDebug() << "WARNING: Pixel Layout and ComponentMaxRef do not contain proper values!";
				}

			}
			TransferCharacteristic = rgba_descriptor->TransferCharacteristic; // (k)
			ColorPrimaries = rgba_descriptor->ColorPrimaries; // (k)
			if(rgba_descriptor->ComponentMinRef.empty() == false) metadata.componentMinRef = rgba_descriptor->ComponentMinRef;
			if(rgba_descriptor->ComponentMaxRef.empty() == false) metadata.componentMaxRef = rgba_descriptor->ComponentMaxRef;
			if (rgba_descriptor->PictureEssenceCoding.HasValue()) {
				char buf[64];
				metadata.pictureEssenceCoding = QString(rgba_descriptor->PictureEssenceCoding.EncodeString(buf, 64)); //WR
			}
		}
		else if(cdci_descriptor) {
			metadata.colorEncoding = Metadata::CDCI;
			metadata.editRate = EditRate(cdci_descriptor->SampleRate.Numerator, cdci_descriptor->SampleRate.Denominator);
			metadata.aspectRatio = cdci_descriptor->AspectRatio;
			if(cdci_descriptor->DisplayHeight.empty() == false) metadata.displayHeight = cdci_descriptor->DisplayHeight;
			else if(cdci_descriptor->SampledHeight.empty() == false) metadata.displayHeight = cdci_descriptor->SampledHeight;
			else metadata.displayHeight = cdci_descriptor->StoredHeight;
			if(cdci_descriptor->DisplayWidth.empty() == false) metadata.displayWidth = cdci_descriptor->DisplayWidth;
			else if(cdci_descriptor->SampledWidth.empty() == false) metadata.displayWidth = cdci_descriptor->SampledWidth;
			else metadata.displayWidth = cdci_descriptor->StoredWidth;
			metadata.storedHeight = cdci_descriptor->StoredHeight;
			metadata.storedWidth = cdci_descriptor->StoredWidth;
			metadata.horizontalSubsampling = cdci_descriptor->HorizontalSubsampling;
			metadata.componentDepth = cdci_descriptor->ComponentDepth;
			if (cdci_descriptor->PictureEssenceCoding.HasValue()) {
				char buf[64];
				metadata.pictureEssenceCoding = QString(cdci_descriptor->PictureEssenceCoding.EncodeString(buf, 64)); //WR
			}
			TransferCharacteristic = cdci_descriptor->TransferCharacteristic; // (k)
			ColorPrimaries = cdci_descriptor->ColorPrimaries; // (k)
		}

		// (k) - start
		char buf[64];

		// get ColorPrimaries
		QString CP;
		if (ColorPrimaries.HasValue()) {

			CP = ColorPrimaries.EncodeString(buf, 64);
			CP = CP.toLower(); //.replace(".", "");
			metadata.colorPrimaries = SMPTE::ColorPrimariesMap[CP];
		}

		// get TransferCharacteristic
		QString TC;
		if (TransferCharacteristic.HasValue()){
			TC = TransferCharacteristic.EncodeString(buf, 64);
			TC = TC.toLower(); //.replace(".", "");
			metadata.transferCharcteristics = SMPTE::TransferCharacteristicMap[TC];

		}
		// (k) - end
	}
	else {
		error = Error(result);
		return error;
	}
	rMetadata = metadata;
	return error;
}

Error MetadataExtractor::ReadPcmMxfDescriptor(Metadata &rMetadata, const QFileInfo &rSourceFile) {

	Error error;
	Metadata metadata(Metadata::Pcm);
	metadata.fileName = rSourceFile.fileName();
	metadata.filePath = rSourceFile.filePath();
	AESDecContext* p_context = NULL;
	HMACContext* p_hmac = NULL;
	AS_02::PCM::MXFReader reader;
	PCM::FrameBuffer frame_buffer;
	ui32_t duration = 0;

	ASDCP::MXF::WaveAudioDescriptor *wave_descriptor = NULL;

	Result_t result = reader.OpenRead(rSourceFile.absoluteFilePath().toStdString(), ASDCP::Rational(24, 1));

	if(KM_SUCCESS(result)) {
		WriterInfo writerinfo;
		result = reader.FillWriterInfo(writerinfo);
		if(KM_SUCCESS(result)) {
			char str_buf[16], str_buf2[40];
			metadata.assetId = convert_uuid((unsigned char*)writerinfo.AssetUUID);
		}

		ASDCP::MXF::InterchangeObject* tmp_obj = NULL;

		result = reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_WaveAudioDescriptor), &tmp_obj);

		if(KM_SUCCESS(result)) {
			wave_descriptor = dynamic_cast<ASDCP::MXF::WaveAudioDescriptor*>(tmp_obj);
			if(wave_descriptor == NULL) {
				error = Error(Error::UnsupportedEssence);
				return error;
			}
			if(wave_descriptor->ContainerDuration.empty() == true) {
				duration = reader.AS02IndexReader().GetDuration();
			}
			else {
				duration = wave_descriptor->ContainerDuration.get();
			}

			if(duration == 0){
				qDebug() << "ContainerDuration not set in index, attempting to use Duration from SourceClip.";
				result = reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_SourceClip), &tmp_obj);
				  if ( KM_SUCCESS(result)){
					  ASDCP::MXF::SourceClip *sourceClip = dynamic_cast<ASDCP::MXF::SourceClip*>(tmp_obj);
					  if ( ! sourceClip->Duration.empty() )
					  {
						  duration = sourceClip->Duration;
					  }
				  }
			}


			if(duration == 0) {
				error = Error(Error::UnknownDuration, QString(), true);
				qDebug() << "Error: " << error;
			}
			ASDCP::MXF::InterchangeObject* tmp_obj = NULL;
			result = reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_SoundfieldGroupLabelSubDescriptor), &tmp_obj);
			if(KM_SUCCESS(result)) {
				ASDCP::MXF::SoundfieldGroupLabelSubDescriptor *soundfield_group = NULL;
				soundfield_group = dynamic_cast<ASDCP::MXF::SoundfieldGroupLabelSubDescriptor*>(tmp_obj);
				if(soundfield_group) {
					if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_ST)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroupST;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_DM)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroupDM;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_30)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroup30;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_40)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroup40;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_50)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroup50;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_60)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroup60;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_70)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroup70;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_LtRt)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroupLtRt;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_51Ex)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroup51EX;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_HI)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroupHA;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioSoundfield_VIN)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroupVA;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioSoundfield_51)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroup51;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioSoundfield_71)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroup71;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioSoundfield_SDS)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroupSDS;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioSoundfield_61)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroup61;
					else if(soundfield_group->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioSoundfield_M)) metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroupM;
					else metadata.soundfieldGroup = SoundfieldGroup::SoundFieldGroupNone;
					//WR
					const unsigned short IdentBufferLen = 256;
					char identbuf[IdentBufferLen];
					if (!soundfield_group->MCATitle.empty()) {
						metadata.mcaTitle = QString(soundfield_group->MCATitle.get().EncodeString(identbuf, IdentBufferLen));
					}
					if (!soundfield_group->MCATitleVersion.empty()) {
						metadata.mcaTitleVersion = QString(soundfield_group->MCATitleVersion.get().EncodeString(identbuf, IdentBufferLen));
					}
					if (!soundfield_group->MCAAudioContentKind.empty()) {
						metadata.mcaAudioContentKind = QString(soundfield_group->MCAAudioContentKind.get().EncodeString(identbuf, IdentBufferLen));
					}
					if (!soundfield_group->MCAAudioElementKind.empty()) {
						metadata.mcaAudioElementKind = QString(soundfield_group->MCAAudioElementKind.get().EncodeString(identbuf, IdentBufferLen));
					}
					//WR
					if(metadata.soundfieldGroup.IsWellKnown()) {
						std::list<InterchangeObject*> object_list;
						result = reader.OP1aHeader().GetMDObjectsByType(DefaultCompositeDict().ul(MDD_AudioChannelLabelSubDescriptor), object_list);
						if(KM_SUCCESS(result)) {
							for(std::list<InterchangeObject*>::iterator it = object_list.begin(); it != object_list.end(); ++it) {
								if(ASDCP::MXF::AudioChannelLabelSubDescriptor *p_channel_descriptor = dynamic_cast<ASDCP::MXF::AudioChannelLabelSubDescriptor*>(*it)) {
									if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioChannel_M1)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelM1);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioChannel_M2)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelM2);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioChannel_Lt)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelLt);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioChannel_Rt)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelRt);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioChannel_Lst)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelLst);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioChannel_Rst)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelRst);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_IMFAudioChannel_S)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelS);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_L)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelL);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_R)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelR);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_C)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelC);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_LFE)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelLFE);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_Ls)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelLs);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_Rs)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelRs);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_Lss)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelLss);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_Rss)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelRss);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_Lrs)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelLrs);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_Rrs)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelRrs);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_Lc)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelLc);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_Rc)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelRc);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_Cs)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelCs);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_HI)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelHI);
									else if(p_channel_descriptor->MCALabelDictionaryID == DefaultCompositeDict().ul(MDD_DCAudioChannel_VIN)) metadata.soundfieldGroup.AddChannel(p_channel_descriptor->MCAChannelID - 1, SoundfieldGroup::ChannelVIN);
									//WR
									const unsigned short IdentBufferLen = 128;
									char identbuf[IdentBufferLen];
									if (!p_channel_descriptor->RFC5646SpokenLanguage.empty()) {
										metadata.languageTag = QString(p_channel_descriptor->RFC5646SpokenLanguage.get().EncodeString(identbuf, IdentBufferLen));
									}
									//WR
								}
							}
						}
					}
				}
			}
			metadata.duration = Duration(duration);
			metadata.editRate = wave_descriptor->SampleRate;
			metadata.audioChannelCount = wave_descriptor->ChannelCount;
			metadata.audioQuantization = wave_descriptor->QuantizationBits;
		}
	}
	else {
		error = Error(result);
		return error;
	}
	rMetadata = metadata;
	return error;
}

Error MetadataExtractor::ReadTimedTextMxfDescriptor(Metadata &rMetadata, const QFileInfo &rSourceFile){

	Error error;
	Metadata metadata(Metadata::TimedText);
	metadata.fileName = rSourceFile.fileName();
	metadata.filePath = rSourceFile.filePath();

	AS_02::TimedText::MXFReader reader;
	//AS_02::TimedText::TimedTextDescriptor TDesc;
	//WR: Only ASDCP::MXF::TimedTextDescriptor exposes RFC5646LanguageTagList:
	ASDCP::MXF::TimedTextDescriptor *tt_descriptor = NULL;

	AS_02::Result_t result = reader.OpenRead(rSourceFile.absoluteFilePath().toStdString());
	if(ASDCP_SUCCESS(result)) {
		//WR
		WriterInfo writerinfo;
		result = reader.FillWriterInfo(writerinfo);
		if(KM_SUCCESS(result)) {
			char str_buf[16], str_buf2[40];
			metadata.assetId = convert_uuid((unsigned char*)writerinfo.AssetUUID);
		}
		ASDCP::MXF::InterchangeObject* tmp_obj = NULL;

		result = reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_TimedTextDescriptor), &tmp_obj);

		if(ASDCP_SUCCESS(result)) {
			tt_descriptor = dynamic_cast<ASDCP::MXF::TimedTextDescriptor*>(tmp_obj);
			if(tt_descriptor == NULL) {
				error = Error(Error::UnsupportedEssence);
				return error;
			}

			//WR
			metadata.effectiveFrameRate = tt_descriptor->SampleRate;
			metadata.editRate = mCplEditRate;
			metadata.originalDuration = Duration(tt_descriptor->ContainerDuration);
			if ( metadata.editRate.IsValid() ) {  // duration and editRate will be set afterwards in ImfPackage::ParseAssetMap()
				metadata.duration = Duration(ceil(tt_descriptor->ContainerDuration / metadata.effectiveFrameRate.GetQuotient() * metadata.editRate.GetQuotient()));
			}
			//WR
			metadata.profile = QString::fromStdString(tt_descriptor->NamespaceURI);
			const unsigned short IdentBufferLen = 128;
			char identbuf[IdentBufferLen];
			if (!tt_descriptor->RFC5646LanguageTagList.empty()) {
				metadata.languageTag = QString(tt_descriptor->RFC5646LanguageTagList.get().EncodeString(identbuf, IdentBufferLen));
			}
			//WR
		} else {
			error = Error(result);
			return error;
		}
	}
	else {
		error = Error(result);
		return error;
	}
	rMetadata = metadata;
	return error;
}



Error MetadataExtractor::ReadWavHeader(Metadata &rMetadata, const QFileInfo &rSourceFile) {

	Error error;
	Metadata metadata(Metadata::Pcm);
	metadata.fileName = rSourceFile.fileName();
	metadata.filePath = rSourceFile.filePath();
	PCM::FrameBuffer buffer;
	ASDCP::PCM::WAVParser parser;
	ASDCP::PCM::AudioDescriptor audio_descriptor;
	buffer.Capacity(rSourceFile.size());
	Result_t result = parser.OpenRead(rSourceFile.absoluteFilePath().toStdString(), ASDCP::Rational(24, 1));
	// set up MXF writer
	if(ASDCP_SUCCESS(result)) {
		result = parser.FillAudioDescriptor(audio_descriptor);
		if(ASDCP_SUCCESS(result)) {
			result = parser.Reset();
			if(ASDCP_SUCCESS(result)) result = parser.OpenRead(rSourceFile.absoluteFilePath().toStdString(), audio_descriptor.AudioSamplingRate); // Dirty hack? We need the duration in multiples of samples.
			if(ASDCP_SUCCESS(result)) result = parser.FillAudioDescriptor(audio_descriptor);
			if(ASDCP_SUCCESS(result)) {
				metadata.duration = audio_descriptor.ContainerDuration;
				metadata.editRate = audio_descriptor.AudioSamplingRate;
				metadata.audioChannelCount = audio_descriptor.ChannelCount;
				metadata.audioQuantization = audio_descriptor.QuantizationBits;
			}
			else {
				error = Error(result);
				return error;
			}
		}
		else {
			error = Error(result);
			return error;
		}
	}
	else {
		error = Error(result);
		return error;
	}
	rMetadata = metadata;
	return error;
}


float MetadataExtractor::ConvertTimingQStringtoDouble(QString string_time, float fr, int tr){

	float time, h, min, sec, msec;

	if (string_time.right(2) == "ms")
		time = string_time.remove(QChar('m'), Qt::CaseInsensitive).remove(QChar('s'), Qt::CaseInsensitive).toFloat() / 1000;

	else if (string_time.right(1) == "s")
		time = string_time.remove(QChar('s'), Qt::CaseInsensitive).toFloat();

	else if (string_time.right(1) == "m")
		time = string_time.remove(QChar('m'), Qt::CaseInsensitive).toFloat() * 60;

	else if (string_time.right(1) == "h")
		time = string_time.remove(QChar('h'), Qt::CaseInsensitive).toFloat() * 3600;

	else if (string_time.right(1) == "f")
		time = string_time.remove(QChar('f'), Qt::CaseInsensitive).toFloat() / fr;

	else if (string_time.right(1) == "t")
		time = string_time.remove(QChar('t'), Qt::CaseInsensitive).toFloat() / tr;

	else if (string_time.left(9).right(1) == "."){ // Time expression with fractions of seconds, e.g. 00:00:20.1
		h = string_time.left(2).toFloat();
		min = string_time.left(5).right(2).toFloat();
		sec = string_time.left(8).right(2).toFloat();
		msec = string_time.remove(0, 7).replace(0, 2, "0.").toFloat();
		time = (h*60*60)+(min*60)+sec+msec;
	}
	else{  // Time expression with frames e.g. 00:00:00:15
		h = string_time.left(2).toFloat();
		min = string_time.left(5).right(2).toFloat();
		sec = string_time.left(8).right(2).toFloat() + (string_time.remove(0, 9).toFloat() / fr);
		time = (h*60*60)+(min*60)+sec;
	}
	return time;
}

float MetadataExtractor::GetElementDuration(DOMElement* eleDom, float fr, int tr){

	float duration=0, end=0, beg=0, dur=0;
	QString end_string = XMLString::transcode(eleDom->getAttribute(XMLString::transcode("end")));
	if(!end_string.isEmpty())
		end = ConvertTimingQStringtoDouble(end_string, fr, tr);

	QString beg_string = XMLString::transcode(eleDom->getAttribute(XMLString::transcode("begin")));
	if(!beg_string.isEmpty())
		beg = ConvertTimingQStringtoDouble(beg_string, fr, tr);

	QString dur_string = XMLString::transcode(eleDom->getAttribute(XMLString::transcode("dur")));
	if(!dur_string.isEmpty())
		dur = ConvertTimingQStringtoDouble(dur_string, fr, tr);

	if(!end_string.isEmpty())
		duration = end;
	else
		duration = beg+dur;

	if(dur == 0 && end == 0)
		duration = 0;

	return duration;
}

float MetadataExtractor::DurationExtractor(DOMDocument *dom_doc, float fr, int tr) {

	float bodyduration=0, eleduration=0, divduration=0, div2duration=0, pduration=0, p2duration=0, spanduration=0, duration=0;

	DOMNodeList	*bodyitems = dom_doc->getElementsByTagName(XMLString::transcode("body"));
	DOMElement* bodyeleDom = dynamic_cast<DOMElement*>(bodyitems->item(0));
	bodyduration = GetElementDuration(bodyeleDom, fr, tr);
	if (bodyduration > 0) {
		duration = bodyduration;
		return duration;
	}

	QString bodytime = XMLString::transcode(bodyeleDom->getAttribute(XMLString::transcode("timeContainer")));  //bodytime: par/seq

	//---start with div childelements of body---
	DOMNodeList	*divitems = dom_doc->getElementsByTagName(XMLString::transcode("div"));
	divduration=0;
	for(int i=0; i < divitems->getLength(); i++){

		DOMElement* diveleDom = dynamic_cast<DOMElement*>(divitems->item(i));

		QString comp = XMLString::transcode(diveleDom->getParentNode()->getNodeName());			//look for body-child div!
		if(comp != "div"){
			QString divtime = XMLString::transcode(diveleDom->getAttribute(XMLString::transcode("timeContainer")));	//divtime: par/seq
			eleduration = GetElementDuration(diveleDom, fr,tr);
			div2duration = 0;
			pduration = 0;

			if(eleduration == 0){
				DOMNodeList *div2items = diveleDom->getElementsByTagName(XMLString::transcode("div"));
				div2duration=0;
				for(int j=0; j < div2items->getLength(); j++){

					DOMElement* div2eleDom = dynamic_cast<DOMElement*>(div2items->item(j));
					QString div2time = XMLString::transcode(div2eleDom->getAttribute(XMLString::transcode("timeContainer"))); //div2time: par/seq

					eleduration = GetElementDuration(div2eleDom, fr, tr);
					p2duration = 0;

					if(eleduration == 0){
						DOMNodeList *p2items = div2eleDom->getElementsByTagName(XMLString::transcode("p"));
						p2duration=0;
						for(int k=0; k < p2items->getLength(); k++){

							DOMElement* p2eleDom = dynamic_cast<DOMElement*>(p2items->item(k));
							QString p2time = XMLString::transcode(p2eleDom->getAttribute(XMLString::transcode("timeContainer"))); //p2time: par/seq

							eleduration = GetElementDuration(p2eleDom, fr, tr);
							spanduration = 0;

							if(eleduration == 0){
								DOMNodeList *spanitems = p2eleDom->getElementsByTagName(XMLString::transcode("span"));

								for(int l=0; l < spanitems->getLength(); l++){

									DOMElement* spaneleDom = dynamic_cast<DOMElement*>(spanitems->item(l));

									eleduration = GetElementDuration(spaneleDom, fr, tr);
									if (p2time == "seq")
										spanduration =spanduration + eleduration;
									else{
										if (eleduration > spanduration)
											spanduration = eleduration;
									}
								}
								eleduration=0;
							}

							if (div2time == "seq")
								p2duration = p2duration + eleduration + spanduration;
							else{
								if (spanduration > p2duration)
									p2duration = spanduration;

								if (eleduration > p2duration)
									p2duration = eleduration;
							}
						}
						eleduration=0;
					}

					if (divtime == "seq")
						div2duration = div2duration + eleduration + p2duration;
					else{
						if (p2duration > div2duration)
							div2duration = p2duration;

						if (eleduration > div2duration)
							div2duration = eleduration;
					}
				}
				eleduration=0;

				DOMNodeList *pitems = diveleDom->getElementsByTagName(XMLString::transcode("p"));
				for(int m=0; m < pitems->getLength(); m++){
					DOMElement* peleDom = dynamic_cast<DOMElement*>(pitems->item(m));
					QString comp2 = XMLString::transcode(peleDom->getParentNode()->getParentNode()->getNodeName());
					if(comp2 == "body"){
						QString ptime = XMLString::transcode(peleDom->getAttribute(XMLString::transcode("timeContainer")));	//ptime: par/seq
						eleduration = GetElementDuration(peleDom, fr,tr);
						spanduration = 0;

						if(eleduration == 0){
							DOMNodeList *spanitems = peleDom->getElementsByTagName(XMLString::transcode("span"));

							for(int n=0; n < spanitems->getLength(); n++){

								DOMElement* spaneleDom = dynamic_cast<DOMElement*>(spanitems->item(n));

								eleduration = GetElementDuration(spaneleDom, fr, tr);
								if (ptime == "seq")
									spanduration =spanduration + eleduration;
								else{
									if (eleduration > spanduration)
										spanduration = eleduration;
								}
							}
							eleduration=0;
						}

						if (divtime == "seq")
							pduration = pduration + eleduration + spanduration;
						else{
							if (spanduration > pduration)
								pduration = spanduration;

							if (eleduration > pduration)
								pduration = eleduration;
						}
					}
					eleduration=0;
				}

				if(divtime =="seq")
					divduration = divduration + pduration + div2duration;
				else{
					if (div2duration > pduration)
						divduration = div2duration;
					else
						divduration = pduration;
				}
			}

			if (bodytime == "seq")
				duration = duration + eleduration + divduration;
			else{
				if (divduration > eleduration)
					duration = divduration;

				else
					duration = eleduration;
			}
		}
	}
	return duration;
}

Error MetadataExtractor::ReadTimedTextMetadata(Metadata &rMetadata, const QFileInfo &rSourceFile) {

	Metadata metadata(Metadata::TimedText);
	Error error;

	metadata.fileName = rSourceFile.fileName();
	metadata.filePath = rSourceFile.filePath();

	try {
		XMLPlatformUtils::Initialize();
	}
	catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		qDebug() << "Error during initialization! :\n" << message << "\n";
		XMLString::release(&message);
		error = (Error::Unknown);
		error.AppendErrorDescription(XMLString::transcode(toCatch.getMessage()));
		return error;
	}

	XercesDOMParser *parser = new XercesDOMParser();
	ErrorHandler *errHandler = (ErrorHandler*) new HandlerBase();

	parser->setValidationScheme(XercesDOMParser::Val_Always);		//TODO:Schema validation
	parser->setValidationSchemaFullChecking(true);
	parser->useScanner(XMLUni::fgWFXMLScanner);
	parser->setErrorHandler(errHandler);
	parser->setDoNamespaces(true);
	parser->setDoSchema(true);



	//convert QString to char*
	std::string fname = rSourceFile.absoluteFilePath().toStdString();
	char* xmlFile = new char [fname.size()+1];
	std::strcpy( xmlFile, fname.c_str() );

	DOMDocument *dom_doc = 0;
	try {

		parser->parse(xmlFile);
		dom_doc = parser->getDocument();

		DOMNodeList *ttitem = dom_doc->getElementsByTagName(XMLString::transcode("tt"));
		//Check if <tt> is present
		if (ttitem->getLength() == 0) {
			error = (Error::XMLSchemeError);
			return error;
		}
		//write <tt> from DOMNodelist to DOMElement
		DOMElement* tteleDom = dynamic_cast<DOMElement*>(ttitem->item(0));

		//Profile Extractor
		// First have a look if ttp:profile attribute is present
		QString profile = XMLString::transcode(tteleDom->getAttributeNS(IMSC1_NS_TTP, XMLString::transcode("profile")));

		//TO-DO: Checking for urn:ebu:tt:distribution:2014-01

		if (profile.isEmpty()) {
			profile = "http://www.w3.org/ns/ttml/profile/imsc1/text";
			DOMNodeList	*divitems = dom_doc->getElementsByTagName(XMLString::transcode("div"));
			//Check if <smpte:backgroundImage> is present
			if (divitems->getLength() != 0) {
				// Checking the first div element only
				DOMElement* backgroundImage_eleDom = dynamic_cast<DOMElement*>(divitems->item(0));
				QString backgroundImage = XMLString::transcode(backgroundImage_eleDom->getAttributeNS(IMSC1_NS_SMPTE, XMLString::transcode("backgroundImage")));
				if (!backgroundImage.isEmpty()) {
					qDebug() << "backgroundImage found, setting profile to imsc1/image.";
					profile = "http://www.w3.org/ns/ttml/profile/imsc1/image";
				}
			}
		}
		metadata.profile = profile;

		//Frame Rate Multiplier Extractor
		QString mult = XMLString::transcode(tteleDom->getAttributeNS(IMSC1_NS_TTP, XMLString::transcode("frameRateMultiplier")));
		float num = 1;
		float den = 1;
		if (!mult.isEmpty()){
			num = mult.section(" ", 0, 0).toInt();
			den = mult.section(" ", 1, 1).toInt();
		}

		//Frame Rate Extractor
		QString fr = XMLString::transcode(tteleDom->getAttributeNS(IMSC1_NS_TTP, XMLString::transcode("frameRate")));
		int editrate = 30;					//editrate is for the metadata object (expects editrate*numerator, denominator)
		float framerate = 30*(num/den);		//framerate is for calculating the duration, we need the fractal editrate!
		if(!fr.isEmpty()){
			framerate = fr.toFloat()*(num/den);
			editrate = fr.toInt();
		}

		//Sub-framerate Extractor - we'll ignore it, since Subframerate is prohibited in IMSC1!
		int subFrameRate = 1;
		//QString sfr = XMLString::transcode(tteleDom->getAttribute(XMLString::transcode("ttp:subFrameRate")));
		//if (!sfr.isEmpty()) subFrameRate = sfr.toInt();

		//Tick Rate Extractor
		int tickrate = 1; //TTML1 section 6.2.10
		QString tr = XMLString::transcode(tteleDom->getAttributeNS(IMSC1_NS_TTP, XMLString::transcode("tickRate")));
		if (!tr.isEmpty())
			tickrate = tr.toInt();
		else if (!fr.isEmpty())  //TTML1 section 6.2.10
			tickrate = ceil (framerate * subFrameRate);

		//Duration Extractor
		DOMNodeList	*bodyitem = dom_doc->getElementsByTagName(XMLString::transcode("body"));
		//Check if <body> is present
		if (bodyitem->getLength() == 0) {
			error = (Error::XMLSchemeError);
			return error;
		}
		float duration;
		duration = DurationExtractor(dom_doc,framerate,tickrate);

		metadata.effectiveFrameRate = EditRate(editrate*num, den);  // Eff. FrameRate in IMSC1 and MXF
		metadata.originalDuration = Duration(ceil(duration * metadata.effectiveFrameRate.GetQuotient())); // MXF Duration
		metadata.editRate = mCplEditRate;
		metadata.duration = Duration(ceil(duration * metadata.editRate.GetQuotient()));

	}
	catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		qDebug() << message << "\n";
		XMLString::release(&message);
		error = (Error::Unknown);
		error.AppendErrorDescription(XMLString::transcode(toCatch.getMessage()));
		return error;
	}
	catch (const DOMException& toCatch) {
		char* message = XMLString::transcode(toCatch.msg);
		qDebug() << message << "\n";
		XMLString::release(&message);
		error = (Error::Unknown);
		error.AppendErrorDescription(XMLString::transcode(toCatch.getMessage()));
		return error;
	}
	catch (const SAXParseException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		qDebug() << message << "\n";
		XMLString::release(&message);
		error = (Error::XMLSchemeError);
		error.AppendErrorDescription(XMLString::transcode(toCatch.getMessage()));
		return error;
	}
    catch (...) {
    	qDebug() << "Unexpected Exception" ;
    	error = (Error::Unknown);
    	error.AppendErrorDescription("Unexpected Exception");
        return error;
    }

	rMetadata = metadata;
    delete parser;
    delete errHandler;
    XMLPlatformUtils::Terminate();
	return error;
}
			/* -----Denis Manthey----- */


