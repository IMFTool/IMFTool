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
#include "MetadataExtractorCommon.h"
#include <QTextCursor>
#include <QTextTable>
#include <QFontMetrics>
#include <QAbstractTextDocumentLayout>
#include "SMPTE_Labels.h"

Metadata::Metadata(eEssenceType rType /*= Unknown_Type*/, eEssenceSubType rSubType /*= Unknown_Type*/) :
type(rType),
subType(rSubType),
editRate(), // frame rate for video and sampling rate for audio milliseconds for timed text
aspectRatio(),
storedWidth(0),
storedHeight(0),
displayWidth(0),
displayHeight(0),
colorEncoding(Metadata::Unknown_Color_Encoding),
horizontalSubsampling(0),
componentDepth(0),
duration(),
audioChannelCount(0),
audioQuantization(0),
soundfieldGroup(),
fileName(),
filePath(),
profile(),
//WR
languageTag(""),
effectiveFrameRate(),
originalDuration(),
componentMinRef(0),
componentMaxRef(0)
//WR
{
}

QString Metadata::GetAsString() {

	QString ret(QObject::tr("Essence Type: "));
	switch(type) {
#ifdef APP5_ACES
		case Metadata::Aces:
#endif
#ifdef CODEC_HTJ2K
		case Metadata::HTJ2K:
#endif
		case Metadata::Jpeg2000:
		case Metadata::ProRes:
			if(type == Metadata::Jpeg2000)	{ if(subType == Metadata::eEssenceSubType::HTJ2K)		ret.append(QObject::tr("%1").arg("HTJ2K\n")); else ret.append(QObject::tr("%1").arg("JPEG2000\n")); }
#ifdef APP5_ACES
			else if(type == Metadata::Aces)										ret.append(QObject::tr("%1").arg("ACES\n"));
#endif
#ifdef CODEC_HTJ2K
			else if(type == Metadata::HTJ2K)										ret.append(QObject::tr("%1").arg("HTJ2K\n"));
#endif
			else if(type == Metadata::ProRes)						ret.append(QObject::tr("%1").arg("ProRes\n"));
			if(duration.IsValid() && editRate.IsValid())	ret.append(QObject::tr("Duration: %1 frames\n").arg(duration.GetCount()));
			if(editRate.IsValid() == true)								ret.append(QObject::tr("Frame Rate: %1\n").arg(editRate.GetQuotient()));
			if(storedHeight != 0 || storedWidth != 0)			ret.append(QObject::tr("Stored Resolution: %1 x %2\n").arg(storedWidth).arg(storedHeight));
			if(displayHeight != 0 || displayWidth != 0)		ret.append(QObject::tr("Displayed Resolution: %1 x %2\n").arg(displayWidth).arg(displayHeight));
			if(aspectRatio != ASDCP::Rational())					ret.append(QObject::tr("Aspect Ratio: %1 (%2:%3)\n").arg(aspectRatio.Quotient()).arg(aspectRatio.Numerator).arg(aspectRatio.Denominator));
			if(horizontalSubsampling != 0 && colorEncoding != Unknown_Color_Encoding) {
				if(colorEncoding == Metadata::RGBA)					ret.append(QObject::tr("Color Mode: %1").arg("RGB"));
				else if(colorEncoding == Metadata::CDCI)		ret.append(QObject::tr("Color Mode: %1").arg("YCbCr"));
				ret.append(QObject::tr("(%1:%2:%3)\n").arg(4).arg(4 / horizontalSubsampling).arg(4 / horizontalSubsampling));
			}
			if (componentDepth == 253)
				ret.append(QObject::tr("Color Depth: 16 bit float\n").arg(componentDepth));
			else if (componentDepth != 0)
				ret.append(QObject::tr("Color Depth: %1 bit\n").arg(componentDepth));
			break;
		case Metadata::Pcm:
			ret.append(QObject::tr("%1").arg("Pcm\n"));
			if(duration.IsValid() == false && editRate.IsValid())	ret.append(QObject::tr("Duration: %1 samples\n").arg(duration.GetAsString(editRate)));
			if(editRate.IsValid() == true)								ret.append(QObject::tr("Sample Rate: %1 Hz\n").arg(editRate.GetQuotient()));
			if(audioQuantization != 0)										ret.append(QObject::tr("Bit Depth: %1 bit\n").arg(audioQuantization));
			if(audioChannelCount != 0)										ret.append(QObject::tr("Channels: %1\n").arg(audioChannelCount));
			ret.append(QObject::tr("Channel Configuration: %1\n").arg(soundfieldGroup.GetName()));
			//WR
			if(!languageTag.isEmpty()) ret.append(QObject::tr("Language: %1\n").arg(languageTag));
			//WR
			break;
		case Metadata::TimedText:
			ret.append(QObject::tr("%1").arg("Timed Text\n"));
			if (editRate.IsValid()) {
				ret.append(QObject::tr("Duration: %1 frames\n").arg(duration.GetCount()));
				ret.append(QObject::tr("Edit Rate: %1 fps\n").arg(editRate.GetQuotient()));
			} else {
				ret.append(QObject::tr("Duration: unknown\n"));
				ret.append(QObject::tr("Edit Rate: not set\n"));
			}
			if (effectiveFrameRate != editRate) ret.append(QObject::tr("TTML Frame Rate: %1 fps\n").arg(effectiveFrameRate.GetQuotient()));
			ret.append(QObject::tr("Profile: %1\n").arg(profile.remove(0, 40)));
			//WR
			if(!languageTag.isEmpty()) ret.append(QObject::tr("Language: %1\n").arg(languageTag));
			//WR
			break;
	default:
			ret = QObject::tr("Unknown\n");
			break;
	}
	ret.chop(1); // remove last \n
	return ret;
}

