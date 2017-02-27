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
#include "AS_DCP.h"
#include "global.h"
#include <QtGlobal>
#include <QString>
#include <QStringList>
#include <QObject>
#include <QPair>
#include <QVector>
#include "openjpeg.h" // (k)


class Duration;
namespace ASDCP {
	namespace MXF {
		class VersionType;
	}
}

typedef unsigned int Channels;

class SoundfieldGroup {

public:
	// IMF
	static const SoundfieldGroup SoundFieldGroupNone; // pseudo soundfield group
	static const SoundfieldGroup SoundFieldGroupST;
	static const SoundfieldGroup SoundFieldGroupDM;
	static const SoundfieldGroup SoundFieldGroup30;
	static const SoundfieldGroup SoundFieldGroup40;
	static const SoundfieldGroup SoundFieldGroup50;
	static const SoundfieldGroup SoundFieldGroup60;
	static const SoundfieldGroup SoundFieldGroup70;
	static const SoundfieldGroup SoundFieldGroupLtRt;
	static const SoundfieldGroup SoundFieldGroup51EX;
	static const SoundfieldGroup SoundFieldGroupHA;
	static const SoundfieldGroup SoundFieldGroupVA;
	// DCP
	static const SoundfieldGroup SoundFieldGroup51;
	static const SoundfieldGroup SoundFieldGroup71;
	static const SoundfieldGroup SoundFieldGroupSDS;
	static const SoundfieldGroup SoundFieldGroup61;
	static const SoundfieldGroup SoundFieldGroupM;

	enum eChannel {
		ChannelM1 = (1u << 0), // 1
		ChannelM2 = (1u << 1), // 2
		ChannelLt = (1u << 2), // 4
		ChannelRt = (1u << 3), // 8
		ChannelLst = (1u << 4), // 16
		ChannelRst = (1u << 5), // 32
		ChannelS = (1u << 6), // 64
		ChannelL = (1u << 7), //128
		ChannelR = (1u << 8), //256
		ChannelC = (1u << 9), // 512
		ChannelLFE = (1u << 10), // 1024
		ChannelLs = (1u << 11), // 2048
		ChannelRs = (1u << 12), // 4096
		ChannelLss = (1u << 13), // 8192
		ChannelRss = (1u << 14), // 16384
		ChannelLrs = (1u << 15), // 32768
		ChannelRrs = (1u << 16), // 65536
		ChannelLc = (1u << 17), //131072
		ChannelRc = (1u << 18), // 262144
		ChannelCs = (1u << 19), // 524288
		ChannelHI = (1u << 20), // 1048576
		ChannelVIN = (1u << 21), // 2097152
	};
	//! Initialized with SoundFieldGroupNone.
	SoundfieldGroup() { *this = SoundFieldGroupNone; }
	~SoundfieldGroup() {}
	//! Returns allowed channel names for this soundfield group.
	QStringList GetAdmittedChannelNames() const;
	//! The name of the soundfield group.
	QString GetName() const { return mName; }
	//! Check if all allowed channels are assigned.
	bool IsComplete() const;
	//! Check if it's not SoundFieldGroupNone.
	bool IsWellKnown() const;
	//! Returns the number of allowed channels for this soundfield group.
	int GetChannelCount() const { return mChannels.size(); }
	//! Adds a channel identified by its name (e.g. Mono One). If a unsupported channel is added false is returned.
	bool AddChannel(int index, const QString &rChannelName);
	//! Adds a channel identified by its enum. If a unsupported channel is added false is returned.
	bool AddChannel(int index, const eChannel channelName);
	//! Returns a channel identified by its name (e.g. Mono One) at index.
	QString GetChannelName(int index) const;
	//! Returns a channel identified by its enum at index.
	eChannel GetChannel(int Indexes) const;
	//! Removes all assigned channels.
	void FlushChannels() { mChannels.clear(); }
	//! Returns the current channel assignment as a string.
	QString GetAsString() const;
	//! Convenience function: Returns a well known soundfield group (no channels assigned).
	static SoundfieldGroup GetSoundFieldGroup(const QString &rSoundFieldGroupName);
	//! Convenience function: Returns the names of all well known soundfield groups.
	static QStringList GetSoundFieldGroupNames();
	//! Compares soundfield symbols only. Assigned channels are ignored.
	bool operator==(const SoundfieldGroup& rOther) const;
	//! Compares soundfield symbols only. Assigned channels are ignored.
	bool operator!=(const SoundfieldGroup& rOther) const;

