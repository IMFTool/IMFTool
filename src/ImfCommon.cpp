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
#include "ImfCommon.h"
#include "MXFTypes.h"
#include <cstdlib>
#include <QStringList>


bool SoundfieldGroup::AddChannel(int index, const QString &rChannelName) {

	eChannel channel = GetChannelForName(rChannelName);
	if((mAdmittedChannels & channel) && index >= 0 && index < mChannels.size()) {
		mChannels.replace(index, channel);
		return true;
	}
	return false;
}

bool SoundfieldGroup::AddChannel(int index, const eChannel channelName) {

	if((mAdmittedChannels & channelName) && index >= 0 && index < mChannels.size()) {
		mChannels.replace(index, channelName);
		return true;
	}
	return false;
}

QString SoundfieldGroup::GetChannelName(int index) const {

	if(index < mChannels.size() && index >= 0) {
		return GetChannelName(mChannels.at(index));
	}
	return QString();
}

QString SoundfieldGroup::GetAsString() const {

	QString ret(mSymbol);
	ret.append("(");
	for(int i = 0; i < mChannels.size(); i++) {
		ret.append(GetChannelSymbol(mChannels.at(i)));
		if(i < mChannels.size() - 1)ret.append(",");
	}
	ret.append(")");
	return ret;
}

QStringList SoundfieldGroup::GetAdmittedChannelNames() const {

	QStringList admitted_channel_names;
	if(mAdmittedChannels & ChannelM1) admitted_channel_names.append(GetChannelName(ChannelM1));
	if(mAdmittedChannels & ChannelM2) admitted_channel_names.append(GetChannelName(ChannelM2));
	if(mAdmittedChannels & ChannelLt) admitted_channel_names.append(GetChannelName(ChannelLt));
	if(mAdmittedChannels & ChannelRt) admitted_channel_names.append(GetChannelName(ChannelRt));
	if(mAdmittedChannels & ChannelLst) admitted_channel_names.append(GetChannelName(ChannelLst));
	if(mAdmittedChannels & ChannelRst) admitted_channel_names.append(GetChannelName(ChannelRst));
	if(mAdmittedChannels & ChannelS) admitted_channel_names.append(GetChannelName(ChannelS));
	if(mAdmittedChannels & ChannelL) admitted_channel_names.append(GetChannelName(ChannelL));
	if(mAdmittedChannels & ChannelR) admitted_channel_names.append(GetChannelName(ChannelR));
	if(mAdmittedChannels & ChannelC) admitted_channel_names.append(GetChannelName(ChannelC));
	if(mAdmittedChannels & ChannelLFE) admitted_channel_names.append(GetChannelName(ChannelLFE));
	if(mAdmittedChannels & ChannelLs) admitted_channel_names.append(GetChannelName(ChannelLs));
	if(mAdmittedChannels & ChannelRs) admitted_channel_names.append(GetChannelName(ChannelRs));
	if(mAdmittedChannels & ChannelLss) admitted_channel_names.append(GetChannelName(ChannelLss));
	if(mAdmittedChannels & ChannelRss) admitted_channel_names.append(GetChannelName(ChannelRss));
	if(mAdmittedChannels & ChannelLrs) admitted_channel_names.append(GetChannelName(ChannelLrs));
	if(mAdmittedChannels & ChannelRrs) admitted_channel_names.append(GetChannelName(ChannelRrs));
	if(mAdmittedChannels & ChannelLc) admitted_channel_names.append(GetChannelName(ChannelLc));
	if(mAdmittedChannels & ChannelRc) admitted_channel_names.append(GetChannelName(ChannelRc));
	if(mAdmittedChannels & ChannelCs) admitted_channel_names.append(GetChannelName(ChannelCs));
	if(mAdmittedChannels & ChannelHI) admitted_channel_names.append(GetChannelName(ChannelHI));
	if(mAdmittedChannels & ChannelVIN) admitted_channel_names.append(GetChannelName(ChannelVIN));
	return admitted_channel_names;
}