void Metadata::GetAsTextDocument(QTextDocument &rDoc) {

	rDoc.setDocumentMargin(2.5);
	QTextCursor cursor(&rDoc);

	QTextTableFormat tableFormat;
	tableFormat.setBorder(.5);
	tableFormat.setCellSpacing(0);
	tableFormat.setCellPadding(2);
	tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
	qreal column_width = (rDoc.size().width() - rDoc.documentMargin() * 2. - tableFormat.border() * 6. - tableFormat.cellSpacing() * 3. /*documentation image wrong?*/) / 2.;
	QVector<QTextLength> columnConstraints;
	columnConstraints << QTextLength(QTextLength::FixedLength, column_width);
	columnConstraints << QTextLength(QTextLength::FixedLength, column_width);
	tableFormat.setColumnWidthConstraints(columnConstraints);

	int column_text_width = column_width - tableFormat.cellPadding() * 2.;
	switch(rDoc.defaultTextOption().wrapMode()) {
		case QTextOption::WordWrap:
		case QTextOption::WrapAnywhere:
		case QTextOption::WrapAtWordBoundaryOrAnywhere:
			column_text_width = 2000;
			break;
		default:
			break;
	}

	QFontMetrics font_metrics(rDoc.defaultFont());

	if(type == Metadata::Jpeg2000 || type == Metadata::ProRes
#ifdef APP5_ACES
	|| type == Metadata::Aces
#endif
#ifdef CODEC_HTJ2K
	|| type == Metadata::HTJ2K
#endif
	) {

		QTextTable *table = cursor.insertTable(5, 2, tableFormat);
		switch(type) {
#ifdef APP5_ACES
			case Metadata::Aces:																						table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("ACES"), Qt::ElideRight, column_text_width)); break;
#endif
#ifdef CODEC_HTJ2K
			case Metadata::HTJ2K:																						table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("HTJ2K"), Qt::ElideRight, column_text_width)); break;