	//! Has to be complient with eChannels (order and count)!!!
	static const QList<QPair<QString, QString> >	mChannelNamesSymbolsMap;

private:
	SoundfieldGroup(const QString &rSymbol, const QString &rName, int admittedChannelCount, Channels admittedChannelSymbols) :
		mSymbol(rSymbol), mName(rName), mAdmittedChannels(admittedChannelSymbols), mChannels(admittedChannelCount, static_cast<eChannel>(0x00)) {

		SoundfieldGroup::mMap.push_back(this);
	}

	QString GetChannelSymbol(eChannel channel) const;
	QString GetChannelName(eChannel channel) const;
	eChannel GetChannelForName(const QString &rChannelName) const;
	eChannel GetChannelForSymbol(const QString &rChannelSymbol) const;

	QString mSymbol;
	QString mName;
	Channels mAdmittedChannels;
	QVector<eChannel> mChannels;
	static QList<SoundfieldGroup*> mMap;
};


class MarkerLabel {

public:

	static const MarkerLabel MarkerLabelNone;
	static const MarkerLabel MarkerLabelFFBT;
	static const MarkerLabel MarkerLabelFFCB;
	static const MarkerLabel MarkerLabelFFCL;
	static const MarkerLabel MarkerLabelFFDL;
	static const MarkerLabel MarkerLabelFFEC;
	static const MarkerLabel MarkerLabelFFHS;
	static const MarkerLabel MarkerLabelFFMC;
	static const MarkerLabel MarkerLabelFFOB;
	static const MarkerLabel MarkerLabelFFOC;
	static const MarkerLabel MarkerLabelFFOI;
	static const MarkerLabel MarkerLabelFFSP;
	static const MarkerLabel MarkerLabelFFTC;
	static const MarkerLabel MarkerLabelFFTS;
	static const MarkerLabel MarkerLabelFTXC;
	static const MarkerLabel MarkerLabelFTXE;
	static const MarkerLabel MarkerLabelFTXM;
	static const MarkerLabel MarkerLabelLFBT;
	static const MarkerLabel MarkerLabelLFCB;
	static const MarkerLabel MarkerLabelLFCL;
	static const MarkerLabel MarkerLabelLFDL;
	static const MarkerLabel MarkerLabelLFEC;
	static const MarkerLabel MarkerLabelLFHS;
	static const MarkerLabel MarkerLabelLFMC;
	static const MarkerLabel MarkerLabelLFOB;
	static const MarkerLabel MarkerLabelLFOC;
	static const MarkerLabel MarkerLabelLFOI;
	static const MarkerLabel MarkerLabelLFSP;
	static const MarkerLabel MarkerLabelLFTC;
	static const MarkerLabel MarkerLabelLFTS;
	static const MarkerLabel MarkerLabelLTXC;
	static const MarkerLabel MarkerLabelLTXE;
	static const MarkerLabel MarkerLabelLTXM;
	static const MarkerLabel MarkerLabelFPCI;
	static const MarkerLabel MarkerLabelFFCO;
	static const MarkerLabel MarkerLabelLFCO;
	static const MarkerLabel MarkerLabelFFOA;
	static const MarkerLabel MarkerLabelLFOA;
	//WR
	static const MarkerLabel MarkerLabelFFDC;
	static const MarkerLabel MarkerLabelLFDC;

	//! Initialized with MarkerLabelNone.
	MarkerLabel() { *this = MarkerLabelNone; }
	//! Used for custom marker label.
	MarkerLabel(const QString &rLabel, const QString &rDescription, const QString &rScope) :
		mLabel(rLabel), mDescription(rDescription), mScope(rScope) {
		MarkerLabel::mMap.push_back(this);
	}
	~MarkerLabel() {}
	QString GetLabel() const { return mLabel; }
	QString GetDescription() const { return mDescription; }
	QString GetScope() const { return mScope; }
	//! Check if it's not MarkerLabelNone or a custom label.
	bool IsWellKnown() const;
	//! Convenience function: Returns the names of all well known marker.
	static QStringList GetMarkerLabels();
	static MarkerLabel GetMarker(const QString &rMarkerLabel);
private:
	MarkerLabel(const QString &rLabel, const QString &rDescription) :
		mLabel(rLabel), mDescription(rDescription), mScope(WELL_KNOWN_MARKER_LABEL_SCOPE_2013) {

		MarkerLabel::mMap.push_back(this);
	}

	QString mLabel;
	QString mDescription;
	QString mScope;
	static QList<MarkerLabel*> mMap;
};