SoundfieldGroup SoundfieldGroup::GetSoundFieldGroup(const QString &rSoundFieldGroupName) {

	SoundfieldGroup ret = SoundfieldGroup::SoundFieldGroupNone;
	for(int i = 0; i < SoundfieldGroup::mMap.size(); i++) {
		if(SoundfieldGroup::mMap.at(i) && rSoundFieldGroupName.compare(SoundfieldGroup::mMap.at(i)->GetName(), Qt::CaseSensitive) == 0) ret = *SoundfieldGroup::mMap.at(i);
	}
	return ret;
}

QStringList SoundfieldGroup::GetSoundFieldGroupNames() {

	QStringList ret;
	for(int i = 0; i < SoundfieldGroup::mMap.size(); i++) {
		if(SoundfieldGroup::mMap.at(i)) ret.push_back(SoundfieldGroup::mMap.at(i)->GetName());
	}
	return ret;
}

bool SoundfieldGroup::IsComplete() const {

	if(mAdmittedChannels == 0x00) return false; // SoundFieldGroupNone is always incomplete.
	Channels channels = 0x00;
	for(int i = 0; i < mChannels.size(); i++) {
		channels |= mChannels.at(i);
	}
	return (channels & mAdmittedChannels) == mAdmittedChannels;
}

bool SoundfieldGroup::IsWellKnown() const {

	bool ret = true;
	if(mSymbol.compare(SoundfieldGroup::SoundFieldGroupNone.mSymbol) == 0) ret = false;
	return ret;
}

bool SoundfieldGroup::operator==(const SoundfieldGroup& rOther) const {

	bool ret = false;
	if(mSymbol.compare(rOther.mSymbol) == 0) ret = true;
	return ret;
}

bool SoundfieldGroup::operator!=(const SoundfieldGroup& rOther) const {

	return !(*this == rOther);
}

SoundfieldGroup::eChannel SoundfieldGroup::GetChannelForName(const QString &rChannelName) const {

	for(int i = 0; i < mChannelNamesSymbolsMap.size(); i++) {
		if(mChannelNamesSymbolsMap.at(i).first.compare(rChannelName) == 0) return static_cast<eChannel>(1U << i);
	}
	return static_cast<eChannel>(0x00);
}

SoundfieldGroup::eChannel SoundfieldGroup::GetChannelForSymbol(const QString &rChannelSymbol) const {

	for(int i = 0; i < mChannelNamesSymbolsMap.size(); i++) {
		if(mChannelNamesSymbolsMap.at(i).second.compare(rChannelSymbol) == 0) return static_cast<eChannel>(1U << i);
	}
	return static_cast<eChannel>(0x00);
}

QString SoundfieldGroup::GetChannelSymbol(eChannel channel) const {

	if(channel == 0x00) return QString();
	int i = 0;
	int loc_channel = channel;
	for(; loc_channel != 0; i++) {
		loc_channel = channel >> i;
	}
	return mChannelNamesSymbolsMap.at(i - 2).second;
}

QString SoundfieldGroup::GetChannelName(eChannel channel) const {

	if(channel == 0x00) return QString();
	int i = 0;
	int loc_channel = channel;
	for(; loc_channel != 0; i++) {
		loc_channel = channel >> i;
	}
	return mChannelNamesSymbolsMap.at(i - 2).first;
}

SoundfieldGroup::eChannel SoundfieldGroup::GetChannel(int Indexes) const {

	if(Indexes < mChannels.count() && Indexes >= 0) return mChannels.at(Indexes);
	return static_cast<eChannel>(0x00);
}

