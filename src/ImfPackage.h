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
#include "ImfPackageCommon.h"
#include "SMPTE-429-8-2014-AM.h"
#include "SMPTE-429-8-2006-PKL.h"
#include "SMPTE-2067-9a-2018-Sidecar.h"
#include "MetadataExtractor.h"
#include "MetadataExtractorCommon.h"
#include <QObject>
#include <QDir>
#include <QString>
#include <QUuid>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QStringList>
#include <QList>
#include <QByteArray>
#include <QTime>
#include <QSharedPointer>
#include <QImage>
#include <QAbstractTableModel>
#include <QUndoCommand>
#include <QVector>
#include <QItemSelection>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

XERCES_CPP_NAMESPACE_USE


class Asset;
class AssetMap;
class PackingList;
class QAbstractItemModel;
//WR
class AssetScm;
class QMessageBox;
class QProgressDialog;
class JobQueue;
class AssetCpl;
//WR
class JPEG2000; // (k)



class ImfPackage : public QAbstractTableModel {

	Q_OBJECT

public:
	enum eModelColumn {
		ColumnIcon = 0,
		ColumnAssetType,
		ColumnFilePath,
		ColumnFileSize,
		ColumnFinalized,
		ColumnAnnotation,
		ColumnProxyImage,
		ColumnMetadata,
		ColumnMax
	};
	//! Import IMF package. You should invoke ImfPackage::Ingest().
	ImfPackage(const QDir &rWorkingDir);
	//! Create new IMF package.
	ImfPackage(const QDir &rWorkingDir, const UserText &rIssuer, const UserText &rAnnotationText = QString());
	virtual ~ImfPackage() {}
	//! Check if Imf Package is in an unsaved state
	bool IsDirty() const { return mIsDirty; }
	//! Ingests an existing Imf package from file system.
	ImfError Ingest();
	//! Outgests (writes) everything back to file system.
	ImfError Outgest();
	//! Returns the root directory of the current IMF package.
	QDir GetRootDir() { return mRootDir; }
	//! Number of Assets this Imf package holds.
	int GetAssetCount() const { return mAssetList.size(); }
	//! Returns Asset with corresponding Uuid. Returns NULL Pointer if Asset is not found.
	QSharedPointer<Asset> GetAsset(const QUuid &rUuid);
	//! Returns Asset at index. Returns NULL Pointer if Asset is not found.
	QSharedPointer<Asset> GetAsset(int index);
	//! Removes Asset from Asset List.
	void RemoveAsset(const QUuid &rUuid);
	//! Removes Asset from Asset List.
	void RemoveAsset(int index);
	//! Adds Asset. An Asset can only be added once.
	bool AddAsset(const QSharedPointer<Asset> &rAsset, const QUuid &rPackingListId);
	//! Returns next best Packing List if index is 0.
	QUuid GetPackingListId(int index = 0);
	//WR
	QVector<EditRate> GetImpEditRates() const {return mImpEditRates;}
	bool GetIsSupplemental() const { return mIsSupplemental;}
	QVector<QString> GetApplicationIdentificationList();
	//WR

	//! Model View related.
	virtual int rowCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &rIndex, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual Qt::ItemFlags flags(const QModelIndex &rIndex) const;
	virtual bool setData(const QModelIndex &rIndex, const QVariant &rValue, int role = Qt::EditRole);
	virtual QMimeData* mimeData(const QModelIndexList &rIndexes) const;
	virtual Qt::DropActions supportedDropActions() const;
	bool selectedIsOutsidePackage(const QModelIndex &selected);

signals:
	void DirtyChanged(bool isDirty);
	void ImpAssetModified(QSharedPointer<Asset> pAsset);
	void rIsSupplemental(bool isSupplemental);

private slots:
	void rAssetModified(Asset *pAsset);
	//WR
	void rJobQueueFinished();
	//WR

private:
	Q_DISABLE_COPY(ImfPackage);
	PackingList* GetPackingList(const QUuid &rUuid);
	QUuid GetPackingListId(PackingList *pPackingList);
	//! Parses the Ingest Dir (all Assets are added). Expects a valid Asset Map file path.
	ImfError ParseAssetMap(const QFileInfo &rAssetMapFilePath);
	void CheckIfSupplemental();

