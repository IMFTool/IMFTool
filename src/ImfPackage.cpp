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
#include "ImfPackage.h"
#include "global.h"
#include "SMPTE-2067-3-2013-CPL.h"
#include "SMPTE-2067-100a-2014-OPL.h"
#include "ImfMimeData.h"
#include <QFile>
#include <fstream>
#include <QThreadPool>
//WR begin
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include<iostream>
#include<fstream>
#include<string>
#include<cstdlib>
#include<sstream>

//WR end


ImfPackage::ImfPackage(const QDir &rWorkingDir) :
QAbstractTableModel(NULL), mpAssetMap(NULL), mPackingLists(), mAssetList(), mRootDir(rWorkingDir), mIsDirty(false), mIsIngest(false) {

	mpAssetMap = new AssetMap(this, mRootDir.absoluteFilePath(ASSET_SEARCH_NAME));
	QUuid pkl_id = QUuid::createUuid();
	QString pkl_file_path(mRootDir.absoluteFilePath(QString("PKL_%1.xml").arg(strip_uuid(pkl_id))));
	QSharedPointer<AssetPkl> pkl_asset(new AssetPkl(pkl_file_path, pkl_id));
	mPackingLists.push_back(new PackingList(this, pkl_file_path, pkl_id));
	AddAsset(pkl_asset, QUuid());
}

ImfPackage::ImfPackage(const QDir &rWorkingDir, const UserText &rIssuer, const UserText &rAnnotationText /*= QString()*/) :
QAbstractTableModel(NULL), mpAssetMap(NULL), mPackingLists(), mAssetList(), mRootDir(rWorkingDir), mIsDirty(true), mIsIngest(false) {

	mpAssetMap = new AssetMap(this, mRootDir.absoluteFilePath(ASSET_SEARCH_NAME), rAnnotationText, rIssuer);
	QUuid pkl_id = QUuid::createUuid();
	QString pkl_file_path(mRootDir.absoluteFilePath(QString("PKL_%1.xml").arg(strip_uuid(pkl_id))));
	QSharedPointer<AssetPkl> pkl_asset(new AssetPkl(pkl_file_path, pkl_id));
	mPackingLists.push_back(new PackingList(this, pkl_file_path, pkl_id, QUuid(), QUuid(), rAnnotationText, rIssuer));
	AddAsset(pkl_asset, QUuid());
}

ImfError ImfPackage::Ingest() {

	mIsIngest = true;
	// see SMPTE ST 429-9:2014 Annex A Basic Map Profile v2
	ImfError error; // Reset last error.
	qDebug() << "Ingest dir: " << mRootDir.dirName();
	if(mRootDir.exists() == false) error = ImfError(ImfError::WorkingDirNotFound);
	else {
		if(mRootDir.exists(ASSET_SEARCH_NAME)) {
			// ASSETMAP.xml found in root dir
			if(mpAssetMap != NULL) {
				mpAssetMap->deleteLater(); // delete old Asset Map
				mpAssetMap = NULL;
			}
			for(int i = 0; i < mPackingLists.size(); i++) {
				if(mPackingLists.at(i) != NULL) mPackingLists.at(i)->deleteLater(); // delete old Packing Lists}
			}
			mPackingLists.clear();
			beginResetModel();
			mAssetList.clear(); // dismiss all Assets
			endResetModel();
			error = ParseAssetMap(mRootDir.absoluteFilePath(ASSET_SEARCH_NAME));
		}
		else {
			// search sub dirs for ASSETMAP.xml
			QStringList dirs = mRootDir.entryList(QDir::AllDirs);
			for(int i = 0; i < dirs.count(); i++) {
				if(mRootDir.exists(QString(dirs.at(i)).append(ASSET_SEARCH_NAME))) {
					error = ImfError(ImfError::MultipleAssetMapsFound);
					break;
				}
			}
			if(error.IsError() == false) {
				error = ImfError(ImfError::NoAssetMapFound);
			}
		}
	}
	if(!error) {
		bool old_dirty = mIsDirty;
		mIsDirty = false;
		if(old_dirty != false) emit DirtyChanged(false);
	}
	qDebug() << "Finished Ingest dir: " << mRootDir.dirName();
	mIsIngest = false;
	return error;
}