QList<SoundfieldGroup*> SoundfieldGroup::mMap;
// IMF
const SoundfieldGroup SoundfieldGroup::SoundFieldGroupNone("None", "No Soundfield", 0, 0x00);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroupVA("VA", "Visual Accessibility", 1, ChannelVIN);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroupHA("HA", "Hearing Accessibility", 1, ChannelHI);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroup51EX("51EX", "5.1EX", 6, ChannelL | ChannelC | ChannelR | ChannelLst | ChannelRst | ChannelLFE);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroupLtRt("LtRt", "Lt-Rt", 2, ChannelLt | ChannelRt);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroup70("70", "7.0DS", 7, ChannelL | ChannelC | ChannelR | ChannelLss | ChannelRss | ChannelLrs | ChannelRrs); // TODO: Typo in SMPTE ST 2067-8:2013?
const SoundfieldGroup SoundfieldGroup::SoundFieldGroup60("60", "6.0", 6, ChannelL | ChannelC | ChannelR | ChannelLs | ChannelRs | ChannelCs);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroup50("50", "5.0", 5, ChannelL | ChannelC | ChannelR | ChannelLs | ChannelRs);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroup40("40", "4.0", 4, ChannelL | ChannelC | ChannelR | ChannelS);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroup30("30", "3.0", 3, ChannelL | ChannelC | ChannelR);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroupDM("DM", "Dual Mono", 2, ChannelM1 | ChannelM2);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroupST("ST", "Standard Stereo", 2, ChannelL | ChannelR);
// DCP
const SoundfieldGroup SoundfieldGroup::SoundFieldGroup51("51", "5.1", 6, ChannelL | ChannelC | ChannelR | ChannelLs | ChannelRs | ChannelLFE);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroup71("71", "7.1DS", 8, ChannelL | ChannelC | ChannelR | ChannelLss | ChannelRss | ChannelLrs | ChannelRrs | ChannelLFE);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroupSDS("SDS", "7.1SDS", 8, ChannelL | ChannelLc | ChannelC | ChannelRc | ChannelR | ChannelLs | ChannelRs | ChannelLFE);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroup61("61", "6.1", 7, ChannelL | ChannelR | ChannelC | ChannelLss | ChannelRss | ChannelCs | ChannelLFE);
const SoundfieldGroup SoundfieldGroup::SoundFieldGroupM("M", "1.0 Monaural", 1, ChannelC);


const QList<QPair<QString, QString> > SoundfieldGroup::mChannelNamesSymbolsMap = QList<QPair<QString, QString> >()
// IMF
<< QPair<QString, QString>("Mono One", "M1")
<< QPair<QString, QString>("Mono Two", "M2")
<< QPair<QString, QString>("Left Total", "Lt")
<< QPair<QString, QString>("Right Total", "Rt")
<< QPair<QString, QString>("Left Surround Total", "Lst")
<< QPair<QString, QString>("Right Surround Total", "Rst")
<< QPair<QString, QString>("Surround", "S")
// DCP
<< QPair<QString, QString>("Left", "L")
<< QPair<QString, QString>("Right", "R")
<< QPair<QString, QString>("Center", "C")
<< QPair<QString, QString>("LFE", "LFE")
<< QPair<QString, QString>("Left Sourround", "Ls")
<< QPair<QString, QString>("Right Surround", "Rs")
<< QPair<QString, QString>("Left Side Surround", "Lss")
<< QPair<QString, QString>("Right Side Surround", "Rss")
<< QPair<QString, QString>("Left Rear Surround", "Lrs")
<< QPair<QString, QString>("Right Rear Surround", "Rrs")
<< QPair<QString, QString>("Left Center", "Lc")
<< QPair<QString, QString>("Right Center", "Rc")
<< QPair<QString, QString>("Center Surround", "Cs")
<< QPair<QString, QString>("Hearing Impaired", "HI")
<< QPair<QString, QString>("Visually Impaired Narrative", "VIN");

Timecode::Timecode(const EditRate &rEditRate, qint64 hours, qint64 minutes, qint64 seconds, qint64 frames) :
mEditRate(rEditRate), mFramesCount((seconds + minutes * 60 + hours * 3600) * mEditRate.GetRoundendQuotient() + frames) {

}

