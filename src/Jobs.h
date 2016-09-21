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
#pragma once
#include "JobQueue.h"
#include "info.h"
#include "ImfCommon.h"


namespace
{

class Info : public ASDCP::WriterInfo {
public:
	Info() {
		static byte_t default_ProductUUID_Data[ASDCP::UUIDlen] =
		{0x7d, 0x83, 0x6e, 0x16, 0x37, 0xc7, 0x4c, 0x22,
		0xb2, 0xe0, 0x46, 0xa7, 0x17, 0xe8, 0x4f, 0x42};
		memcpy(ProductUUID, default_ProductUUID_Data, ASDCP::UUIDlen);
		CompanyName = "HSRM";
		ProductName = PROJECT_NAME;
		ProductVersion = VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH;
		LabelSetType = ASDCP::LS_MXF_SMPTE;
		EncryptedEssence = false;
		UsesHMAC = false;
	}
};

} // namespace

class JobCalculateHash : public AbstractJob {

	Q_OBJECT

public:
	JobCalculateHash(const QString &rSourceFile);
	virtual ~JobCalculateHash() {}

signals:
	void Result(const QByteArray &rHash, const QVariant &rIdentifier = QVariant());

protected:
	virtual Error Execute();

private:
	Q_DISABLE_COPY(JobCalculateHash);

	const QString mSourceFile;
};


class JobWrapWav : public AbstractJob {

	Q_OBJECT

public:
	JobWrapWav(const QStringList &rSourceFiles, const QString &rOutputFile, const SoundfieldGroup &rSoundFieldGroup, const QUuid &rAssetId, const QString &rLanguageTag);
	virtual ~JobWrapWav() {}

protected:
	virtual Error Execute();

private:
	Q_DISABLE_COPY(JobWrapWav);

	const QString mOutputFile;
	const QStringList mSourceFiles;
	const SoundfieldGroup	mSoundFieldGoup;
	//WR
	const QString mLanguageTag;
	//WR
	Info mWriterInfo;
};






class JobWrapTimedText : public AbstractJob {

	Q_OBJECT

public:
	JobWrapTimedText(const QStringList &rSourceFiles, const QString &rOutputFile, const EditRate &rEditRate, const Duration &rDuration, const QUuid &rAssetId, const QString &rProfile, const EditRate &rFrameRate, const QString &rLanguageTag);
	virtual ~JobWrapTimedText() {}

protected:
	virtual Error Execute();

private:
	Q_DISABLE_COPY(JobWrapTimedText);

	const QString mOutputFile;
	const QStringList mSourceFiles;
	const QString mProfile;
	const EditRate mEditRate; //Based on milliseconds
	const Duration mDuration;
	const EditRate mFrameRate; //Based on real Framerate

	Info mWriterInfo;
	//WR
	const QString mLanguageTag;
	//WR
};

class JobExtractEssenceDescriptor : public AbstractJob {

	Q_OBJECT

public:
	JobExtractEssenceDescriptor(const QString &rSourceFile);
	virtual ~JobExtractEssenceDescriptor() {}

signals:
	void Result(const QString &qresult, const QVariant &rIdentifier = QVariant());

protected:
	virtual Error Execute();

private:
	Q_DISABLE_COPY(JobExtractEssenceDescriptor);

	const QString mSourceFile;

};