	AssetMap						*mpAssetMap;
	QList<PackingList*>				mPackingLists;
	QList<QSharedPointer<Asset> >	mAssetList;
	const QDir						mRootDir;
	bool mIsDirty;
	bool mIsIngest; // Used for suppressing DirtyChanged signals during ingest.
	//WR
	QMessageBox *mpMsgBox;
	QProgressDialog *mpProgressDialog;
	JobQueue *mpJobQueue;
	QVector<EditRate> mImpEditRates; //required for creating TT assets
	bool mIsSupplemental = false;
	QList<cpl2016::CompositionPlaylistType> mCplList;
	QList<QSharedPointer<AssetScm> > mScmList;
	//WR
};


class AssetMap : public QObject {

	Q_OBJECT

public:
	//! Import Asset Map.
	AssetMap(ImfPackage *pParent, const QFileInfo &rFilePath, const am::AssetMapType &rAssetMap);
	//! Creates new Asset Map.
	AssetMap(ImfPackage *pParent, const QFileInfo &rFilePath, const UserText &rAnnotationText = QString(), const UserText &rIssuer = QString());
	virtual ~AssetMap() {};
	//! Check if Asset Map physically exists on file system.
	bool Exists() const { return mFilePath.exists() && mFilePath.isFile() && !mFilePath.isSymLink(); }
	QFileInfo GetFilePath() const { return mFilePath; }
	//! Call this function to receive the Dom Tree for serialization.
	const am::AssetMapType& Write();

	QUuid GetId() const { return ImfXmlHelper::Convert(mData.getId()); }
	UserText GetAnnotationText() const { if(mData.getAnnotationText().present() == true) return ImfXmlHelper::Convert(mData.getAnnotationText().get()); else return UserText(); }
	QDateTime GetIssueDate() const { return ImfXmlHelper::Convert(mData.getIssueDate()); }
	UserText GetIssuer() const { return ImfXmlHelper::Convert(mData.getIssuer()); }
	void SetId() {mData.setId(ImfXmlHelper::Convert(QUuid::createUuid()));}

	void SetAnnotationText(const UserText &rAnnotationText) { mData.setAnnotationText(ImfXmlHelper::Convert(rAnnotationText)); }
	void SetIssuer(const UserText &rIssuer) { mData.setIssuer(ImfXmlHelper::Convert(rIssuer)); }

	const QFileInfo		mFilePath;
	am::AssetMapType	mData;
};


class PackingList : public QObject {

	Q_OBJECT

public:
	//! Import Asset Map.
	PackingList(ImfPackage *pParent, const QFileInfo &rFilePath, const pkl2016::PackingListType &rPackingList);
	//! Creates new Asset Map.
	PackingList(ImfPackage *pParent, const QFileInfo &rFilePath, const QUuid &rId, const QUuid &rIconId = QUuid(), const QUuid &rGroupId = QUuid(), const UserText &rAnnotationText = QString(), const UserText &rIssuer = QString());
	virtual ~PackingList() {};
	//! Check if Packing List physically exists on file system.
	bool Exists() const { return mFilePath.exists() && mFilePath.isFile() && !mFilePath.isSymLink(); }
	QFileInfo GetFilePath() const { return mFilePath; }
	const pkl2016::PackingListType& Write();