class EditRate {

public:
	// frame rates
	static const EditRate EditRate23_98;
	static const EditRate EditRate24;
	static const EditRate EditRate25;
	static const EditRate EditRate29_97;
	static const EditRate EditRate30;
	static const EditRate EditRate48;
	static const EditRate EditRate50;
	static const EditRate EditRate59_94;
	static const EditRate EditRate60;
	static const EditRate EditRate96;
	static const EditRate EditRate100;
	static const EditRate EditRate119_88;
	static const EditRate EditRate120;
	// audio sampling rates
	static const EditRate EditRate48000;
	static const EditRate EditRate96000;

	EditRate(qint32 n, qint32 d);
	EditRate(const ASDCP::Rational &rRational);
	EditRate() : mNumerator(0), mDenominator(0), mName() {}
	qreal GetQuotient() const { return (qreal)mNumerator / (qreal)mDenominator; }
	qint32 GetRoundendQuotient() const { return (qint32)(GetQuotient() + .5); }
	qreal GetEditUnit() const { return (qreal)mDenominator / (qreal)mNumerator; }
	qint32 GetNumerator() const { return mNumerator; }
	qint32 GetDenominator() const { return mDenominator; }
	QString GetName() const { return mName; }
	QString GetRoundedName() const { return QString::number(GetRoundendQuotient()); }
	bool IsValid() const { return (mNumerator > 0 && mDenominator > 0); }
	bool operator==(const EditRate& rhs) const;
	bool operator!=(const EditRate& rhs) const;
	bool operator<(const EditRate& rhs) const;
	bool operator>(const EditRate& rhs) const;
	EditRate& operator=(const EditRate& other) {
		mNumerator = other.GetNumerator();
		mDenominator = other.GetDenominator();
		mName = other.GetName();
	    return *this;
	}

	//! Convenience function: Returns a well known edit rate.
	static EditRate GetEditRate(const QString &rEditRateName);
	//! Convenience function: Returns the names of all well known frame rates.
	static QStringList GetFrameRateNames();

private:
	EditRate(qint32 n, qint32 d, const QString &rName) : mNumerator(n), mDenominator(d), mName(rName) {

		EditRate::mMap.push_back(this);
	}

	qint32									mNumerator;
	qint32									mDenominator;
	QString									mName;
	static QList<EditRate*>	mMap;
};

//! Represents the number of frames or samples between two time codes (e.g.: 00:00:00:00 - 00:00:00:30 ---> Dur.: 30 frames).
class Duration {

public:
	//! Generates Null duration.
	Duration() : mSamplesFrames(0) {}
	Duration(qint64 samplesOrFrames) : mSamplesFrames(samplesOrFrames) {}
	~Duration() {}
	bool IsValid() const { return (mSamplesFrames >= 0 ? true : false); }
	bool IsNull() const { return (mSamplesFrames == 0 ? true : false); }
	QString GetAsString(const EditRate &rEditRate) const;
	qint64 GetCount() const { return mSamplesFrames; }
	void SetCount(qint64 samplesOrFrames) { mSamplesFrames = samplesOrFrames; }
	bool operator==(const Duration &rOther) const { return mSamplesFrames == rOther.mSamplesFrames; }
	bool operator!=(const Duration &rOther) const { return mSamplesFrames != rOther.mSamplesFrames; }
	bool operator<(const Duration &rOther) const { return mSamplesFrames < rOther.mSamplesFrames; }
	bool operator>(const Duration &rOther) const { return mSamplesFrames > rOther.mSamplesFrames; }
	bool operator<=(const Duration &rOther) const { return mSamplesFrames <= rOther.mSamplesFrames; }
	bool operator>=(const Duration &rOther) const { return mSamplesFrames >= rOther.mSamplesFrames; }
	Duration operator+(const Duration &rOther) const { return Duration(mSamplesFrames + rOther.mSamplesFrames); }
	Duration operator-(const Duration &rOther) const { return Duration(mSamplesFrames - rOther.mSamplesFrames); }
	Duration& operator+=(const Duration &rOther) { mSamplesFrames += rOther.mSamplesFrames; return *this; }
	Duration& operator-=(const Duration &rOther) { mSamplesFrames -= rOther.mSamplesFrames; return *this; }

private:
	qint64 mSamplesFrames;
};

Duration operator*(int i, const Duration &rOther);
Duration operator*(const Duration &rOther, int i);