Timecode::Timecode(const EditRate &rEditRate, Duration duration /*= 0*/) :
mEditRate(rEditRate), mFramesCount(duration.GetCount()) {

}

Timecode::Timecode(const EditRate &rEditRate, qint64 frames) :
mEditRate(rEditRate), mFramesCount(frames) {

}

Timecode::Timecode() :
mEditRate(EditRate()), mFramesCount(0) {

}

Timecode Timecode::operator+(const Timecode &rRight) {

	if(mEditRate != rRight.mEditRate) {
		qCritical() << "Timcode values with different Edit Rates detected.";
		return *this;
	}
	Timecode result(*this);
	result.mFramesCount += rRight.mFramesCount;
	return result;
}

Timecode Timecode::operator+(const Duration &rRight) {

	Timecode result(*this);
	result.mFramesCount += rRight.GetCount();
	return result;
}

Timecode Timecode::operator-(const Timecode &rRight) {

	if(mEditRate != rRight.mEditRate) {
		qCritical() << "Timcode values with different Edit Rates detected.";
		return *this;
	}
	Timecode result(*this);
	result.mFramesCount -= rRight.mFramesCount;
	return result;
}

Timecode Timecode::operator-(const Duration &rRight) {

	Timecode result(*this);
	result.mFramesCount -= rRight.GetCount();
	return result;
}

Duration Timecode::AsPositiveDuration() const {

	return std::abs(mFramesCount);
}

Duration Timecode::AsPositiveDuration(const Timecode &rOther) const {

	return std::abs(mFramesCount - rOther.mFramesCount);
}

Timecode& Timecode::operator++() {

	mFramesCount += 1;
	return *this;
}

Timecode Timecode::operator++(int unused) {

	Timecode result = *this;
	++(*this);
	return result;
}

QString Timecode::GetAsString() const {

	QString ret;
	if(mFramesCount < 0) ret.append("-");
	return ret.append(QString("%1:%2:%3:%4").arg(GetHours(), 2, 10, QChar('0')).arg(GetMinutes(), 2, 10, QChar('0')).arg(GetSeconds(), 2, 10, QChar('0')).arg(GetFrames(), 2, 10, QChar('0')));
}

QString Timecode::GetFramesAsString() const {

	QString ret;
	if(mFramesCount < 0) ret.append("-");
	return ret.append(QString("%1").arg((mFramesCount < 0 ? 0 : mFramesCount), 6, 10, QChar('0')));
}


QString Timecode::GetAsString(const QString &rMarker) const {

	// TODO: improve
	QString ret;
	QStringList list = rMarker.split("%", QString::SkipEmptyParts);
	if(mFramesCount < 0) ret.append("-");
	for(int i = 0; i < list.size(); i++) {
		if(list.at(i).startsWith(QChar('1'))) ret.append(QString("%").append(list.at(i)).arg(GetHours(), 2, 10, QChar('0')));
		else if(list.at(i).startsWith(QChar('2'))) ret.append(QString("%").append(list.at(i)).arg(GetMinutes(), 2, 10, QChar('0')));
		else if(list.at(i).startsWith(QChar('3'))) ret.append(QString("%").append(list.at(i)).arg(GetSeconds(), 2, 10, QChar('0')));
		else if(list.at(i).startsWith(QChar('4'))) ret.append(QString("%").append(list.at(i)).arg(GetFrames(), 2, 10, QChar('0')));
	}
	return ret;
}

const EditRate& Timecode::GetEditRate() const {

	return mEditRate;
}

qint64 Timecode::GetTargetFrame() const {

	qint64 target_frame = mFramesCount;
	if (mFramesCount < 0) target_frame = 0;
	return target_frame;
}

Duration Timecode::AsDuration() const {

	return mFramesCount;
}

Duration Timecode::AsDuration(const Timecode &rOther) const {

	return (mFramesCount - rOther.mFramesCount);
}