	QUuid GetId() const { return ImfXmlHelper::Convert(mData.getId()); }
	UserText GetAnnotationText() const { if(mData.getAnnotationText().present() == true) return ImfXmlHelper::Convert(mData.getAnnotationText().get()); else return UserText(); }
	QDateTime GetIssueDate() const { return ImfXmlHelper::Convert(mData.getIssueDate()); }
	UserText GetIssuer() const { return ImfXmlHelper::Convert(mData.getIssuer()); }
	QUuid GetIconId() const { if(mData.getIconId().present() == true) return ImfXmlHelper::Convert(mData.getIconId().get()); else return QUuid(); }
	QUuid GetGroupId() const { if(mData.getGroupId().present() == true) return ImfXmlHelper::Convert(mData.getGroupId().get()); else return QUuid(); }

	void SetAnnotationText(const UserText &rAnnotationText) { mData.setAnnotationText(ImfXmlHelper::Convert(rAnnotationText)); }
	void SetIssuer(const UserText &rIssuer) { mData.setIssuer(ImfXmlHelper::Convert(rIssuer)); }
	void SetIconId(const QUuid &rIconId) { mData.setIconId(ImfXmlHelper::Convert(rIconId)); }
	void SetGroupId(const QUuid &rGroupId) { mData.setGroupId(ImfXmlHelper::Convert(rGroupId)); }

	const QFileInfo				mFilePath;
	pkl2016::PackingListType	mData;
};

//! Represents Entry in Packing List AND Asset Map.
class Asset : public QObject {

	Q_OBJECT

public:
	enum eAssetType {
		mxf = 0,	// application/mxf
		opl,			// text/xml
		cpl,			// text/xml
		pkl,			// text/xml
		scm,			// text/xml
		sidecar,		// multiple
		unknown
	};
	//! Import existing Asset
	Asset(eAssetType type, const QFileInfo &rFilePath, const am::AssetType &rAsset, std::auto_ptr<pkl2016::AssetType> assetType = std::auto_ptr<pkl2016::AssetType>(NULL));
	//! Creates new Asset. rFilePath must be the PROSPECTIVE path of the asset (collisions must be avoided). If the asset doesn't exist on the file system when ImfPackage::Outgest() is invoked the asset will be ignored.
	Asset(eAssetType type, const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText = UserText());
	virtual ~Asset() {}
	//! Check if Asset physically exists on file system.
	bool Exists() const { return mFilePath.exists() && mFilePath.isFile() && !mFilePath.isSymLink(); }
	//! Check if Asset belongs to an Asset Map.
	bool HasAffinity() const { if(mType != Asset::pkl) return mpAssetMap; else return mpAssetMap && mpPackageList; }
	//! Checks saved Hash against new calculated Hash. Useful for imported Assets.
	bool ValidateHash(const QByteArray &rHash) const { return rHash == GetHash(); }
	//! Hashes are calculated externally (time consuming). Check if this Asset needs a new Hash. Set the new Hash using Asset::SetHash().
	bool NeedsNewHash() const { return (mFileNeedsNewHash || GetHash() == QByteArray()); }
	//! Call this function to receive the Dom Tree for serialization.
	const am::AssetType& WriteAm();
	//! Call this function to receive the Dom Tree for serialization.
	const std::auto_ptr<pkl2016::AssetType>& WritePkl();
	QFileInfo GetPath() { return mFilePath; }

	QUuid GetId() const { return ImfXmlHelper::Convert(mAmData.getId()); }
	QUuid GetPklId() const { if(mpPackageList) return mpPackageList->GetId(); else return QUuid(); }
	QUuid GetAmId() const { if(mpAssetMap) return mpAssetMap->GetId(); else return QUuid(); }
	am::AssetType GetAmData() { return mAmData; }
	void SetAmData(am::AssetType rAmData) { mAmData = rAmData; }
	pkl2016::AssetType* GetPklData() { return mpPklData.get(); }

	UserText GetAnnotationText() const;
	QByteArray GetHash() const { if(mpPklData.get()) return ImfXmlHelper::Convert(mpPklData->getHash()); else return QByteArray(); }
	quint64 GetSize() const { if(mpPklData.get()) return mpPklData->getSize(); else return quint64(0); }
	eAssetType GetType() const { return mType; }
	UserText GetOriginalFileName() const { if(mpPklData.get() && mpPklData->getOriginalFileName().present() == true) return ImfXmlHelper::Convert(mpPklData->getOriginalFileName().get()); else return UserText(); }