ImfError ImfPackage::Outgest() {

	ImfError error; // Reset last error.
	XmlSerializationError serialization_error;
	// namespace maps
	xml_schema::NamespaceInfomap am_namespace;
	am_namespace[""].name = XML_NAMESPACE_AM;
	am_namespace["xs"].name = XML_NAMESPACE_XS;

	xml_schema::NamespaceInfomap pkl_namespace;
	pkl_namespace[""].name = XML_NAMESPACE_PKL;
	pkl_namespace["ds"].name = XML_NAMESPACE_DS;
	pkl_namespace["xs"].name = XML_NAMESPACE_XS;

	QUuid pkl_id = QUuid::createUuid();
	QString pkl_file_path(mRootDir.absoluteFilePath(QString("PKL_%1.xml").arg(strip_uuid(pkl_id))));

	for(int i = 0; i < mPackingLists.size(); i++) {
		if(mPackingLists.at(i)) {

			pkl::PackingListType packing_list(mPackingLists.at(i)->Write());
			packing_list.setAssetList(pkl::PackingListType_AssetListType());
			packing_list.getAssetList().setAsset(pkl::PackingListType_AssetListType::AssetSequence());
			for(int ii = 0; ii < mAssetList.size(); ii++) {
				// Pkl Asset must not be written in Packing List
				if(mAssetList.at(ii)->GetType() != Asset::pkl) {
					if(mAssetList.at(ii)->Exists()) {
						if(mAssetList.at(ii)->GetPklId() == mPackingLists.at(i)->GetId()) {
							packing_list.getAssetList().getAsset().push_back(*mAssetList.at(ii)->WritePkl().get());
						}
					}
					else qWarning() << "Asset doesn't exist on file system. Asset will not be written into Asset.";
				}
			}
			// Write Packing List
			packing_list.setId(ImfXmlHelper::Convert(pkl_id));

			std::ofstream pkl_ofs(mPackingLists.at(i)->GetFilePath().absoluteFilePath().toStdString().c_str(), std::ofstream::out);
			try {
				pkl::serializePackingList(pkl_ofs, packing_list, pkl_namespace, "UTF-8", xml_schema::Flags::dont_initialize);
			}
			catch(xml_schema::Serialization &e) { serialization_error = XmlSerializationError(e); }
			catch(xml_schema::UnexpectedElement &e) { serialization_error = XmlSerializationError(e); }
			catch(xml_schema::NoTypeInfo &e) { serialization_error = XmlSerializationError(e); }
			catch(...) { serialization_error = XmlSerializationError(XmlSerializationError::Unknown); }
			pkl_ofs.close();
			if(serialization_error.IsError() == false) {
				if(QSharedPointer<Asset> pkl_asset = GetAsset(mPackingLists.at(i)->GetId())) {
					pkl_asset->FileModified();
				}
			}
			else break;
		}
	}

	QString old_pkl_file_path;
	for(int i = 0; i < mPackingLists.size(); i++){
		old_pkl_file_path = mPackingLists.at(i)->GetFilePath().absoluteFilePath();
		RemoveAsset(mPackingLists.at(i)->GetId());
	}
	QFile::rename(old_pkl_file_path, pkl_file_path);
	QSharedPointer<AssetPkl> pkl_asset(new AssetPkl(pkl_file_path, pkl_id));
	mPackingLists.clear();
	mPackingLists.push_back(new PackingList(this, pkl_file_path, pkl_id));
	AddAsset(pkl_asset, QUuid());

	if(serialization_error.IsError() == false) {
		// Write VOLINDEX.xml
		am::VolumeIndexType volume_index(xml_schema::PositiveInteger(1));
		std::ofstream volindex_ofs(mRootDir.absoluteFilePath(VOLINDEX_SEARCH_NAME).toStdString().c_str(), std::ofstream::out);
		try {
			am::serializeVolumeIndex(volindex_ofs, volume_index, am_namespace, "UTF-8", xml_schema::Flags::dont_initialize);
		}
		catch(xml_schema::Serialization &e) { serialization_error = XmlSerializationError(e); }
		catch(xml_schema::UnexpectedElement &e) { serialization_error = XmlSerializationError(e); }
		catch(xml_schema::NoTypeInfo &e) { serialization_error = XmlSerializationError(e); }
		catch(...) { serialization_error = XmlSerializationError(XmlSerializationError::Unknown); }
		volindex_ofs.close();
	}

	if(serialization_error.IsError() == false) {
		if(mpAssetMap) {
			am::AssetMapType asset_map(mpAssetMap->Write());
			asset_map.setAssetList(am::AssetMapType_AssetListType());
			asset_map.getAssetList().setAsset(am::AssetMapType_AssetListType::AssetSequence());
			for(int i = 0; i < mAssetList.size(); i++) {
				if(mAssetList.at(i)->Exists()) asset_map.getAssetList().getAsset().push_back(mAssetList.at(i)->WriteAm());
				else qWarning() << "Asset doesn't exist on file system. Asset will not be written into Packing List.";
			}

			// Write ASSETMAP.xml
			mpAssetMap->SetId();	//Generates new UUID for AM everytime the AM is written
			std::ofstream am_ofs(mpAssetMap->GetFilePath().absoluteFilePath().toStdString().c_str(), std::ofstream::out);
			try {
				am::serializeAssetMap(am_ofs, asset_map, am_namespace, "UTF-8", xml_schema::Flags::dont_initialize);
			}
			catch(xml_schema::Serialization &e) { serialization_error = XmlSerializationError(e); }
			catch(xml_schema::UnexpectedElement &e) { serialization_error = XmlSerializationError(e); }
			catch(xml_schema::NoTypeInfo &e) { serialization_error = XmlSerializationError(e); }
			catch(...) { serialization_error = XmlSerializationError(XmlSerializationError::Unknown); }
			am_ofs.close();
		}
	}

	if(serialization_error.IsError() == true) error = ImfError(serialization_error);
	if(error.IsError() == false) {
		bool old_dirty = mIsDirty;
		mIsDirty = false;
		if(old_dirty != false) emit DirtyChanged(false);
	}
	else qWarning() << error;
	return error;
}