//! Negative time codes exist and can be calculated with but can not be printed (Timecode::GetAsString()). A negative time code is printed as time code -00:00:00:00.
class Timecode {

public:
	Timecode(const EditRate &rEditRate, qint64 hours, qint64 minutes, qint64 seconds, qint64 frames);
	Timecode(const EditRate &rEditRate, qint64 frames);
	Timecode();
	//! Duration is the number of frames between 00:00:00:00 and the constructed time code.
	explicit Timecode(const EditRate &rEditRate, Duration duration);
	~Timecode() {}
	bool IsValid() const { return (mFramesCount >= 0 && mEditRate.IsValid() ? true : false); }
	bool IsNull() const { return (mFramesCount == 0 ? true : false); }
	//! A negative time code is returned as time code -00:00:00:00.
	QString GetAsString() const;
	//WR
	QString GetFramesAsString() const;
	//! Use place marker %1, %2, %3, %4. %1 = hours, %2 = minutes, %3 = seconds, %5 = frames. A negative time code is returned as time code -00:00:00:00.
	QString GetAsString(const QString &rMarker) const;
	qint64 GetHours() const { return (mFramesCount >= 0 && mEditRate.IsValid() ? mFramesCount / (3600 * mEditRate.GetRoundendQuotient()) % 60 : 0); }
	qint64 GetMinutes() const { return (mFramesCount >= 0 && mEditRate.IsValid() ? mFramesCount / (60 * mEditRate.GetRoundendQuotient()) % 60 : 0); }
	qint64 GetSeconds() const { return (mFramesCount >= 0 && mEditRate.IsValid() ? mFramesCount / mEditRate.GetRoundendQuotient() % 60 : 0); }
	float GetSecondsF() const { return (mFramesCount / mEditRate.GetQuotient()); } // (k)
	qint64 GetFrames() const { return (mFramesCount >= 0 && mEditRate.IsValid() ? mFramesCount % mEditRate.GetRoundendQuotient() : 0); }
	qint64 GetOverallFrames() const { return mFramesCount; }
	//! The frame this time code points at. 
	qint64 GetTargetFrame() const;
	//! The positive duration between 00:00:00:00 and *this.
	Duration AsPositiveDuration() const;
	//! The positive duration between rOther and *this.
	Duration AsPositiveDuration(const Timecode &rOther) const;
	//! The duration between 00:00:00:00 and *this.
	Duration AsDuration() const;
	//! The duration between rOther and *this.
	Duration AsDuration(const Timecode &rOther) const;

	const EditRate& GetEditRate() const;
	bool operator==(const Timecode &rOther) const { return mFramesCount == rOther.mFramesCount; }
	bool operator!=(const Timecode &rOther) const { return mFramesCount != rOther.mFramesCount; }
	bool operator<(const Timecode &rOther) const { return mFramesCount < rOther.mFramesCount; }
	bool operator>(const Timecode &rOther) const { return mFramesCount > rOther.mFramesCount; }
	bool operator<=(const Timecode &rOther) const { return mFramesCount <= rOther.mFramesCount; }
	bool operator>=(const Timecode &rOther) const { return mFramesCount >= rOther.mFramesCount; }
	Timecode operator+(const Timecode &rRight);
	Timecode operator+(const Duration &rRight);
	Timecode operator-(const Timecode &rRight);
	Timecode operator-(const Duration &rRight);
	Timecode& operator++(); // prefix
	Timecode operator++(int unused); // postfix

private:
	EditRate mEditRate;
	qint64 mFramesCount;
};

typedef struct {
	QString id;
	bool alwaysActive;
	float origin[2]; // %
	float extent[2]; // %
	QImage bgImage; // background image
	QImage bgImageScaled;
	QColor bgColor; // background color
	QMap<QString, QString> CSS; // ["background-color"] = "#ff8000"
} TTMLRegion;

typedef struct {
	int type; // 0 = text, 1 = image
	bool error; // some error ocurred with the content (e.g. no image found)
	QImage bgImage;
	float beg; // (sec. rel. to timeline)
	float end; // (sec. rel. to timeline)
	QString text;
	TTMLRegion region;
	QMap<QString, QString> CSS; // ["background-color"] = "#ff8000"
} TTMLelem;

typedef struct {
	float timeline_in; // e.g. 3.123 seconds
	float timeline_out;
	float in; // e.g. 0.123 seconds
	float out;
	float frameRate;
	QVector<TTMLelem> items;
	QString doc; // serialized ttml document
	int track_index;
} TTMLtimelineResource;

typedef struct {
	int decoded_total = 0;
	int pending_requests = 0;
} DecodedFrames;

typedef struct {
	QString formatted_time; // hh:mm:ss
	QString fractional_frames; // ff.f
	QVector<TTMLelem> elements;
	TTMLtimelineResource resource;
} visibleTTtrack;
// (k) - end

Q_DECLARE_METATYPE(EditRate);
Q_DECLARE_METATYPE(SoundfieldGroup);
Q_DECLARE_METATYPE(Timecode);
Q_DECLARE_METATYPE(Duration);