	void SetAnnotationText(const UserText &rAnnotationText);

	void SetIsOutsidePackage(const bool &rIsOutsidePackage) {mIsOutsidePackage = rIsOutsidePackage; mpAssetMap = 0;} // mpAssetMap = 0 means AffinityLost() = true
	bool GetIsOutsidePackage() const { return mIsOutsidePackage; }

	void SetColor(const QColor rColor) {mColor = rColor;}
	QColor GetColor() const {return mColor;}

	inline bool operator==(const Asset &rOther) const { return GetId() == rOther.GetId(); }
	inline bool operator!=(const Asset &rOther) const { return GetId() != rOther.GetId(); }

	friend bool ImfPackage::AddAsset(const QSharedPointer<Asset> &rAsset, const QUuid &rPackingListId);
	friend void ImfPackage::RemoveAsset(const QUuid &rUuid);

signals:
	/*! This signal hast to be emitted if the asset on the file system has changed (e.g. size changes, asset was deleted, ...) or some metadata have changed (e.g. soundfield group).
	Every time this signal is emitted the model can notify its views to update their data.
	*/
	void AssetModified(Asset *pAsset);

	public slots:
	//! Invoke if the file of the asset is modified externally. Emits Asset::AssetModified().
	void FileModified();
	void SetHash(const QByteArray &rHash);

	private slots:
	void AffinityLost(QObject *pPklOrAm);
	void AffinityWon(QObject *pPklOrAm);
	void rAssetModified(Asset *pAsset);

protected:
	//! Implement to handle the loss of affinity (Asset was removed from Imf Package). Default implementation emits  Asset::AssetModified()
	virtual void HandleAffinitiyLoss() { emit AssetModified(this); }
	//! Implement to handle the increment of affinity (Asset was added to Imf Package). Default implementation emits  Asset::AssetModified()
	virtual void HandleAffinitiyIncrement() { emit AssetModified(this); }

private:
	Q_DISABLE_COPY(Asset);
	AssetMap											*mpAssetMap;
	PackingList										*mpPackageList;
	eAssetType										mType;
	QFileInfo											mFilePath;
	am::AssetType									mAmData;
	std::auto_ptr<pkl2016::AssetType>	mpPklData;
	bool mFileNeedsNewHash;
	bool mIsOutsidePackage;
	QColor mColor = QColor(Qt::black); // Font color for IMP browser and timeline view
};


class AssetPkl : public Asset {

	Q_OBJECT

public:
	//! Import Packing List.
	AssetPkl(const QFileInfo &rFilePath, const am::AssetType &rAsset);
	//! Create New Packing List.
	AssetPkl(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText = QString());
	virtual ~AssetPkl() {}

private:
	Q_DISABLE_COPY(AssetPkl);

};


class AssetCpl : public Asset {

	Q_OBJECT

public:
	//! Import CPL.
	AssetCpl(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset);
	//! Create New CPL.
	AssetCpl(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText = QString());
	virtual ~AssetCpl() {}
	bool GetIsNewOrModified() {return mIsNewOrModified;}
	void SetIsNewOrModified(bool rIsNewOrModified) { mIsNewOrModified = rIsNewOrModified;}
	bool GetIsNew() {return mIsNew;}
	void SetIsNew(bool rIsNew) { mIsNew = rIsNew;}

private:
	Q_DISABLE_COPY(AssetCpl);
	bool mIsNewOrModified;
	bool mIsNew;
};


class AssetOpl : public Asset {

	Q_OBJECT

public:
	//! Import Opl.
	AssetOpl(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset);
	//! Create New Opl.
	AssetOpl(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText = QString());
	virtual ~AssetOpl() {}

private:
	Q_DISABLE_COPY(AssetOpl);
};