#endif
			case Metadata::Jpeg2000:
				if (subType == Metadata::eEssenceSubType::HTJ2K)
					table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("HTJ2K"), Qt::ElideRight, column_text_width));
				else
					table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("JPEG2000"), Qt::ElideRight, column_text_width));
			break;
			case Metadata::ProRes:																				table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("ProRes"), Qt::ElideRight, column_text_width)); break;
			default:																												table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: Unknown"), Qt::ElideRight, column_text_width)); break;
		}
		if(duration.IsValid() && editRate.IsValid())											table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: %1 frames").arg(duration.GetCount()), Qt::ElideRight, column_text_width));
		else																															table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: Unknown"), Qt::ElideRight, column_text_width));
		if(editRate.IsValid())																						table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: %1 fps").arg(editRate.GetQuotient()), Qt::ElideRight, column_text_width));
		else																															table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Frame Rate: Unknown"), Qt::ElideRight, column_text_width));
		if(storedHeight != 0 || storedWidth != 0)													table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Stored Resolution: %1 x %2").arg(storedWidth).arg(storedHeight), Qt::ElideRight, column_text_width));
		else																															table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Stored Resolution: Unknown"), Qt::ElideRight, column_text_width));
		if(aspectRatio != ASDCP::Rational())															table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Aspect Ratio: %1 (%2:%3)").arg(aspectRatio.Quotient()).arg(aspectRatio.Numerator).arg(aspectRatio.Denominator), Qt::ElideRight, column_text_width));
		else																															table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Aspect Ratio: Unknown"), Qt::ElideRight, column_text_width));
		if(displayHeight != 0 || displayWidth != 0)												table->cellAt(2, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Displayed Resolution: %1 x %2").arg(displayWidth).arg(displayHeight), Qt::ElideRight, column_text_width));
		else																															table->cellAt(2, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Displayed Resolution: Unknown"), Qt::ElideRight, column_text_width));
		if(horizontalSubsampling != 0 && colorEncoding != Metadata::Unknown_Color_Encoding) {
			if(colorEncoding == Metadata::RGBA)															table->cellAt(3, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Color Mode: %1(%2:%3:%4)").arg("RGB").arg(4).arg(4 / horizontalSubsampling).arg(4 / horizontalSubsampling), Qt::ElideRight, column_text_width));
			else if(colorEncoding == Metadata::CDCI)												table->cellAt(3, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Color Mode: %1(%2:%3:%4)").arg("YCbCr").arg(4).arg(4 / horizontalSubsampling).arg(4 / horizontalSubsampling), Qt::ElideRight, column_text_width));
		}
		else																															table->cellAt(3, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Color Mode: Unknown"), Qt::ElideRight, column_text_width));
		if(componentDepth == 253)																						table->cellAt(3, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Color Depth: 16 bit float"), Qt::ElideRight, column_text_width));
		else if(componentDepth != 0)																						table->cellAt(3, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Color Depth: %1 bit").arg(componentDepth), Qt::ElideRight, column_text_width));
		else																															table->cellAt(3, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Color Depth: Unknown"), Qt::ElideRight, column_text_width));

		// (k) - start
		if (SMPTE::vColorPrimaries.contains(colorPrimaries)) {
			table->cellAt(4, 0).firstCursorPosition().insertText(font_metrics.elidedText(QString("Primaries: %1").arg(SMPTE::vColorPrimaries[colorPrimaries]), Qt::ElideRight, column_text_width));
		}

		if (SMPTE::vTransferCharacteristic.contains(transferCharcteristics)) {
			table->cellAt(4, 1).firstCursorPosition().insertText(font_metrics.elidedText(QString("OETF: %1").arg(SMPTE::vTransferCharacteristic[transferCharcteristics]), Qt::ElideRight, column_text_width));
		}
		// (k) - end
	}
	else if(type == Metadata::Pcm) {

		QTextTable *table = cursor.insertTable(4, 2, tableFormat);
		switch(type) {
			case Metadata::Pcm:																							table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("PCM"), Qt::ElideRight, column_text_width)); break;
			default:																												table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: Unknown"), Qt::ElideRight, column_text_width)); break;
		}
		if(duration.IsValid() && editRate.IsValid())											table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: %1 samples").arg(duration.GetCount()), Qt::ElideRight, column_text_width));
		else																															table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: Unknown"), Qt::ElideRight, column_text_width));
		if(editRate.IsValid())																						table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Sample Rate: %1 Hz").arg(editRate.GetQuotient()), Qt::ElideRight, column_text_width));
		else																															table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Sample Rate: Unknown"), Qt::ElideRight, column_text_width));
		if(audioQuantization != 0)																				table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Bit Depth: %1 bit").arg(audioQuantization), Qt::ElideRight, column_text_width));
		else																															table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Bit Depth: Unknown"), Qt::ElideRight, column_text_width));
		if(audioChannelCount != 0)																				table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Channels: %1").arg(audioChannelCount), Qt::ElideRight, column_text_width));
		else																															table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Channels: Unknown"), Qt::ElideRight, column_text_width));
		table->cellAt(2, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Channel Configuration: %1").arg(soundfieldGroup.GetName()), Qt::ElideRight, column_text_width));
		//WR
		if(!languageTag.isEmpty()) table->cellAt(3, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Language: %1").arg(languageTag), Qt::ElideRight, column_text_width));
		//WR
	}
	else if(type == Metadata::TimedText) {

		QTextTable *table = cursor.insertTable(4, 2, tableFormat);
		if(is_ttml_file(filePath))
			table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Source File Name: %1").arg(fileName), Qt::ElideRight, column_text_width));
		table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("Timed Text"), Qt::ElideRight, column_text_width));
		table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Profile: %1").arg(tt_profile_is_text ? "Text" : "Image"), Qt::ElideRight, column_text_width));

		if (editRate.IsValid()) {
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: %1 frames").arg(duration.GetCount()), Qt::ElideRight, column_text_width));
			table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: %1 fps").arg(editRate.GetQuotient()), Qt::ElideRight, column_text_width));
		} else {
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: unknown"), Qt::ElideRight, column_text_width));
			table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: not set"), Qt::ElideRight, column_text_width));
		}
/*
		if (effectiveFrameRate != editRate)
			table->cellAt(2, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("TTML Frame Rate: %1 fps").arg(effectiveFrameRate.GetQuotient()), Qt::ElideRight, column_text_width));
*/
		//WR
		if(!languageTag.isEmpty()) table->cellAt(3, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Language: %1").arg(languageTag), Qt::ElideRight, column_text_width));
		//WR
	}
	else if(type == Metadata::ISXD) {

		QTextTable *table = cursor.insertTable(4, 2, tableFormat);
		//if(is_ttml_file(filePath))
			//table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Source File Name: %1").arg(fileName), Qt::ElideRight, column_text_width));
		table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("ISXD"), Qt::ElideRight, column_text_width));

		if (editRate.IsValid()) {
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: %1 frames").arg(duration.GetCount()), Qt::ElideRight, column_text_width));
			table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: %1 fps").arg(editRate.GetQuotient()), Qt::ElideRight, column_text_width));
		} else {
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: unknown"), Qt::ElideRight, column_text_width));
			table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: not set"), Qt::ElideRight, column_text_width));
		}
		//WR
		if(!namespaceURI.isEmpty()) {
			table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("NSp: %1").arg(namespaceURI), Qt::ElideRight, column_text_width));
		}
		//WR
	}
	else if(type == Metadata::IAB) {

		QTextTable *table = cursor.insertTable(4, 2, tableFormat);
		//if(is_ttml_file(filePath))
			//table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Source File Name: %1").arg(fileName), Qt::ElideRight, column_text_width));
		table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("IAB"), Qt::ElideRight, column_text_width));

		if (editRate.IsValid()) {
			table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: %1 frames").arg(duration.GetCount()), Qt::ElideRight, column_text_width));
			table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: %1 fps").arg(editRate.GetQuotient()), Qt::ElideRight, column_text_width));
		} else {
			table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: unknown"), Qt::ElideRight, column_text_width));
			table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: not set"), Qt::ElideRight, column_text_width));
		}
		if(audioQuantization != 0)
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Bit Depth: %1 bit").arg(audioQuantization), Qt::ElideRight, column_text_width));
		else
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Bit Depth: Unknown"), Qt::ElideRight, column_text_width));
		table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Sample Rate: %1").arg(audioSamplingRate.GetQuotient()), Qt::ElideRight, column_text_width));
		table->cellAt(3, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Language: %1").arg(languageTag), Qt::ElideRight, column_text_width));
	}
	else if(type == Metadata::SADM) {

		QTextTable *table = cursor.insertTable(4, 2, tableFormat);
		table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("MGA S-ADM"), Qt::ElideRight, column_text_width));

		if (editRate.IsValid()) {
			table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: %1 frames").arg(duration.GetCount()), Qt::ElideRight, column_text_width));
			table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: %1 fps").arg(editRate.GetQuotient()), Qt::ElideRight, column_text_width));
		} else {
			table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: unknown"), Qt::ElideRight, column_text_width));
			table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: not set"), Qt::ElideRight, column_text_width));
		}
		if(audioQuantization != 0)
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Bit Depth: %1 bit").arg(audioQuantization), Qt::ElideRight, column_text_width));
		else
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Bit Depth: Unknown"), Qt::ElideRight, column_text_width));
		table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Sample Rate: %1").arg(audioSamplingRate.GetQuotient()), Qt::ElideRight, column_text_width));
		table->cellAt(3, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Number of Soundfields: %1").arg(mgaSoundFieldGroupList.size()), Qt::ElideRight, column_text_width));
	}
	else if(type == Metadata::ADM) {

		QTextTable *table = cursor.insertTable(4, 2, tableFormat);
		table->cellAt(0, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Essence Type: %1").arg("ADM Audio"), Qt::ElideRight, column_text_width));

		if (editRate.IsValid()) {
			table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: %1 samples").arg(duration.GetCount()), Qt::ElideRight, column_text_width));
			table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: %1 fps").arg(editRate.GetQuotient()), Qt::ElideRight, column_text_width));
		} else {
			table->cellAt(0, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Duration: unknown"), Qt::ElideRight, column_text_width));
			table->cellAt(1, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Edit Rate: not set"), Qt::ElideRight, column_text_width));
		}
		if(audioQuantization != 0)
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Bit Depth: %1 bit").arg(audioQuantization), Qt::ElideRight, column_text_width));
		else
			table->cellAt(1, 1).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Bit Depth: Unknown"), Qt::ElideRight, column_text_width));
		table->cellAt(2, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Sample Rate: %1").arg(audioSamplingRate.GetQuotient()), Qt::ElideRight, column_text_width));
		table->cellAt(3, 0).firstCursorPosition().insertText(font_metrics.elidedText(QObject::tr("Number of Soundfields: %1").arg(admSoundFieldGroupList.size()), Qt::ElideRight, column_text_width));
	}
}