QList<EditRate*> EditRate::mMap;
const EditRate EditRate::EditRate23_98(24000, 1001, "23.976");
const EditRate EditRate::EditRate24(24, 1, "24");
const EditRate EditRate::EditRate25(25, 1, "25");
const EditRate EditRate::EditRate29_97(30000, 1001, "29.97");
const EditRate EditRate::EditRate30(30, 1, "30");
const EditRate EditRate::EditRate48(48, 1, "48");
const EditRate EditRate::EditRate50(50, 1, "50");
const EditRate EditRate::EditRate59_94(60000, 1001, "59.94");
const EditRate EditRate::EditRate60(60, 1, "60");
const EditRate EditRate::EditRate96(96, 1, "96");
const EditRate EditRate::EditRate100(100, 1, "100");
const EditRate EditRate::EditRate119_88(120000, 1001, "119.88");
const EditRate EditRate::EditRate120(120, 1, "120");

const EditRate EditRate::EditRate48000(48000, 1, "48000");
const EditRate EditRate::EditRate96000(96000, 1, "96000");

EditRate::EditRate(qint32 n, qint32 d) : mName() {

	mNumerator = (d == 1000) ? n/1000 : n;
	mDenominator = (d == 1000) ? 1 : d;

	for(int i = 0; i < EditRate::mMap.size(); i++) {
		EditRate *p_edit_rate = EditRate::mMap.at(i);
		if(p_edit_rate && p_edit_rate->mNumerator == mNumerator && p_edit_rate->mDenominator == mDenominator) mName = p_edit_rate->mName;
	}
}

EditRate::EditRate(const ASDCP::Rational &rRational) : mNumerator(rRational.Numerator), mDenominator(rRational.Denominator), mName() {

	for(int i = 0; i < EditRate::mMap.size(); i++) {
		EditRate *p_edit_rate = EditRate::mMap.at(i);
		if(p_edit_rate && p_edit_rate->mNumerator == rRational.Numerator && p_edit_rate->mDenominator == rRational.Denominator) mName = p_edit_rate->mName;
	}
}

bool EditRate::operator==(const EditRate& rhs) const {

	return (rhs.mNumerator == mNumerator && rhs.mDenominator == mDenominator);
}

bool EditRate::operator!=(const EditRate& rhs) const {

	return (rhs.mNumerator != mNumerator || rhs.mDenominator != mDenominator);
}

bool EditRate::operator<(const EditRate& rhs) const {

	if(mNumerator < rhs.mNumerator) return true;
	if(mNumerator == rhs.mNumerator && mDenominator < rhs.mDenominator) return true;
	return false;
}

bool EditRate::operator>(const EditRate& rhs) const {

	if(mNumerator > rhs.mNumerator) return true;
	if(mNumerator == rhs.mNumerator && mDenominator > rhs.mDenominator) return true;
	return false;
}

EditRate EditRate::GetEditRate(const QString &rEditRateName) {

	EditRate ret;
	for(int i = 0; i < EditRate::mMap.size(); i++) {
		if(EditRate::mMap.at(i) && rEditRateName.compare(EditRate::mMap.at(i)->GetName(), Qt::CaseSensitive) == 0) ret = *EditRate::mMap.at(i);
	}
	return ret;
}

QStringList EditRate::GetFrameRateNames() {

	QStringList ret;
	for(int i = 0; i < EditRate::mMap.size(); i++) {
		if(EditRate::mMap.at(i) && *EditRate::mMap.at(i) != EditRate48000 && *EditRate::mMap.at(i) != EditRate96000) ret.push_back(EditRate::mMap.at(i)->GetName());
	}
	return ret;
}

QString Duration::GetAsString(const EditRate &rEditRate) const {

	return Timecode(rEditRate, 0, 0, 0, mSamplesFrames).GetAsString();
}

Duration operator*(int i, const Duration &rOther) {

	return Duration(rOther.GetCount() * i);
}

Duration operator*(const Duration &rOther, int i) {

	return Duration(rOther.GetCount() * i);
}