ImfError ImfPackage::ParseAssetMap(const QFileInfo &rAssetMapFilePath) {

	ImfError error;
	XmlParsingError parse_error;
	// ---Parse Asset Map---
	std::auto_ptr<am::AssetMapType> asset_map;
	try {
		asset_map = am::parseAssetMap(rAssetMapFilePath.absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize);
	}
	catch(const xml_schema::Parsing &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::ExpectedElement &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::UnexpectedElement &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::ExpectedAttribute &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::UnexpectedEnumerator &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::ExpectedTextContent &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::NoTypeInfo &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::NotDerived &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::NoPrefixMapping &e) { parse_error = XmlParsingError(e); }
	catch(...) { parse_error = XmlParsingError(XmlParsingError::Unknown); }

	if(parse_error.IsError() == false) {
		// XML parsing succeeds.
		if(asset_map->getVolumeCount() == 1) {
			mpAssetMap = new AssetMap(this, rAssetMapFilePath, *asset_map.get()); // add new Asset Map
			// We must find all Packing Lists.
			for(unsigned int i = 0; i < asset_map->getAssetList().getAsset().size(); i++) {
				am::AssetType asset = asset_map->getAssetList().getAsset().at(i);
				if(asset.getPackingList().present() && asset.getPackingList().get() == xml_schema::Boolean(true)) {
					// We found a Packing List.
					if(asset.getChunkList().getChunk().size() == 1) {
						QFileInfo packing_list_path = QFileInfo(mRootDir.absolutePath().append("/").append(asset.getChunkList().getChunk().back().getPath().c_str()));
						qDebug() << QDir::toNativeSeparators(packing_list_path.absoluteFilePath());
						if(packing_list_path.exists() == true) {
							// ---Parse Packing List---
							std::auto_ptr<pkl::PackingListType> packing_list;
							try {
								packing_list = (pkl::parsePackingList(QDir::toNativeSeparators(packing_list_path.absoluteFilePath()).toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize));
							}
							catch(const xml_schema::Parsing &e) { parse_error = XmlParsingError(e); }
							catch(const xml_schema::ExpectedElement &e) { parse_error = XmlParsingError(e); }
							catch(const xml_schema::UnexpectedElement &e) { parse_error = XmlParsingError(e); }
							catch(const xml_schema::ExpectedAttribute &e) { parse_error = XmlParsingError(e); }
							catch(const xml_schema::UnexpectedEnumerator &e) { parse_error = XmlParsingError(e); }
							catch(const xml_schema::ExpectedTextContent &e) { parse_error = XmlParsingError(e); }
							catch(const xml_schema::NoTypeInfo &e) { parse_error = XmlParsingError(e); }
							catch(const xml_schema::NotDerived &e) { parse_error = XmlParsingError(e); }
							catch(const xml_schema::NoPrefixMapping &e) { parse_error = XmlParsingError(e); }
							catch(...) { parse_error = XmlParsingError(XmlParsingError::Unknown); }

							if(parse_error.IsError() == false) {
								// XML parsing succeeds
								PackingList *p_new_packing_list = new PackingList(this, packing_list_path, *packing_list.get()); // add new Packing list
								mPackingLists.push_back(p_new_packing_list);
								// First add Packing List Asset itself.		
								AddAsset(QSharedPointer<AssetPkl>(new AssetPkl(packing_list_path, asset)), QUuid()); // PKL Id doesn't matter. It's a new Packing List which cannot be added to an existing PKL.
								// Add all Assets found in Packing List
								for(unsigned int i = 0; i < packing_list->getAssetList().getAsset().size(); i++) {
									pkl::AssetType pkl_asset = packing_list->getAssetList().getAsset().at(i);
									// Find equivalent Asset in Asset Map
									for(unsigned int ii = 0; ii < asset_map->getAssetList().getAsset().size(); ii++) {
										am::AssetType am_asset = asset_map->getAssetList().getAsset().at(ii);
										if(pkl_asset.getId() == am_asset.getId()) {
											if(am_asset.getChunkList().getChunk().size() == 1) {
												QFileInfo new_asset_path = QFileInfo(mRootDir.absolutePath().append("/").append(am_asset.getChunkList().getChunk().back().getPath().c_str()));
												if(pkl_asset.getType().compare(MIME_TYPE_MXF) == 0) {
													// Add Asset MXF Track
													QSharedPointer<AssetMxfTrack> mxf_track(new AssetMxfTrack(new_asset_path, am_asset, pkl_asset));
													AddAsset(mxf_track, ImfXmlHelper::Convert(packing_list->getId()));
												}
												else if(pkl_asset.getType().compare(MIME_TYPE_XML) == 0) {
													// Add CPL or OPL
													// We have to parse the file to determine the type (opl or cpl)
													bool is_opl = true;
													bool is_cpl = true;

													try { cpl::parseCompositionPlaylist(new_asset_path.absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
													catch(...) { is_cpl = false; }
													try { opl::parseOutputProfileList(new_asset_path.absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
													catch(...) { is_opl = false; }
													if(is_cpl && !is_opl) {
														// Add CPL
														QSharedPointer<AssetCpl> cpl(new AssetCpl(new_asset_path, am_asset, pkl_asset));
														AddAsset(cpl, ImfXmlHelper::Convert(packing_list->getId()));
													}
													else if(is_opl && !is_cpl) {
														// Add Opl
														QSharedPointer<AssetOpl> opl(new AssetOpl(new_asset_path, am_asset, pkl_asset));
														AddAsset(opl, ImfXmlHelper::Convert(packing_list->getId()));
													}
													else {
														qDebug() << "Unknown " MIME_TYPE_XML " Asset found: " << ImfXmlHelper::Convert(am_asset.getId());
														error = ImfError(ImfError::UnknownAsset, am_asset.getId().c_str(), true);
														QSharedPointer<Asset> unknown(new Asset(Asset::unknown, new_asset_path, am_asset, std::auto_ptr<pkl::AssetType>(new pkl::AssetType(pkl_asset))));
														AddAsset(unknown, ImfXmlHelper::Convert(packing_list->getId()));
													}
												}
												else {
													qWarning() << "Unsupported Asset type element " << pkl_asset.getType().c_str() << " found: " << ImfXmlHelper::Convert(am_asset.getId());
													error = ImfError(ImfError::UnknownAsset, am_asset.getId().c_str(), true);
													QSharedPointer<Asset> unknown(new Asset(Asset::unknown, new_asset_path, am_asset, std::auto_ptr<pkl::AssetType>(new pkl::AssetType(pkl_asset))));
													AddAsset(unknown, ImfXmlHelper::Convert(packing_list->getId()));
												}
											}
											else {
												error = ImfError(ImfError::MultipleChunks, "", true);
												continue;
											}
										}
									}
								}
							}
							else {
								qDebug() << parse_error;
								error = ImfError(parse_error);
								break;
							}
						}
						else {
							error = ImfError(ImfError::NoPackingListFound, tr("Asset Map refers to: %1").arg(packing_list_path.absoluteFilePath()));
						}
					}
					else {
						error = ImfError(ImfError::MultipleChunks);
					}
				}
			}
		}
		else {
			error = ImfError(ImfError::AssetMapSplit);
		}
	}
	else {
		qDebug() << parse_error;
		error = ImfError(parse_error);
	}
	return error;
}

PackingList* ImfPackage::GetPackingList(const QUuid &rUuid) {

	for(int i = 0; i < mPackingLists.size(); i++) {
		if(mPackingLists.at(i)->GetId() == rUuid) return mPackingLists.at(i);
	}
	return NULL;
}

QUuid ImfPackage::GetPackingListId(PackingList *pPackingList) {

	for(int i = 0; i < mPackingLists.size(); i++) {
		if(mPackingLists.at(i) == pPackingList) return mPackingLists.at(i)->GetId();
	}
	return QUuid();
}

QUuid ImfPackage::GetPackingListId(int index /*= 0*/) {

	if(index < mPackingLists.size()) {
		return mPackingLists.at(index)->GetId();
	}
	return QUuid();
}

bool ImfPackage::AddAsset(const QSharedPointer<Asset> &rAsset, const QUuid &rPackingListId) {

	bool success = true;
	if(GetAsset(rAsset->GetId()).isNull()) {
		if(mpAssetMap) {
			if(rAsset->GetType() != Asset::pkl) {
				PackingList *p_packing_list = GetPackingList(rPackingListId);
				if(p_packing_list) {
					connect(p_packing_list, SIGNAL(destroyed(QObject *)), rAsset.data(), SLOT(AffinityLost(QObject *)));
					connect(mpAssetMap, SIGNAL(destroyed(QObject *)), rAsset.data(), SLOT(AffinityLost(QObject *)));
					connect(rAsset.data(), SIGNAL(AssetModified(Asset *)), this, SLOT(rAssetModified(Asset *)));
					beginInsertRows(QModelIndex(), mAssetList.size(), mAssetList.size());
					mAssetList.push_back(rAsset);
					endInsertRows();
					rAsset->AffinityWon(p_packing_list);
					rAsset->AffinityWon(mpAssetMap);
				}
				else {
					success = false;
					qWarning() << "Couldn't add Asset: " << rAsset->GetId() << ": No Packing List available.";
				}
			}
			else {
				connect(mpAssetMap, SIGNAL(destroyed(QObject *)), rAsset.data(), SLOT(AffinityLost(QObject *)));
				connect(rAsset.data(), SIGNAL(AssetModified(Asset *)), this, SLOT(rAssetModified(Asset *)));
				beginInsertRows(QModelIndex(), mAssetList.size(), mAssetList.size());
				mAssetList.push_back(rAsset);
				endInsertRows();
				rAsset->AffinityWon(mpAssetMap);
			}
		}
		else {
			success = false;
			qWarning() << "Couldn't add Asset: " << rAsset->GetId() << ": No Asset Map available.";
		}
	}
	else {
		success = false;
		qWarning() << "Asset " << rAsset->GetId() << " already exists.";
	}
	return success;
}

QSharedPointer<Asset> ImfPackage::GetAsset(const QUuid &rUuid) {

	for(int i = 0; i < mAssetList.size(); i++) {
		if(rUuid == mAssetList.at(i)->GetId()) return mAssetList.at(i);
	}
	return QSharedPointer<Asset>();
}

QSharedPointer<Asset> ImfPackage::GetAsset(int index) {

	if(index >= 0 && index < mAssetList.size()) return mAssetList.at(index);
	return QSharedPointer<Asset>();
}

void ImfPackage::RemoveAsset(const QUuid &rUuid) {

	mpAssetMap->SetId();
	for(int i = 0; i < mAssetList.size(); i++) {
		if(rUuid == mAssetList.at(i)->GetId()) {
			mAssetList.at(i)->AffinityLost(mpAssetMap);
			mAssetList.at(i)->AffinityLost(GetPackingList(mAssetList.at(i)->GetPklId()));
			disconnect(mAssetList.at(i).data(), NULL, this, NULL);
			beginRemoveRows(QModelIndex(), i, i);
			mAssetList.removeAt(i);
			endRemoveRows();
		}
	}
}

void ImfPackage::RemoveAsset(int index) {

	if(index < mAssetList.size()) {
		RemoveAsset(mAssetList.at(index)->GetId());
	}
}

int ImfPackage::rowCount(const QModelIndex &rParent /*= QModelIndex()*/) const {

	if(!rParent.isValid()) {
		return mAssetList.size();
	}
	return 0;
}

int ImfPackage::columnCount(const QModelIndex &rParent /*= QModelIndex()*/) const {

	if(!rParent.isValid()) {
		return ImfPackage::ColumnMax;
	}
	return 0;
}

QVariant ImfPackage::data(const QModelIndex &rIndex, int role /*= Qt::DisplayRole*/) const {

	const int row = rIndex.row();
	const int column = rIndex.column();

	if(row < mAssetList.size()) {
		if(column == ImfPackage::ColumnIcon) {
			// icon
			if(role == Qt::DecorationRole) {
				switch(mAssetList.at(row)->GetType()) {
					case Asset::mxf:
						return QVariant(QPixmap(":/asset_mxf.png"));
						break;
					case Asset::cpl:
						return QVariant(QPixmap(":/asset_cpl.png"));
						break;
					case Asset::opl:
						return QVariant(QPixmap(":/asset_opl.png"));
						break;
					default:
						return QVariant(QPixmap(":/asset_unknown.png"));
						break;
				}
			}
			else if(role == Qt::SizeHintRole) {
				return QVariant(QSize(32, 34));
			}
		}
		else if(column == ImfPackage::ColumnAssetType) {
			if(role == Qt::DisplayRole) {
				switch(mAssetList.at(row)->GetType()) {
					case Asset::mxf:
						return QVariant("mxf");
						break;
					case Asset::cpl:
						return QVariant("cpl");
						break;
					case Asset::opl:
						return QVariant("opl");
						break;
					case Asset::pkl:
						return QVariant("pkl");
						break;
					default:
						return QVariant("unknown");
						break;
				}
			}
		}
		else if(column == ImfPackage::ColumnFilePath) {
			if(role == Qt::DisplayRole) {
				return QVariant(mRootDir.relativeFilePath(mAssetList.at(row)->GetPath().absoluteFilePath()));
			}
			else if(role == Qt::ToolTipRole) {
				return QVariant(mRootDir.relativeFilePath(mAssetList.at(row)->GetPath().absoluteFilePath()));
			}
		}
		else if(column == ImfPackage::ColumnFileSize) {
			if(role == Qt::DisplayRole) {
				if(mAssetList.at(row)->Exists() == true) {
					quint32 size = mAssetList.at(row)->GetSize();
					if(size < 1048576) return QVariant(QString::number((double)size / 1024., 'f', 2).append(" KiB"));
					else if(size < 1073741824) return QVariant(QString::number((double)size / 1048576., 'f', 2).append(" MiB"));
					else return QVariant(QString::number((double)size / 1073741824., 'f', 2).append(" GiB"));
				}
				else return QVariant(tr("Not Finalized"));
			}
			else if(role == Qt::TextAlignmentRole) {
				return QVariant(Qt::AlignRight | Qt::AlignCenter);
			}
		}
		else if(column == ImfPackage::ColumnFinalized) {
			if(role == Qt::CheckStateRole) {
				if(mAssetList.at(row)->Exists() == true) return Qt::Checked;
				else return Qt::Unchecked;
			}
		}
		else if(column == ImfPackage::ColumnAnnotation) {
			if(role == Qt::DisplayRole) {
				return QVariant(mAssetList.at(row)->GetAnnotationText().first);
			}
			else if(role == Qt::EditRole) {
				return QVariant(mAssetList.at(row)->GetAnnotationText().first);
			}
		}
		else if(column == ImfPackage::ColumnProxyImage) {
			if(role == Qt::DecorationRole) {
				if(mAssetList.at(row)->GetType() == Asset::mxf) {
					QSharedPointer<AssetMxfTrack> p_asset = mAssetList.at(row).objectCast<AssetMxfTrack>();
					if(p_asset) {
						return QVariant(QPixmap::fromImage(p_asset->GetProxyImage()));
					}
				}
			}
		}
		else if(column == ImfPackage::ColumnMetadata) {
			if(role == UserRoleMetadata) {
				if(mAssetList.at(row)->GetType() == Asset::mxf) {
					QSharedPointer<AssetMxfTrack> p_asset = mAssetList.at(row).objectCast<AssetMxfTrack>();
					if(p_asset) {
						return QVariant::fromValue(p_asset->GetMetadata());
					}
				}
			}
		}
	}
	return QVariant();
}

QVariant ImfPackage::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const {

	switch(orientation) {
		case Qt::Horizontal:
			if(role == Qt::DisplayRole) {
				if(section == ImfPackage::ColumnIcon) {
					return QVariant(tr("Icon"));
				}
				else if(section == ImfPackage::ColumnAssetType) {
					return QVariant(tr("Type"));
				}
				else if(section == ImfPackage::ColumnFilePath) {
					return QVariant(tr("rel. File Path"));
				}
				else if(section == ImfPackage::ColumnFileSize) {
					return QVariant(tr("File Size"));
				}
				else if(section == ImfPackage::ColumnFinalized) {
					return QVariant(tr("Finalized"));
				}
				else if(section == ImfPackage::ColumnAnnotation) {
					return QVariant(tr("Annotation"));
				}
				else if(section == ImfPackage::ColumnProxyImage) {
					return QVariant(tr("Proxy Image"));
				}
				else if(section == ImfPackage::ColumnMetadata) {
					return QVariant(tr("Metadata"));
				}
			}
			else if(role == Qt::SizeHintRole && section == ImfPackage::ColumnIcon) {
				// 				return QVariant(QSize(38, -1));
			}
			break;
		case Qt::Vertical:
			if(role == Qt::DisplayRole) {
				return QVariant(tr("Asset"));
			}
			break;
		default:
			return QVariant();
			break;
	}
	return QVariant();
}

Qt::ItemFlags ImfPackage::flags(const QModelIndex &rIndex) const {

	const int row = rIndex.row();
	const int column = rIndex.column();
	Qt::ItemFlags ret = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if(column == ImfPackage::ColumnAnnotation) ret |= Qt::ItemIsEditable;
	else ret |= Qt::ItemIsDragEnabled;
	return ret;
}

bool ImfPackage::setData(const QModelIndex &rIndex, const QVariant &rValue, int role /*= Qt::EditRole*/) {

	const int row = rIndex.row();
	const int column = rIndex.column();

	if(row < mAssetList.size()) {
		if(role == Qt::EditRole) {
			if(column == ImfPackage::ColumnAnnotation) {
				mAssetList.at(row)->SetAnnotationText(rValue.toString());
				emit dataChanged(rIndex, rIndex);
				return true;
			}
		}
	}
	return false;
}

void ImfPackage::rAssetModified(Asset *pAsset) {

	if(pAsset) {
		for(int i = 0; i < mAssetList.size(); i++) {
			if(*pAsset == *mAssetList.at(i)) {
				if(mIsIngest == false) {
					bool old_dirty = mIsDirty;
					mIsDirty = true;
					if(old_dirty != true) emit DirtyChanged(true);
				}
				emit dataChanged(index(i, ImfPackage::ColumnIcon), index(i, ImfPackage::ColumnMax - 1));
			}
		}
	}
}

QMimeData* ImfPackage::mimeData(const QModelIndexList &indexes) const {

	ImfMimeData *mimeData = new ImfMimeData();
	QList<QUrl> urls;

	for(int i = 0; i < indexes.size(); i++) {
		const QModelIndex &index = indexes.at(i);
		if(index.isValid()) {
			QSharedPointer<Asset> asset = mAssetList.at(index.row());
			if(asset) {
				if(asset->GetType() != Asset::unknown) {
					urls << QUrl::fromLocalFile(asset->GetPath().absoluteFilePath());
					if(asset->GetType() == Asset::mxf) {
						mimeData->SetAsset(asset);
					}
				}
			}
		}
	}
	mimeData->setUrls(urls);
	return mimeData;
}

Qt::DropActions ImfPackage::supportedDropActions() const {

	return Qt::CopyAction;
}

AssetMap::AssetMap(ImfPackage *pParent, const QFileInfo &rFilePath, const am::AssetMapType &rAssetMap) :
QObject(pParent), mFilePath(rFilePath), mData(rAssetMap) {

	// Delete Asset List. The Asset List is generated in ImfPackage::Outgest()
	mData.setAssetList(am::AssetMapType_AssetListType());
}

AssetMap::AssetMap(ImfPackage *pParent, const QFileInfo &rFilePath, const UserText &rAnnotationText /*= QString()*/, const UserText &rIssuer /*= QString()*/) :
QObject(pParent), mFilePath(rFilePath),
mData(ImfXmlHelper::Convert(QUuid::createUuid()),
ImfXmlHelper::Convert(UserText(CREATOR_STRING)),
xml_schema::PositiveInteger(1),
ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()),
ImfXmlHelper::Convert(UserText(rIssuer)),
am::AssetMapType_AssetListType()
) {

	if(rAnnotationText.IsEmpty() == false) SetAnnotationText(rAnnotationText);
}

const am::AssetMapType& AssetMap::Write() {

	// Update values before serialization.
	mData.setCreator(ImfXmlHelper::Convert(UserText(CREATOR_STRING)));
	mData.setIssueDate(ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()));
	mData.setVolumeCount(xml_schema::PositiveInteger(1));
	return mData;
}


PackingList::PackingList(ImfPackage *pParent, const QFileInfo &rFilePath, const pkl::PackingListType &rPackingList) :
QObject(pParent), mFilePath(rFilePath), mData(rPackingList) {

	mData.setAssetList(pkl::PackingListType_AssetListType());
	if(mData.getSigner().present() == true) qWarning() << "Signer is in Packing List not supported.";
	if(mData.getSignature().present() == true) qWarning() << "Signature is in Packing List not supported.";
}

PackingList::PackingList(ImfPackage *pParent, const QFileInfo &rFilePath, const QUuid &rId, const QUuid &rIconId /*= QUuid()*/, const QUuid &rGroupId /*= QUuid()*/, const UserText &rAnnotationText /*= QString()*/, const UserText &rIssuer /*= QString()*/) :
QObject(pParent), mFilePath(rFilePath),
mData(ImfXmlHelper::Convert(rId),
ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()),
ImfXmlHelper::Convert(UserText(rIssuer)),
ImfXmlHelper::Convert(UserText(CREATOR_STRING)),
pkl::PackingListType_AssetListType()
) {

	if(rAnnotationText.IsEmpty() == false) SetAnnotationText(rAnnotationText);
	if(rIconId.isNull() == false) mData.setIconId(ImfXmlHelper::Convert(rIconId));
	if(rGroupId.isNull() == false) mData.setGroupId(ImfXmlHelper::Convert(rGroupId));
}

const pkl::PackingListType& PackingList::Write() {

	// Update values before serialization.
	mData.setCreator(ImfXmlHelper::Convert(UserText(CREATOR_STRING)));
	mData.setIssueDate(ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()));
	// Remove Signature and Signer. We don't support this.
	mData.setSignature(std::auto_ptr<ds::SignatureType>(NULL));
	mData.setSigner(std::auto_ptr<ds::KeyInfoType>(NULL));
	return mData;
}