class AssetMxfTrack : public Asset {

	Q_OBJECT

public:
	//! Import Mxf Track. All imported Tracks are finalized.
	AssetMxfTrack(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset);
	//! Create New Mxf Track.
	AssetMxfTrack(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText = QString());
	//! Import existing MXF file to IMP.
	AssetMxfTrack(const QFileInfo &rFilePath, const Metadata &rMetadata, const UserText &rAnnotationText = QString());
	virtual ~AssetMxfTrack() {}
	//! Returns current metadata.
	Metadata GetMetadata() const { return mMetadata; }
	//! Returns the Essence type of this Mxf track.
	Metadata::eEssenceType GetEssenceType() const { return mMetadata.type; }
	//! Returns the source files.
	QStringList GetSourceFiles() const { return mSourceFiles; }
	bool HasSourceFiles() const { return !mSourceFiles.empty(); }
	SoundfieldGroup GetSoundfieldGroup() const { return mMetadata.soundfieldGroup; }
	EditRate GetEditRate() const { return mMetadata.editRate; }
	void SetEditRate(const EditRate rEditRate) { mMetadata.editRate = rEditRate; }
	//WR
	QString GetLanguageTag() const { return mMetadata.languageTag; }
	QString GetMCATitle() const { return mMetadata.mcaTitle; }
	QString GetMCATitleVersion() const { return mMetadata.mcaTitleVersion; }
	QString GetMCAAudioContentKind() const { return mMetadata.mcaAudioContentKind; }
	QString GetMCAAudioElementKind() const { return mMetadata.mcaAudioElementKind; }
	EditRate GetCplEditRate() const { return mCplEditRate; }
	Duration GetOriginalDuration() const { return mMetadata.originalDuration; }
	QImage extractFrame(Timecode tc); // (k)

	Duration GetDuration() const { return mMetadata.duration; }
	QString GetProfile() const { return mMetadata.profile; }
	EditRate GetTimedTextFrameRate() const {return mMetadata.effectiveFrameRate;};
	QImage GetProxyImage() const { return mFirstProxyImage; }
	//WR begin
	//Getter methods for the corresponding members
	cpl2016::EssenceDescriptorBaseType* GetEssenceDescriptor() { return mEssenceDescriptor;};
	QUuid GetSourceEncoding() const {return mSourceEncoding;};
	//WR end
	//! Set the Wav or Aces files that should be wrapped into Mxf. Does nothing if finalized. WARNING: overwrites old frame rate, soundfield group.
	void SetSourceFiles(const QStringList &rSourceFiles);
	//! Set the frame rate for Aces Asset. Does nothing if finalized.
	void SetFrameRate(const EditRate &rFrameRate);
	//! Set the soundfield group for Pcm Asset. Does nothing if finalized.
	void SetSoundfieldGroup(const SoundfieldGroup &rSoundfieldGroup);
	//! Set the duration for timed Text Asset. Does nothing if finalized.
	void SetDuration(const Duration &rDuration);
	//WR begin
	//! Set the Language Tag for Audio and Timed Text Assets.
	void SetLanguageTag(const QString &rLanguageTag) {mMetadata.languageTag = rLanguageTag;};
	// Set MCA metadata (mandatory per ST 2067-2)
	void SetMCATitle(const QString &rText) {mMetadata.mcaTitle = rText;};
	void SetMCATitleVersion(const QString &rText) {mMetadata.mcaTitleVersion = rText;};
	void SetMCAAudioContentKind(const QString &rText) {mMetadata.mcaAudioContentKind = rText;};
	void SetMCAAudioElementKind(const QString &rText) {mMetadata.mcaAudioElementKind = rText;};
	void SetCplEditRate(const EditRate &rCplEditRate) { mCplEditRate = rCplEditRate;};
	//This method extracts the essence descriptor from rFilePath and writes it into mEssenceDescriptor
	//Error ExtractEssenceDescriptor(const QString &filePath);
	Error ExtractEssenceDescriptor(const QString &filePath);
	bool GetIsNew() {return mIsNew;}
	void SetIsNew(bool rIsNew) { mIsNew = rIsNew;}