QList<MarkerLabel*> MarkerLabel::mMap;
const MarkerLabel MarkerLabel::MarkerLabelNone("None", "No Label", "");
const MarkerLabel MarkerLabel::MarkerLabelFFBT("FFBT", "First Frame of Bars and Tone");
const MarkerLabel MarkerLabel::MarkerLabelFFCB("FFCB", "First Frame of Commercial Blacks");
const MarkerLabel MarkerLabel::MarkerLabelFFCL("FFCL", "First Frame of Company/Production Logo");
const MarkerLabel MarkerLabel::MarkerLabelFFDL("FFDL", "First Frame of Distribution Logo");
const MarkerLabel MarkerLabel::MarkerLabelFFEC("FFEC", "First Frame of End Credits. First displayable frame of content that contains any intensity of the End Credits (a non zero alpha value), which appear at the end of a feature.");
const MarkerLabel MarkerLabel::MarkerLabelFFHS("FFHS", "First Frame of Head Slate");
const MarkerLabel MarkerLabel::MarkerLabelFFMC("FFMC", "First displayable frame of content that contains any intensity of moving, rolling or scrolling credits (a non-zero alpha value), which appear at the end of the feature.");
const MarkerLabel MarkerLabel::MarkerLabelFFOB("FFOB", "First Frame of Ratings Band. First displayable frame of content of the Rating Band, which is usually a slate at the beginning of a feature.");
const MarkerLabel MarkerLabel::MarkerLabelFFOC("FFOC", "First Frame of Composition. The first frame of a composition that is intended for display.");
const MarkerLabel MarkerLabel::MarkerLabelFFOI("FFOI", "First Frame of Intermission.");
const MarkerLabel MarkerLabel::MarkerLabelFFSP("FFSP", "First Frame of Digital Sync Pop");
const MarkerLabel MarkerLabel::MarkerLabelFFTC("FFTC", "First Frame of Title Credits. First displayable frame of content that contains any intensity of the Title Credits (a non zero alpha value), which appear at the beginning of a feature.");
const MarkerLabel MarkerLabel::MarkerLabelFFTS("FFTS", "First Frame of Tail Slate");
const MarkerLabel MarkerLabel::MarkerLabelFTXC("FTXC", "First Frame of Textless Title Credits");
const MarkerLabel MarkerLabel::MarkerLabelFTXE("FTXE", "First Frame of Textless End Credits");
const MarkerLabel MarkerLabel::MarkerLabelFTXM("FTXM", "First Frame of Textless Material Segment");
const MarkerLabel MarkerLabel::MarkerLabelLFBT("LFBT", "Last Frame of Bars and Tone");
const MarkerLabel MarkerLabel::MarkerLabelLFCB("LFCB", "Last Frame of Commercial Blacks");
const MarkerLabel MarkerLabel::MarkerLabelLFCL("LFCL", "Last Frame of Company/Production Logo");
const MarkerLabel MarkerLabel::MarkerLabelLFDL("LFDL", "Last Frame of Distribution Logo");
const MarkerLabel MarkerLabel::MarkerLabelLFEC("LFEC", "Last Frame of End Credits. Last displayable frame of content that contains any intensity of the End Credits (a non zero alpha value), which appear at the end of a feature.");
const MarkerLabel MarkerLabel::MarkerLabelLFHS("LFHS", "Last Frame of Head Slate");
const MarkerLabel MarkerLabel::MarkerLabelLFMC("LFMC", "Last displayable frame of content that contains any intensity of moving, rolling or scrolling credits (a non-zero alpha value), which appear at the end of the feature.");
const MarkerLabel MarkerLabel::MarkerLabelLFOB("LFOB", "Last Frame of Ratings Band. Last displayable frame of content of the Rating Band, which is usually a slate at the beginning of a feature.");
const MarkerLabel MarkerLabel::MarkerLabelLFOC("LFOC", "Last Frame of Composition. The last frame of a composition that is intended for display.");
const MarkerLabel MarkerLabel::MarkerLabelLFOI("LFOI", "Last Frame of Intermission.");
const MarkerLabel MarkerLabel::MarkerLabelLFSP("LFSP", "Last Frame of Digital Sync Pop");
const MarkerLabel MarkerLabel::MarkerLabelLFTC("LFTC", "Last Frame of Title Credits. Last displayable frame of content that contains any intensity of the Title Credits (a non zero alpha value), which appear at the beginning of a feature.");
const MarkerLabel MarkerLabel::MarkerLabelLFTS("LFTS", "Last Frame of Tail Slate");
const MarkerLabel MarkerLabel::MarkerLabelLTXC("LTXC", "Last frame of Textless Title Credits");
const MarkerLabel MarkerLabel::MarkerLabelLTXE("LTXE", "Last Frame of Textless End Credits");
const MarkerLabel MarkerLabel::MarkerLabelLTXM("LTXM", "Last frame of Textless Material Segment");
const MarkerLabel MarkerLabel::MarkerLabelFPCI("FPCI", "Fixed Point Candidate Insertion. Indicates possible point in the timeline where it would be allowable to insert content downstrean. This is for material that may not have commercial blacks, but could indicate a candidate point where a commercial could be inserted.");
const MarkerLabel MarkerLabel::MarkerLabelFFCO("FFCO", "First Frame of Candidate Overlay. First frame of a sequence of frames where overlays, e.g. commercial overlays, may be placed.");
const MarkerLabel MarkerLabel::MarkerLabelLFCO("LFCO", "Last Frame of Candidate Overlay. Last frame of a sequence of frames where overlays, e.g. commercial overlays, may be placed.");
const MarkerLabel MarkerLabel::MarkerLabelFFOA("FFOA", "Audio First Frame. First frame of audio ring-in/ring-out where the video is in black.");
const MarkerLabel MarkerLabel::MarkerLabelLFOA("LFOA", "Audio Last Frame. Last frame of audio ring-in/ring-out where the video is in black.");
//Markers from ST 2067-3:2016
const MarkerLabel MarkerLabel::MarkerLabelFFDC("FFDC", "First Frame of Dubbing Credits. First displayable frame of content that contains any intensity of dubbing credits.", WELL_KNOWN_MARKER_LABEL_SCOPE_2016);
const MarkerLabel MarkerLabel::MarkerLabelLFDC("LFDC", "Last Frame of Dubbing credits: Last displayable frame of content that contains any intensity of dubbing credits.", WELL_KNOWN_MARKER_LABEL_SCOPE_2016);