// Creates new Asset. rFilePath must be the prospective path of the asset. If the asset doesn't exist on the file system when ImfPackage::Outgest() is invoked the asset will be ignored.
Asset::Asset(eAssetType type, const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
QObject(NULL), mpAssetMap(NULL), mpPackageList(NULL), mType(type), mFilePath(rFilePath),
mAmData(ImfXmlHelper::Convert(QUuid() /*empty*/), am::AssetType_ChunkListType() /*empty*/),
mpPklData(NULL), mFileNeedsNewHash(true) {

	if(mType != pkl) {
		mpPklData = std::auto_ptr<pkl::AssetType>(new pkl::AssetType(
			ImfXmlHelper::Convert(QUuid() /*empty*/),
			ImfXmlHelper::Convert(QByteArray() /*empty*/),
			xml_schema::PositiveInteger(0) /*empty*/,
			mType == mxf ? xml_schema::String(MIME_TYPE_MXF) : xml_schema::String(MIME_TYPE_XML)));
	}

	QUuid id;
	if(rId.isNull()) {
		id = QUuid::createUuid();
		qDebug() << "The asset ctor was invoked with an empty UUID. A new valid UUID will be generated.";
	}
	else id = rId;
	mAmData.setId(ImfXmlHelper::Convert(id));
	if(mpPklData.get()) mpPklData->setId(ImfXmlHelper::Convert(id));

	if(mType == pkl) mAmData.setPackingList(xml_schema::Boolean(true));
	if(rAnnotationText.IsEmpty() == false) {
		SetAnnotationText(rAnnotationText);
	}
	if(Exists()) {
		if(mpPklData.get()) {
			mpPklData->setSize(xml_schema::PositiveInteger(mFilePath.size()));
			mpPklData->setOriginalFileName(ImfXmlHelper::Convert(UserText(mFilePath.fileName())));
		}
	}
	connect(this, SIGNAL(AssetModified(Asset*)), this, SLOT(rAssetModified(Asset*)));
}

// Import existing Asset
Asset::Asset(eAssetType type, const QFileInfo &rFilePath, const am::AssetType &rAsset, std::auto_ptr<pkl::AssetType> assetType /*= std::auto_ptr<pkl::AssetType>(NULL)*/) :
QObject(NULL), mpAssetMap(NULL), mpPackageList(NULL), mType(type), mFilePath(rFilePath), mAmData(rAsset), mpPklData(assetType), mFileNeedsNewHash(false) {

	connect(this, SIGNAL(AssetModified(Asset*)), this, SLOT(rAssetModified(Asset*)));
}

const am::AssetType& Asset::WriteAm() {

	// Update values before serialization.
	if(mType == pkl) mAmData.setPackingList(xml_schema::Boolean(true));
	if(Exists() == true) {
		if(mpAssetMap) {
			QDir root_dir = mpAssetMap->GetFilePath().absoluteDir();
			mAmData.setChunkList(am::AssetType_ChunkListType()); // Remove old Chunks.
			mAmData.getChunkList().setChunk(am::AssetType_ChunkListType::ChunkSequence());
			mAmData.getChunkList().getChunk().push_back(am::ChunkType(xml_schema::Uri(root_dir.relativeFilePath(mFilePath.absoluteFilePath()).toStdString())));
		}
		else {
			mAmData.setChunkList(am::AssetType_ChunkListType()); // Remove old Chunks.
		}
	}
	return mAmData;
}

const std::auto_ptr<pkl::AssetType>& Asset::WritePkl() {

	return mpPklData;
}

UserText Asset::GetAnnotationText() const {
	if(mAmData.getAnnotationText().present() == true) return ImfXmlHelper::Convert(mAmData.getAnnotationText().get());
	else if(mpPklData.get() && mpPklData->getAnnotationText().present() == true) return ImfXmlHelper::Convert(mpPklData->getAnnotationText().get());
	return UserText();
}

void Asset::SetAnnotationText(const UserText &rAnnotationText) {

	if(rAnnotationText != GetAnnotationText()) {
		mAmData.setAnnotationText(ImfXmlHelper::Convert(rAnnotationText));
		if(mpPklData.get()) {
			mpPklData->setAnnotationText(ImfXmlHelper::Convert(rAnnotationText));
		}
		emit AssetModified(this);
	}
}

void Asset::SetHash(const QByteArray &rHash) {

	mFileNeedsNewHash = false;
	if(mpPklData.get()) mpPklData->setHash(ImfXmlHelper::Convert(rHash));
}

void Asset::AffinityLost(QObject *pPklOrAm) {

	if(pPklOrAm == mpAssetMap) {
		bool old_affinity = HasAffinity();
		mpAssetMap = NULL;
		if(HasAffinity() == false && old_affinity == true) HandleAffinitiyLoss();
	}
	else if(pPklOrAm == mpPackageList) {
		bool old_affinity = HasAffinity();
		mpPackageList = NULL;
		if(HasAffinity() == false && old_affinity == true) HandleAffinitiyLoss();
	}
}

void Asset::AffinityWon(QObject *pPklOrAm) {

	AssetMap *p_am = qobject_cast<AssetMap*>(pPklOrAm);
	if(p_am) {
		mpAssetMap = p_am;
		if(HasAffinity() == true) HandleAffinitiyIncrement();
	}
	else {
		PackingList *p_pkl = qobject_cast<PackingList*>(pPklOrAm);
		if(p_pkl) {
			mpPackageList = p_pkl;
			if(HasAffinity() == true) HandleAffinitiyIncrement();
		}
		else qCritical() << "Couldn't extract Packing List or Asset Map.";
	}
}

void Asset::rAssetModified(Asset *pAsset) {

	mFilePath.refresh(); // Qt caches information (e.g. QFileInfo::exists()).
	if(mpPklData.get()) {
		mpPklData->setSize(xml_schema::PositiveInteger(mFilePath.size()));
		mpPklData->setOriginalFileName(ImfXmlHelper::Convert(UserText(mFilePath.fileName())));
	}
}

void Asset::FileModified() {

	mFileNeedsNewHash = true;
	emit AssetModified(this);
	//WR begin
	//This slot is called when wrapping was successful. "this" points to the Asset that was modified.
	//First we try to cast this to AssetMxfTrack
	AssetMxfTrack *assetMxfTrack = dynamic_cast<AssetMxfTrack*>(this);
	if (assetMxfTrack){
		//Here we call SetEssenceDescriptorSetAny, which extracts and sets mEssenceDescriptor
		assetMxfTrack->SetEssenceDescriptorSetAny(QString(this->GetPath().absoluteFilePath()));
		qDebug() << "Essence Descriptor updated for " << this->GetPath().absoluteFilePath();
	}
	//WR end
}

AssetPkl::AssetPkl(const QFileInfo &rFilePath, const am::AssetType &rAsset) :
Asset(Asset::pkl, rFilePath, rAsset) {

}

AssetPkl::AssetPkl(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::pkl, rFilePath, rId, rAnnotationText) {

}

AssetCpl::AssetCpl(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl::AssetType &rPklAsset) :
Asset(Asset::cpl, rFilePath, rAmAsset, std::auto_ptr<pkl::AssetType>(new pkl::AssetType(rPklAsset))) {
//WR begin
	mIsNewOrModified = false;
//WR end
}

AssetCpl::AssetCpl(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::cpl, rFilePath, rId, rAnnotationText) {
	//WR begin
	mIsNewOrModified = false;
	//WR end
}

AssetOpl::AssetOpl(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl::AssetType &rPklAsset) :
Asset(Asset::opl, rFilePath, rAmAsset, std::auto_ptr<pkl::AssetType>(new pkl::AssetType(rPklAsset))) {

}

AssetOpl::AssetOpl(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::opl, rFilePath, rId, rAnnotationText) {

}

AssetMxfTrack::AssetMxfTrack(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl::AssetType &rPklAsset) :
Asset(Asset::mxf, rFilePath, rAmAsset, std::auto_ptr<pkl::AssetType>(new pkl::AssetType(rPklAsset))), mMetadata(), mSourceFiles(), mFirstProxyImage(), mMetadataExtr() {
	mMetadataExtr.ReadMetadata(mMetadata, rFilePath.absoluteFilePath());
	SetDefaultProxyImages();
	//WR begin
	//New UUID for SourceENcoding
	mSourceEncoding = QUuid::createUuid();
	//new ED with SourceEncoding as ID
	mEssenceDescriptor = new cpl::EssenceDescriptorBaseType(ImfXmlHelper::Convert(mSourceEncoding));
	//Extract ED from MXF and write it into mEssenceDescriptor
	SetEssenceDescriptorSetAny(QString(rFilePath.absoluteFilePath()));

	//WR end
}

AssetMxfTrack::AssetMxfTrack(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::mxf, rFilePath, rId, rAnnotationText), mMetadata(), mSourceFiles(), mFirstProxyImage() {
	mSourceEncoding = QUuid::createUuid();
	mEssenceDescriptor = new cpl::EssenceDescriptorBaseType(ImfXmlHelper::Convert(mSourceEncoding));
	//leave ED empty because file does not exist yet on the file system


}

void AssetMxfTrack::SetSourceFiles(const QStringList &rSourceFiles) {

	if(Exists() == false) {
		mSourceFiles = rSourceFiles;
		if(mSourceFiles.isEmpty() == false) {
			if(is_wav_file(mSourceFiles.first()) == true) {
				mMetadataExtr.ReadMetadata(mMetadata, mSourceFiles.first());
				SetDefaultProxyImages();
				int channel_count = 0;
				for(int i = 0; i < mSourceFiles.size(); i++) {
					Metadata metadata;
					mMetadataExtr.ReadMetadata(metadata, mSourceFiles.at(i));
					channel_count += metadata.audioChannelCount;
				}
				mMetadata.audioChannelCount = channel_count;
				emit AssetModified(this);
			}


			/* -----Denis Manthey----- */
			else if(is_ttml_file(mSourceFiles.first()) == true) {
				mMetadataExtr.ReadMetadata(mMetadata, mSourceFiles.first());
				SetDefaultProxyImages();
				emit AssetModified(this);
			}
			/* -----Denis Manthey----- */


		}
	}
}

void AssetMxfTrack::SetFrameRate(const EditRate &rFrameRate) {

}

void AssetMxfTrack::SetSoundfieldGroup(const SoundfieldGroup &rSoundfieldGroup) {

	if(Exists() == false && GetEssenceType() == Metadata::Pcm) {
		mMetadata.soundfieldGroup = rSoundfieldGroup;
		emit AssetModified(this);
	}
}

void AssetMxfTrack::SetDuration(const Duration &rDuration) {

	if(Exists() == false && GetEssenceType() == Metadata::TimedText) {
		mMetadata.duration = rDuration;
		emit AssetModified(this);
	}
}

void AssetMxfTrack::rTransformationFinished(const QImage &rImage, const QVariant &rIdentifier /*= QVariant()*/) {

	if(rIdentifier.canConvert<QUuid>() == true && rIdentifier.toUuid() == GetId()) {
		mFirstProxyImage = rImage;
		emit AssetModified(this);
	}
}

void AssetMxfTrack::SetDefaultProxyImages() {

	switch(GetEssenceType()) {
		case Metadata::Jpeg2000:
			mFirstProxyImage = QImage(":/proxy_film.png");
			break;
		case Metadata::Pcm:
			mFirstProxyImage = QImage(":/proxy_sound.png");
			break;
		case Metadata::TimedText:
			mFirstProxyImage = QImage(":/proxy_text.png");
			break;
		case Metadata::Unknown_Type:
		default:
			mFirstProxyImage = QImage(":/proxy_unknown.png");
			break;
	}
}

//WR begin

void AssetMxfTrack::SetEssenceDescriptorSetAny(const QString &filePath) {
	QString fPath = filePath;
	QString classPath = QApplication::applicationDirPath() + QString("/regxmllib/regxmllib.jar");
	const unsigned short dictLength = 4;
	QString dict[dictLength] = {
		QApplication::applicationDirPath() + QString("/regxmllib/www-smpte-ra-org-reg-335-2012.xml"),
		QApplication::applicationDirPath() + QString("/regxmllib/www-smpte-ra-org-reg-335-2012-13-1-aaf.xml"),
		QApplication::applicationDirPath() + QString("/regxmllib/www-smpte-ra-org-reg-395-2014-13-1-aaf.xml"),
		QApplication::applicationDirPath() + QString("/regxmllib/www-smpte-ra-org-reg-2003-2012.xml")
	};
#ifdef WIN32
	classPath = QString("\"") + classPath + QString("\"");
	fPath = QString("\"") + fPath + QString("\"");
	for (int i=0; i < dictLength; i++) {
		dict[i] = QString("\"") + dict[i] + QString("\"");
	}
#else
	classPath = classPath.replace(" ", "\\ ");
	fPath = fPath.replace(" ", "\\ ");
	for (int i=0; i < dictLength; i++) {
		dict[i] = dict[i].replace(" ", "\\ ");
	}
#endif

	QString command = "java -cp " + classPath + " com.sandflow.smpte.tools.RegXMLDump -ed -d ";
	for (int i=0; i < dictLength; i++) {
		command += dict[i] + QString(" ");
	}
	command += QString("-i ") + fPath;

	std::string result = this->ssystem(command.toStdString().c_str());
	QString qresult = QString::fromStdString(result.c_str());

	if (result.length() > 0) {
		cpl::EssenceDescriptorBaseType::AnySequence &r_any_sequence(mEssenceDescriptor->getAny());
		xercesc::DOMElement * p_dom_element;
		const char * strEssenceDescriptor;
		xercesc::DOMNode * node;
		xercesc::XercesDOMParser * parser = new xercesc::XercesDOMParser();
		try {
			strEssenceDescriptor = qresult.toUtf8();
			xercesc::MemBufInputSource src((const XMLByte*)strEssenceDescriptor, qresult.length(), "dummy", false);
			parser->parse(src);

			xercesc::DOMDocument * p_dom_document2 = parser->getDocument();

			xercesc::DOMDocument &p_dom_document  = mEssenceDescriptor->getDomDocument();
			p_dom_element = p_dom_document2->getDocumentElement();
			if (p_dom_element) {
			  node = p_dom_document.importNode(p_dom_element, true);

			  // Appending the node to the new document
			  if (node) {
				  p_dom_document.appendChild(node);
			  }
			}
			p_dom_element = p_dom_document.getDocumentElement();
		}
	  catch (const xercesc::DOMException& toCatch) {
			char* message = xercesc::XMLString::transcode(toCatch.msg);
			qDebug() << "Exception message is:"
				 << QString(message);
			xercesc::XMLString::release(&message);
			delete parser;
			return;
	  }
	  catch (...) {
			qDebug() << "Failed to extract essence descriptor from " << filePath;
			return;
	  }

	  if (p_dom_element) {
		  //p_dom_element->setAttribute(X("xmlns"), NULL);
		  try {
		  r_any_sequence.push_back(p_dom_element);
		  mEssenceDescriptor->setAny(r_any_sequence);
		  }
		  catch (...) {
				qDebug() << "Failed to extract essence descriptor from " << filePath;
				return;
		  }
	  }
	  else
		  qDebug() << "p_dom_element == NULL";

	  delete parser;
  }
}
std::string AssetMxfTrack::ssystem (const char *command) {
    char tmpname [L_tmpnam];
    std::tmpnam ( tmpname );
    std::string scommand = command;
    std::string cmd = scommand + " >> " + tmpname;
    std::system(cmd.c_str());
    std::ifstream file(tmpname, std::ios::in );
    std::string result;
        if (file) {
      while (!file.eof()) result.push_back(file.get());
          file.close();
    }
    remove(tmpname);
    return result;
}
//WR end