	public slots:
	//void SetEssenceDescriptor(const QString& qresult);
	void SetEssenceDescriptor(const DOMDocument* dom_result);
	//WR end

	private slots :
	void rTransformationFinished(const QImage &rImage, const QVariant &rIdentifier = QVariant());

private:
	Q_DISABLE_COPY(AssetMxfTrack);
	void SetDefaultProxyImages();

	JPEG2000 *mpJP2K; // (k) JP2K decoder
	Metadata		mMetadata;
	QStringList mSourceFiles;
	QImage			mFirstProxyImage;
	MetadataExtractor mMetadataExtr;
//WR begin
	//These are member variables for the corresponding CPL elements
	cpl2016::EssenceDescriptorBaseType* mEssenceDescriptor;
	QUuid mSourceEncoding;
	bool mIsNew;
	EditRate mCplEditRate;
//WR end
};


class AssetScm : public Asset {

	Q_OBJECT

public:
	//! Import SCM.
	AssetScm(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset, const scm::SidecarCompositionMapType &rSidecarCompositionMap);
	//! Create New SCM.
	AssetScm(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText = QString());
	virtual ~AssetScm() {}
	bool GetIsNew() {return mIsNew;}
	void SetIsNew(bool rIsNew) { mIsNew = rIsNew;}
	void SetAnnotationText(const UserText &rAnnotationText) { mData.getProperties().setAnnotation(ImfXmlHelper::Convert(rAnnotationText)); }
	void SetIssuer(const UserText &rIssuer) { mData.getProperties().setIssuer(ImfXmlHelper::Convert(rIssuer)); }
	UserText GetAnnotationText() const { if(mData.getProperties().getAnnotation().present() == true) return ImfXmlHelper::Convert(mData.getProperties().getAnnotation().get()); else return UserText(); }
	UserText GetIssuer() const { if(mData.getProperties().getIssuer().present() == true) return ImfXmlHelper::Convert(mData.getProperties().getIssuer().get()); else return UserText(); }
	QDateTime GetIssueDate() const { return ImfXmlHelper::Convert(mData.getProperties().getIssueDate()); }
	const scm::SidecarCompositionMapType& WriteScm();
	const scm::SidecarCompositionMapType& GetScm() { return mData; }

	typedef struct  {
		QFileInfo filepath;
		QUuid id;
		QVector< QSharedPointer<AssetCpl> > mAssociatedCplAssets;
		QList<QUuid> mCplIdsNotInCurrentImp;
	} SidecarCompositionMapEntry;
	bool AddSidecarCompositionMapEntry(const SidecarCompositionMapEntry &rSidecarCompositionMapEntry);
	QList<SidecarCompositionMapEntry> GetSidecarCompositionMapEntries() const { return mSidecarCompositionMapEntries; }


private:
	Q_DISABLE_COPY(AssetScm);
	bool mIsNew;
	scm::SidecarCompositionMapType mData;
	QList<SidecarCompositionMapEntry> mSidecarCompositionMapEntries;
};


class AssetSidecar : public Asset {

	Q_OBJECT

public:
	//! Import Sidecar Asset.
	AssetSidecar(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset);
	//! Create New Sidecar Asset.
	AssetSidecar(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText = QString());
	virtual ~AssetSidecar() {}
	bool GetIsNew() {return mIsNew;}
	void SetIsNew(bool rIsNew) { mIsNew = rIsNew;}

private:
	Q_DISABLE_COPY(AssetSidecar);
	QUuid mUuid;
	bool mIsNew;


};


// (k) - start
struct VideoResource {
	QSharedPointer<AssetMxfTrack> asset; // pointer to asset
	qint64 in; // in-point
	qint64 out; // out-point
	qint64 Duration;
	int RepeatCount;
};
// (k) - end