bool MarkerLabel::IsWellKnown() const {
	bool retValue = false;
	if (mScope.compare(WELL_KNOWN_MARKER_LABEL_SCOPE_2013) == 0 && GetMarker(this->GetLabel()).GetLabel().compare(MarkerLabel::MarkerLabelNone.GetLabel()) != 0)
		retValue = true;
	if (mScope.compare(WELL_KNOWN_MARKER_LABEL_SCOPE_2016) == 0 && GetMarker(this->GetLabel()).GetLabel().compare(MarkerLabel::MarkerLabelNone.GetLabel()) != 0)
		retValue = true;

	return retValue; //mScope.compare(WELL_KNOWN_MARKER_LABEL_SCOPE) == 0 && GetMarker(this->GetLabel()).GetLabel().compare(MarkerLabel::MarkerLabelNone.GetLabel()) != 0;
}

QStringList MarkerLabel::GetMarkerLabels() {

	QStringList ret;
	for(int i = 0; i < mMap.size(); i++) {
		ret << mMap.at(i)->GetLabel();
	}
	return ret;
}

MarkerLabel MarkerLabel::GetMarker(const QString &rMarkerLabel) {

	MarkerLabel ret = MarkerLabel::MarkerLabelNone;
	for(int i = 0; i < mMap.size(); i++) {
		if(rMarkerLabel.compare(mMap.at(i)->GetLabel()) == 0) return *mMap.at(i);
	}
	return ret;
}
