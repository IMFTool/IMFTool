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
#include "ImfPackage.h"
#include "global.h"
#include "SMPTE-2067-3-2013-CPL.h"
#include "SMPTE-2067-100a-2014-OPL.h"
#include "ImfMimeData.h"
#include <QFile>
#include <fstream>
#include <QThreadPool>
//WR begin
#include <QMessageBox>
#include <QProgressDialog>
#include <QProcess>
#include <QTemporaryFile>

#include "JobQueue.h"
#include "Jobs.h"
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

//regxmllibc
#include <com/sandflow/smpte/regxml/dict/MetaDictionaryCollection.h>
#include <com/sandflow/smpte/regxml/dict/importers/XMLImporter.h>
#include <com/sandflow/smpte/regxml/MXFFragmentBuilder.h>

XERCES_CPP_NAMESPACE_USE

using namespace rxml;
//regxmllibc

//WR end


ImfPackage::ImfPackage(const QDir &rWorkingDir) :
QAbstractTableModel(NULL), mpAssetMap(NULL), mPackingLists(), mAssetList(), mRootDir(rWorkingDir), mIsDirty(false), mIsIngest(false), mpJobQueue(NULL), mCplList() {

	mpAssetMap = new AssetMap(this, mRootDir.absoluteFilePath(ASSET_SEARCH_NAME));
	QUuid pkl_id = QUuid::createUuid();
	QString pkl_file_path(mRootDir.absoluteFilePath(QString("PKL_%1.xml").arg(strip_uuid(pkl_id))));
	QSharedPointer<AssetPkl> pkl_asset(new AssetPkl(pkl_file_path, pkl_id));
	mPackingLists.push_back(new PackingList(this, pkl_file_path, pkl_id));
	AddAsset(pkl_asset, QUuid());
	Init();
}

ImfPackage::ImfPackage(const QDir &rWorkingDir, const UserText &rIssuer, const UserText &rAnnotationText /*= QString()*/) :
QAbstractTableModel(NULL), mpAssetMap(NULL), mPackingLists(), mAssetList(), mRootDir(rWorkingDir), mIsDirty(true), mIsIngest(false), mpJobQueue(NULL), mCplList() {

	mpAssetMap = new AssetMap(this, mRootDir.absoluteFilePath(ASSET_SEARCH_NAME), rAnnotationText, rIssuer);
	QUuid pkl_id = QUuid::createUuid();
	QString pkl_file_path(mRootDir.absoluteFilePath(QString("PKL_%1.xml").arg(strip_uuid(pkl_id))));
	QSharedPointer<AssetPkl> pkl_asset(new AssetPkl(pkl_file_path, pkl_id));
	mPackingLists.push_back(new PackingList(this, pkl_file_path, pkl_id, QUuid(), QUuid(), rAnnotationText, rIssuer));
	AddAsset(pkl_asset, QUuid());
	Init();
}

void ImfPackage::Init() {
	//WR
	mpJobQueue = new JobQueue(this);
	mpJobQueue->SetInterruptIfError(true);
	connect(mpJobQueue, SIGNAL(finished()), this, SLOT(rJobQueueFinished()));
	mpProgressDialog = new QProgressDialog();
	mpProgressDialog->setWindowModality(Qt::WindowModal);
	mpProgressDialog->setMinimumSize(500, 150);
	mpProgressDialog->setMinimum(0);
	mpProgressDialog->setMaximum(100);
	mpProgressDialog->setValue(100);
	mpProgressDialog->setMinimumDuration(0);
	connect(mpJobQueue, SIGNAL(Progress(int)), mpProgressDialog, SLOT(setValue(int)));
	connect(mpJobQueue, SIGNAL(NextJobStarted(const QString&)), mpProgressDialog, SLOT(setLabelText(const QString&)));
	//connect(mpProgressDialog, SIGNAL(canceled()), mpJobQueue, SLOT(InterruptQueue()));
	mpMsgBox = new QMessageBox();
	//WR

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
			if (!error) CheckIfSupplemental();
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

	QList<QUuid> pkl_ids;
	QList<QString> pkl_file_paths;
	for (int i = 0; i< mPackingLists.size(); i++) {
		pkl_ids << QUuid::createUuid();
		pkl_file_paths << mRootDir.absoluteFilePath(QString("PKL_%1.xml").arg(strip_uuid(pkl_ids[i])));
	}
	//QString pkl_file_path(mRootDir.absoluteFilePath(QString("PKL_%1.xml").arg(strip_uuid(pkl_id))));

	for(int i = 0; i < mPackingLists.size(); i++) {
		if(mPackingLists.at(i)) {

			pkl2016::PackingListType packing_list(mPackingLists.at(i)->Write());
			packing_list.setAssetList(pkl2016::PackingListType_AssetListType());
			packing_list.getAssetList().setAsset(pkl2016::PackingListType_AssetListType::AssetSequence());
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
			packing_list.setId(ImfXmlHelper::Convert(pkl_ids[i]));

			std::ofstream pkl_ofs(mPackingLists.at(i)->GetFilePath().absoluteFilePath().toStdString().c_str(), std::ofstream::out);
			try {
				pkl2016::serializePackingList(pkl_ofs, packing_list, pkl_namespace, "UTF-8", xml_schema::Flags::dont_initialize);
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
				if (ImfXmlHelper::RemoveWhiteSpaces(mPackingLists.at(i)->GetFilePath().absoluteFilePath()))
					qDebug() << "Error removing XML whitespaces from " << mPackingLists.at(i)->GetFilePath().absoluteFilePath();
			}
			else break;
		}
	}

	QString old_pkl_file_path;
	QList<PackingList*>	rPackingLists;
	for(int i = 0; i < mPackingLists.size(); i++){
		old_pkl_file_path = mPackingLists.at(i)->GetFilePath().absoluteFilePath();
		RemoveAsset(mPackingLists.at(i)->GetId());
		QFile::rename(old_pkl_file_path, pkl_file_paths[i]);
		QSharedPointer<AssetPkl> pkl_asset(new AssetPkl(pkl_file_paths[i], pkl_ids[i]));
		AddAsset(pkl_asset, QUuid());
		rPackingLists.push_back(new PackingList(this, pkl_file_paths[i], pkl_ids[i]));
	}
	//QFile::rename(old_pkl_file_path, pkl_file_path);
	//QSharedPointer<AssetPkl> pkl_asset(new AssetPkl(pkl_file_path, pkl_id));
	mPackingLists.clear();
	//mPackingLists.push_back(new PackingList(this, pkl_file_path, pkl_id));
	mPackingLists = rPackingLists;
	//AddAsset(pkl_asset, QUuid());

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
				bool dont_write = false;
				if(mAssetList.at(i)->Exists()) {
					QSharedPointer<AssetMxfTrack> asset = mAssetList.at(i).objectCast<AssetMxfTrack>();
						if (!asset.isNull()) {
							if (asset->GetIsOutsidePackage()) dont_write = true;
						}
					if (!dont_write) asset_map.getAssetList().getAsset().push_back(mAssetList.at(i)->WriteAm());
				}
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
	std::unique_ptr<am::AssetMapType> p_asset_map;
	try {
		p_asset_map = am::parseAssetMap(rAssetMapFilePath.absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize);
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
		//WR
		mpJobQueue->FlushQueue();
		//WR
		if(p_asset_map->getVolumeCount() == 1) {
			mpAssetMap = new AssetMap(this, rAssetMapFilePath, *p_asset_map.get()); // add new Asset Map
			// We must find all Packing Lists.
			for(unsigned int i = 0; i < p_asset_map->getAssetList().getAsset().size(); i++) {
				am::AssetType asset = p_asset_map->getAssetList().getAsset().at(i);
				if(asset.getPackingList().present() && asset.getPackingList().get() == xml_schema::Boolean(true)) {
					// We found a Packing List.
					if(asset.getChunkList().getChunk().size() == 1) {
						QFileInfo packing_list_path = QFileInfo(mRootDir.absolutePath().append("/").append(asset.getChunkList().getChunk().back().getPath().c_str()));
						if(packing_list_path.exists() == true) {
							// ---Parse Packing List---
							std::unique_ptr<pkl::PackingListType> p_packing_list2013;
							std::unique_ptr<pkl2016::PackingListType> p_packing_list;
							try {
								p_packing_list2013 = (pkl::parsePackingList(QDir::toNativeSeparators(packing_list_path.absoluteFilePath()).toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize));
								qDebug() << "Parsing 2013 PKL";
							}
							catch(...) { parse_error = XmlParsingError(XmlParsingError::Unknown); }
							if(parse_error.IsError() == false) {
								p_packing_list = ImfXmlHelper::Convert(p_packing_list2013);
								qDebug() << "Success: Parsing 2013 PKL";
							} else {
								parse_error = XmlParsingError();
								try {
									p_packing_list = (pkl2016::parsePackingList(QDir::toNativeSeparators(packing_list_path.absoluteFilePath()).toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize));
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
							}
							if(parse_error.IsError() == false) {
								// XML parsing succeeds
								PackingList *p_new_packing_list = new PackingList(this, packing_list_path, *p_packing_list.get()); // add new Packing list
								mPackingLists.push_back(p_new_packing_list);
								// First add Packing List Asset itself.		
								AddAsset(QSharedPointer<AssetPkl>(new AssetPkl(packing_list_path, asset)), QUuid()); // PKL Id doesn't matter. It's a new Packing List which cannot be added to an existing PKL.
								// Add all Assets found in Packing List
								for(unsigned int i = 0; i < p_packing_list->getAssetList().getAsset().size(); i++) {
									pkl2016::AssetType pkl_asset = p_packing_list->getAssetList().getAsset().at(i);
									// Find equivalent Asset in Asset Map
									for(unsigned int ii = 0; ii < p_asset_map->getAssetList().getAsset().size(); ii++) {
										am::AssetType am_asset = p_asset_map->getAssetList().getAsset().at(ii);
										if(pkl_asset.getId() == am_asset.getId()) {
											if(am_asset.getChunkList().getChunk().size() == 1) {
												QFileInfo new_asset_path = QFileInfo(mRootDir.absolutePath().append("/").append(am_asset.getChunkList().getChunk().back().getPath().c_str()));
												if(pkl_asset.getType().compare(MIME_TYPE_MXF) == 0) {
													// Add Asset MXF Track
													QSharedPointer<AssetMxfTrack> mxf_track(new AssetMxfTrack(new_asset_path, am_asset, pkl_asset));
													AddAsset(mxf_track, ImfXmlHelper::Convert(p_packing_list->getId()));
													//WR
													JobExtractEssenceDescriptor *p_ed_job_c = new JobExtractEssenceDescriptor(mxf_track->GetPath().absoluteFilePath());
													connect(p_ed_job_c, SIGNAL(Result(const DOMDocument*, const QVariant&)), mxf_track.data(), SLOT(SetEssenceDescriptor(const DOMDocument*)));
													mpJobQueue->AddJob(p_ed_job_c);
													//WR
												}
												else if(pkl_asset.getType().compare(MIME_TYPE_XML) == 0) {
													// Add CPL or OPL
													// We have to parse the file to determine the type (opl or cpl)
													bool is_scm = true;
													bool is_opl = true;
													bool is_cpl = true;
													bool is_cpl2013 = true;
													std::unique_ptr< cpl2016::CompositionPlaylistType> cpl_data;
													std::unique_ptr< cpl::CompositionPlaylistType> cpl2013_data;
													try { cpl_data = cpl2016::parseCompositionPlaylist(new_asset_path.absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
													catch(...) { is_cpl = false; }
													try { cpl2013_data = cpl::parseCompositionPlaylist(new_asset_path.absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
													catch(...) {is_cpl2013 = false;}
													if (is_cpl2013) {
														QString tempFile = nullptr;
														bool conversionError = false;
															try {
																// This is a Q&D hack to convert ST 2067-3:2013 CPLs into ST 2067-3:2016 CPLs,
																// pending a more sophisticated solution using proper XLS Transformation.
																// QTemporaryFile was causing issues under Windows, the delete cpl2016_file_tmp command is required for Windows
																QTemporaryFile* cpl2016_file_tmp = new QTemporaryFile();
																if (!cpl2016_file_tmp->open()) {
																	qDebug() << "Cant't create temp file at " << cpl2016_file_tmp->fileName();
																	conversionError = true;
																	break;
																}
																QFile f_in(new_asset_path.absoluteFilePath());
																if (!f_in.open(QFile::ReadOnly | QFile::Text)) break;
																QTextStream in(&f_in);
																QString tempCPL = in.readAll();
																tempCPL.replace("http://www.smpte-ra.org/schemas/2067-3/2013", "http://www.smpte-ra.org/schemas/2067-3/2016");
																tempCPL.replace("http://www.smpte-ra.org/schemas/2067-2/2013", "http://www.smpte-ra.org/schemas/2067-2/2016");
																cpl2016_file_tmp->write(tempCPL.toUtf8());
																tempFile = cpl2016_file_tmp->fileName();
																cpl2016_file_tmp->setAutoRemove(false);
																if (!cpl2016_file_tmp->flush()) {
																	qDebug() << "Cant't flush to temp file at " << cpl2016_file_tmp->fileName();
																	conversionError = true;
																	break;
																}
																cpl2016_file_tmp->close();
																//if the QTemporaryFile instance is not explicitely deleted, it won't be accesible on Windows (weird)
																delete cpl2016_file_tmp;
																if (!conversionError)
																	is_cpl = true;
															}
															catch (...) { is_cpl = false; conversionError = true;  qDebug() << "Transformation of 2013 CPL to 2016 CPL failed"; }
															if (!conversionError) {
																parse_error = XmlParsingError();
																try { cpl_data = cpl2016::parseCompositionPlaylist(tempFile.toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
																catch (const xml_schema::Parsing &e) { is_cpl = false; parse_error = XmlParsingError(e); }
																catch (const xml_schema::ExpectedElement &e) { is_cpl = false; parse_error = XmlParsingError(e); }
																catch (const xml_schema::UnexpectedElement &e) { is_cpl = false; parse_error = XmlParsingError(e); }
																catch (const xml_schema::ExpectedAttribute &e) { is_cpl = false; parse_error = XmlParsingError(e); }
																catch (const xml_schema::UnexpectedEnumerator &e) { is_cpl = false; parse_error = XmlParsingError(e); }
																catch (const xml_schema::ExpectedTextContent &e) { is_cpl = false; parse_error = XmlParsingError(e); }
																catch (const xml_schema::NoTypeInfo &e) { is_cpl = false; parse_error = XmlParsingError(e); }
																catch (const xml_schema::NotDerived &e) { is_cpl = false; parse_error = XmlParsingError(e); }
																catch (const xml_schema::NoPrefixMapping &e) { is_cpl = false; parse_error = XmlParsingError(e); }
																catch (...) { is_cpl = false; qDebug() << "Parsing transformed 2016 CPL failed"; }
																qDebug() << parse_error;
																try { QFile::remove(tempFile); }
																catch (...) {} // just ignore..
															}
													}
													try { opl::parseOutputProfileList(new_asset_path.absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
													catch(...) { is_opl = false; }
													::std::unique_ptr< ::scm::SidecarCompositionMapType> p_scm;
													try { p_scm = scm::parseSidecarCompositionMap(new_asset_path.absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
													catch(...) { is_scm = false; }
													if(is_cpl && !is_opl && !is_scm) {
														// Add CPL
														QSharedPointer<AssetCpl> cpl(new AssetCpl(new_asset_path, am_asset, pkl_asset));
														AddAsset(cpl, ImfXmlHelper::Convert(p_packing_list->getId()));
														//WR
														mCplList << *cpl_data.get();
														mImpEditRates.push_back(ImfXmlHelper::Convert(cpl_data->getEditRate()));
														qDebug() << "CPL Edit Rate: " << mImpEditRates.last().GetNumerator()  << mImpEditRates.last().GetDenominator();
														//WR
													}
													else if(is_opl && !is_cpl && !is_scm) {
														// Add Opl
														QSharedPointer<AssetOpl> opl(new AssetOpl(new_asset_path, am_asset, pkl_asset));
														AddAsset(opl, ImfXmlHelper::Convert(p_packing_list->getId()));
													}
													else if(is_scm && !is_cpl && !is_opl) {
														// Add Scm
														QSharedPointer<AssetScm> scm_asset(new AssetScm(new_asset_path, am_asset, pkl_asset, *std::move(p_scm).get()));
														AddAsset(scm_asset, ImfXmlHelper::Convert(p_packing_list->getId()));
														mScmList << scm_asset;
													}
													else {
														// It will be checked below if the unknown asset is a sidecar asset
														QSharedPointer<Asset> unknown(new Asset(Asset::unknown, new_asset_path, am_asset, std::unique_ptr<pkl2016::AssetType>(new pkl2016::AssetType(pkl_asset))));
														AddAsset(unknown, ImfXmlHelper::Convert(p_packing_list->getId()));
													}
												}
												else { // MIME Type neither XML nor MXF
													// It will be checked below if the unknown asset is a sidecar asset
													QSharedPointer<Asset> unknown(new Asset(Asset::unknown, new_asset_path, am_asset, std::unique_ptr<pkl2016::AssetType>(new pkl2016::AssetType(pkl_asset))));
													AddAsset(unknown, ImfXmlHelper::Convert(p_packing_list->getId()));
												}
											}
											else {
												error = ImfError(ImfError::MultipleChunks, "", true);
												continue;
											}
										}
									}
								}
								int count = 0;
								// Remove dups in mImpEditRates
								/*for (QVector<EditRate>::iterator i=mImpEditRates.begin(); i < mImpEditRates.end(); i++) {
									// Remove duplicate frame rates
									while (int pos = mImpEditRates.lastIndexOf(*i) != count) {
										mImpEditRates.remove(count);
									}
									count ++;
								}*/
								foreach(QSharedPointer<Asset> asset, mAssetList) {
									// All assets: Check, if they are sidecar assets. If so, change type to Asset:scm
									// TTML XF assets: Set Edit Rate in metadata object to CPL Edit Rate, re-calculate duration in CPL Edit Rate units
									// Uses the first CPL Edit Rate in mImpEditRates, this can be an issue in multi-edit rate IMPs
									bool converted_to_sidecar = false;
									for (int i = 0; i < mScmList.length(); i++) {
										scm::SidecarCompositionMapType scm = mScmList[i].data()->GetScm();
										for (scm::SidecarCompositionMapType_SidecarAssetListType::SidecarAssetSequence::iterator
												it = scm.getSidecarAssetList().getSidecarAsset().begin();
												it < scm.getSidecarAssetList().getSidecarAsset().end();
												it++) {
											if (asset->GetId() == ImfXmlHelper::Convert(it->getId())) {
												const pkl2016::AssetType pkl_asset(*asset->WritePkl());
												QSharedPointer<AssetSidecar> sidecar_asset(new AssetSidecar(asset->GetPath(), asset->GetAmData(), pkl_asset));
												QUuid packing_list_id = asset->GetPklId();
												RemoveAsset(asset->GetId());
												AddAsset(sidecar_asset, packing_list_id);
												converted_to_sidecar = true;
												break;
											}
										}
										if (converted_to_sidecar) break;
									}
									QSharedPointer <AssetMxfTrack> assetMxfTrack;
									try {
										assetMxfTrack = qSharedPointerCast<AssetMxfTrack>(asset);
									}
									catch(...) {
										qDebug() << "Error in qSharedPointerCast";
										break;
									}
									if (!assetMxfTrack.isNull() && !mImpEditRates.isEmpty()){
										if (assetMxfTrack->GetEssenceType() == Metadata::TimedText) {
											if (!assetMxfTrack->GetTimedTextFrameRate().IsValid()) break;  //CPLs arbitrarily expose EssenceType == Metadata::TimedText
											assetMxfTrack->SetCplEditRate(mImpEditRates.first());
											assetMxfTrack->SetEditRate(mImpEditRates.first());
											if (assetMxfTrack->GetTimedTextFrameRate() != assetMxfTrack->GetEditRate()) {
												assetMxfTrack->SetDuration(Duration(ceil(assetMxfTrack->GetOriginalDuration().GetCount() / assetMxfTrack->GetTimedTextFrameRate().GetQuotient() * assetMxfTrack->GetEditRate().GetQuotient())));
												qDebug() << "Warning: SampleRate of Asset" << assetMxfTrack->GetId() << "does not match CPL EditRate!";
											} else {
												assetMxfTrack->SetDuration(assetMxfTrack->GetOriginalDuration());
											}
										}
									}
								}
								// Refresh list of SCMs. Note: Users may have picked SCM files, that don't belong to the IMP, as sidecar assets
								mScmList.clear();
								foreach(QSharedPointer<Asset> asset, mAssetList) {
									if (asset->GetType() == Asset::scm) {
										QSharedPointer <AssetScm> asset_scm = qSharedPointerCast<AssetScm>(asset);
										if (asset_scm) mScmList << asset_scm;

									}
								}
								// Loop through all SCMs
								for (int i = 0; i < mScmList.length(); i++) {
									scm::SidecarCompositionMapType_SidecarAssetListType::SidecarAssetSequence sidecar_asset_sequence_assets_found;
									scm::SidecarCompositionMapType scm = mScmList[i].data()->GetScm();
									// Loop through all sidecar assets
									for (scm::SidecarCompositionMapType_SidecarAssetListType::SidecarAssetSequence::iterator
											it = scm.getSidecarAssetList().getSidecarAsset().begin();
											it < scm.getSidecarAssetList().getSidecarAsset().end(); it++) {
										bool found = false;
										AssetScm::SidecarCompositionMapEntry entry;
										// Loop through all IMP assets
										foreach(QSharedPointer<Asset> asset, mAssetList) {
											// collect all sidecar assets that are present in sidecar_asset_sequence_assets_found
											if (asset->GetId() == ImfXmlHelper::Convert(it->getId())) {
												found = true;
												sidecar_asset_sequence_assets_found.push_back(*it);
												if (scm.getProperties().getAnnotation().present())
													mScmList[i].data()->SetAnnotationText(ImfXmlHelper::Convert(scm.getProperties().getAnnotation().get()));
												if (scm.getProperties().getIssuer().present())
													mScmList[i].data()->SetIssuer(ImfXmlHelper::Convert(scm.getProperties().getIssuer().get()));
												entry.id = ImfXmlHelper::Convert(it->getId());
												entry.filepath = asset.data()->GetPath();
												scm::SidecarAssetType::AssociatedCPLListType& cpl_list = it->getAssociatedCPLList();
												for (scm::SidecarAssetType_AssociatedCPLListType::CPLIdSequence::iterator
														it_cpl_id = cpl_list.getCPLId().begin();
														it_cpl_id < cpl_list.getCPLId().end(); it_cpl_id++) {
													//const QUuid sidecar_id = ImfXmlHelper::Convert(*it_cpl_id);
													bool cpl_found = false;
													foreach(QSharedPointer<Asset> asset, mAssetList) {
														if ((asset.data()->GetType() == Asset::cpl) && (asset.data()->GetId() == ImfXmlHelper::Convert(*it_cpl_id)) ) {
															QSharedPointer<AssetCpl> asset_cpl = asset.staticCast<AssetCpl>();
															if (asset_cpl) {
																entry.mAssociatedCplAssets.append(asset_cpl);
																cpl_found = true;
																break;
															}
														}
													}
													if (cpl_found == false) {
														entry.mCplIdsNotInCurrentImp.append(ImfXmlHelper::Convert(*it_cpl_id));
													}
												}
												mScmList[i].data()->AddSidecarCompositionMapEntry(entry);

												break;
											}
										}
										if (!found) {
											error = ImfError(ImfError::MissingSidecarAsset, ImfXmlHelper::Convert(mScmList[i].data()->GetScm().getId()).toString(), true);
										}
									}
									// Replace original SidecarAssetSequence with SidecarAssetSequence containing only sidecar assets that are present in IMP
									scm.getSidecarAssetList().setSidecarAsset(sidecar_asset_sequence_assets_found);
								} // END Loop through all SCMs
								foreach(QSharedPointer<Asset> asset, mAssetList) {
									if (asset->GetType() == Asset::unknown) {
										// Throw an error for each remaining unknown asset
										qDebug() << "Unknown Asset found: " << asset->GetId().toString();
										error = ImfError(ImfError::UnknownAsset, asset->GetId().toString(), true);
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
		//WR
		mpJobQueue->StartQueue();
		//WR
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
					if (rAsset->GetIsOutsidePackage()) {
						success = true;
						qDebug() << "Adding OV Asset" << rAsset->GetId() << rAsset->HasAffinity();
						connect(rAsset.data(), SIGNAL(AssetModified(Asset *)), this, SLOT(rAssetModified(Asset *)));
						beginInsertRows(QModelIndex(), mAssetList.size(), mAssetList.size());
						mAssetList.push_back(rAsset);
						endInsertRows();
						// Notify all GraphicWidgetResource with unknown mAsset
						emit ImpAssetModified(QSharedPointer<Asset>(rAsset));
					} else {
						success = false;
						qWarning() << "Couldn't add Asset: " << rAsset->GetId() << ": No Packing List available.";
					}
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
		if (role == Qt::ForegroundRole && mAssetList.at(row)->GetIsOutsidePackage()) { // Assets not belonging to the IMP are yellow.
			return QColor(mAssetList.at(row)->GetColor());
		}
		if(column == ImfPackage::ColumnIcon) {
			// icon
			if(role == Qt::DecorationRole) {
				switch(mAssetList.at(row)->GetType()) {
					case Asset::mxf:
						return QVariant(QPixmap(":/asset_mxf.png"));
					case Asset::cpl:
						return QVariant(QPixmap(":/asset_cpl.png"));
					case Asset::opl:
						return QVariant(QPixmap(":/asset_opl.png"));
					case Asset::scm:
						return QVariant(QPixmap(":/asset_scm.png"));
					case Asset::sidecar:
						return QVariant(QPixmap(":/asset_sidecar.png"));
					default:
						return QVariant(QPixmap(":/asset_unknown.png"));
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
					case Asset::cpl:
						return QVariant("cpl");
					case Asset::opl:
						return QVariant("opl");
					case Asset::pkl:
						return QVariant("pkl");
					default:
						return QVariant("unknown");
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
					quint64 size = mAssetList.at(row)->GetSize();
					if(size < 1048576) return QVariant(QString::number((double)size / 1024., 'f', 2).append(" KiB"));
					else if(size < 1073741824) return QVariant(QString::number((double)size / 1048576., 'f', 2).append(" MiB"));
					else return QVariant(QString::number((double)size / 1073741824., 'f', 2).append(" GiB"));
				}
				else if (mAssetList.at(row)->GetType() == Asset::mxf) {
					QSharedPointer <AssetMxfTrack> assetMxfTrack = qSharedPointerCast<AssetMxfTrack>(mAssetList.at(row));
					if (!assetMxfTrack->HasSourceFiles()) return QVariant(tr("Missing file"));
					else return QVariant(tr("Not Finalized"));
				}
				else if (mAssetList.at(row)->GetType() == Asset::scm) {
					QSharedPointer <AssetScm> assetScm = qSharedPointerCast<AssetScm>(mAssetList.at(row));
					if (!assetScm->GetIsNew()) return QVariant(tr("Missing file"));
					else return QVariant(tr("Not Finalized"));
				}
				else return QVariant(tr("Missing file"));
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

	if ((column == ImfPackage::ColumnAnnotation) && (row < mAssetList.size()) && !mAssetList.at(row)->GetIsOutsidePackage())ret |= Qt::ItemIsEditable;
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

//WR

void ImfPackage::rJobQueueFinished() {
	mpProgressDialog->reset();
	QString error_msg;
	QList<Error> errors = mpJobQueue->GetErrors();
	for(int i = 0; i < errors.size(); i++) {
		error_msg.append(QString("%1: %2\n%3\n").arg(i + 1).arg(errors.at(i).GetErrorMsg()).arg(errors.at(i).GetErrorDescription()));
	}
	error_msg.chop(1); // remove last \n
	if (error_msg != "") {
		qDebug() << "rJobQueueFinished error:" << error_msg;
		mpMsgBox->setText(tr("Critical error, can't extract Essence Descriptor:"));
		mpMsgBox->setInformativeText(error_msg + "\n\n Aborting to extract Essence Descriptors");
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->setIcon(QMessageBox::Critical);
		mpMsgBox->exec();
	}
}

bool ImfPackage::selectedIsOutsidePackage(const QModelIndex &selected) {

		const int row = selected.row();

		if (mAssetList.at(row)->GetIsOutsidePackage())
			return true;
		else
			return false;
}

/*
 * This method checks if an IMP is Supplemental, i.e. if at least one resource referenced by a CPL is missing.
 */
void ImfPackage::CheckIfSupplemental() {
	// Loop all CPLs
	QList<QUuid> cpl_track_file_uuids;
	for (int i = 0; i < mCplList.size(); i++ ) {
		// Loop segments
		for(int ii = 0; ii < mCplList.at(i).getSegmentList().getSegment().size(); ii++) {

			cpl2016::CompositionPlaylistType_SegmentListType::SegmentType *r_segment = new cpl2016::CompositionPlaylistType_SegmentListType::SegmentType(mCplList.at(i).getSegmentList().getSegment().at(ii));
			cpl2016::SegmentType::SequenceListType &r_sequence_list = r_segment->getSequenceList();
			cpl2016::SegmentType_SequenceListType::AnySequence &r_any_sequence(r_sequence_list.getAny());
			// Loop sequences
			for(cpl2016::SegmentType_SequenceListType::AnySequence::iterator sequence_iter(r_any_sequence.begin()); sequence_iter != r_any_sequence.end(); ++sequence_iter) {
				xercesc::DOMElement& element(*sequence_iter);
				cpl2016::SequenceType sequence(element);
				// Loop resources
				for(cpl2016::SequenceType_ResourceListType::ResourceIterator resource_iter(sequence.getResourceList().getResource().begin()); resource_iter != sequence.getResourceList().getResource().end(); ++resource_iter) {
					cpl2016::TrackFileResourceType *p_file_resource = dynamic_cast<cpl2016::TrackFileResourceType*>(&(*resource_iter));
					if(p_file_resource) {
						//Add CPL Track File IDs to QList
						cpl_track_file_uuids << ImfXmlHelper::Convert(p_file_resource->getTrackFileId());
					}
				}
			}
		}
	}
	// Loop CPL Track File IDs
	for (int i = 0; i < cpl_track_file_uuids.size(); i++) {
		bool found = false;
		// Loop ASSETMAP Assets
		for (int ii=0; ii < mAssetList.size(); ii++) {
			QSharedPointer <AssetMxfTrack> assetMxfTrack = qSharedPointerCast<AssetMxfTrack>(mAssetList.at(ii));
			if (!assetMxfTrack.isNull())
				if (cpl_track_file_uuids.at(i) == assetMxfTrack->GetId())
					found = true; // CPL Track File UUID belongs to a resource contained in the ASSETMAP!
		}
		if (!found) // It's a Supplemental Package!
			mIsSupplemental = true;
	}
}

QVector<QString> ImfPackage::GetApplicationIdentificationList() {
	QVector<QString> appList;
	foreach (cpl2016::CompositionPlaylistType cpl, mCplList) {
		if (cpl.getExtensionProperties().present()) {
			cpl2016::CompositionPlaylistType_ExtensionPropertiesType sequence_list = cpl.getExtensionProperties().get();
			cpl2016::CompositionPlaylistType_ExtensionPropertiesType::AnySequence &r_any_sequence(sequence_list.getAny());
			QString app = UserText(XMLString::transcode(r_any_sequence.front().getFirstChild()->getNodeValue())).first;
			if (!appList.contains(app)) appList << app;
		}
	}
	return appList;
}

//WR

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


PackingList::PackingList(ImfPackage *pParent, const QFileInfo &rFilePath, const pkl2016::PackingListType &rPackingList) :
QObject(pParent), mFilePath(rFilePath), mData(rPackingList) {

	mData.setAssetList(pkl2016::PackingListType_AssetListType());
	if(mData.getSigner().present() == true) qWarning() << "Signer is in Packing List not supported.";
	if(mData.getSignature().present() == true) qWarning() << "Signature is in Packing List not supported.";
}

PackingList::PackingList(ImfPackage *pParent, const QFileInfo &rFilePath, const QUuid &rId, const QUuid &rIconId /*= QUuid()*/, const QUuid &rGroupId /*= QUuid()*/, const UserText &rAnnotationText /*= QString()*/, const UserText &rIssuer /*= QString()*/) :
QObject(pParent), mFilePath(rFilePath),
mData(ImfXmlHelper::Convert(rId),
ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()),
ImfXmlHelper::Convert(UserText(rIssuer)),
ImfXmlHelper::Convert(UserText(CREATOR_STRING)),
pkl2016::PackingListType_AssetListType()
) {

	if(rAnnotationText.IsEmpty() == false) SetAnnotationText(rAnnotationText);
	if(rIconId.isNull() == false) mData.setIconId(ImfXmlHelper::Convert(rIconId));
	if(rGroupId.isNull() == false) mData.setGroupId(ImfXmlHelper::Convert(rGroupId));
}

const pkl2016::PackingListType& PackingList::Write() {

	// Update values before serialization.
	mData.setCreator(ImfXmlHelper::Convert(UserText(CREATOR_STRING)));
	mData.setIssueDate(ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()));
	// Remove Signature and Signer. We don't support this.
	mData.setSignature(nullptr);
	mData.setSigner(nullptr);
	return mData;
}

// Creates new Asset. rFilePath must be the prospective path of the asset. If the asset doesn't exist on the file system when ImfPackage::Outgest() is invoked the asset will be ignored.
Asset::Asset(eAssetType type, const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
QObject(NULL), mpAssetMap(NULL), mpPackageList(NULL), mType(type), mFilePath(rFilePath),
mAmData(ImfXmlHelper::Convert(QUuid() /*empty*/), am::AssetType_ChunkListType() /*empty*/),
mpPklData(nullptr), mFileNeedsNewHash(true), mIsOutsidePackage(false) {

	if(mType != pkl) {
		mpPklData = std::unique_ptr<pkl2016::AssetType>(new pkl2016::AssetType(
			ImfXmlHelper::Convert(QUuid() /*empty*/),
			ImfXmlHelper::Convert(QByteArray() /*empty*/),
			xml_schema::PositiveInteger(0) /*empty*/,
			mType == mxf ? xml_schema::String(MIME_TYPE_MXF) : xml_schema::String(MIME_TYPE_XML),
			pkl2016::AssetType::HashAlgorithmType(ds::CanonicalizationMethodType::AlgorithmType("http://www.w3.org/2000/09/xmldsig#sha1"))
			)
		);
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
Asset::Asset(eAssetType type, const QFileInfo &rFilePath, const am::AssetType &rAsset, std::unique_ptr<pkl2016::AssetType> assetType /* =nullptr */) :
QObject(NULL), mpAssetMap(NULL), mpPackageList(NULL), mType(type), mFilePath(rFilePath), mAmData(rAsset), mpPklData(std::move(assetType)), mFileNeedsNewHash(false), mIsOutsidePackage(false) {

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

const std::unique_ptr<pkl2016::AssetType>& Asset::WritePkl() {

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
		//Here we call ExtractEssenceDescriptor, which extracts and sets mEssenceDescriptor
		//assetMxfTrack->ExtractEssenceDescriptor(QString(this->GetPath().absoluteFilePath()));
		assetMxfTrack->ExtractEssenceDescriptor(QString(this->GetPath().absoluteFilePath()));
		qDebug() << "Essence Descriptor updated for " << this->GetPath().absoluteFilePath();
	}
	AssetScm *asset_scm = dynamic_cast<AssetScm*>(this);
	if (asset_scm) {
		// Per ST 2067-2, SCM Id in AM and PKL are RFC4122 Type 5 UUIDs using the entire asset as name.
		// Thus, a new Id needs to be calculated if an SCM is modified or created.
		QUuid new_type5_uuid;
		UUIDVersion5::CalculateFromEntireFile(UUIDVersion5::s_asset_id_prefix, asset_scm->GetPath().absoluteFilePath(), new_type5_uuid);
		am::AssetType new_am_data = asset_scm->GetAmData();
		new_am_data.setId(ImfXmlHelper::Convert(new_type5_uuid));
		asset_scm->SetAmData(new_am_data);
		asset_scm->GetPklData()->setId(ImfXmlHelper::Convert(new_type5_uuid));
	}
	//WR end
}

AssetPkl::AssetPkl(const QFileInfo &rFilePath, const am::AssetType &rAsset) :
Asset(Asset::pkl, rFilePath, rAsset) {

}

AssetPkl::AssetPkl(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::pkl, rFilePath, rId, rAnnotationText) {

}

AssetCpl::AssetCpl(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset) :
Asset(Asset::cpl, rFilePath, rAmAsset, std::unique_ptr<pkl2016::AssetType>(new pkl2016::AssetType(rPklAsset))) {
//WR begin
	mIsNewOrModified = false;
//WR end
	mIsNew = false;
}

AssetCpl::AssetCpl(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::cpl, rFilePath, rId, rAnnotationText) {
	//WR begin
	mIsNewOrModified = false;
	//WR end
	mIsNew = false;
}

AssetOpl::AssetOpl(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset) :
Asset(Asset::opl, rFilePath, rAmAsset, std::unique_ptr<pkl2016::AssetType>(new pkl2016::AssetType(rPklAsset))) {

}

AssetOpl::AssetOpl(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::opl, rFilePath, rId, rAnnotationText) {

}

AssetMxfTrack::AssetMxfTrack(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset) :
Asset(Asset::mxf, rFilePath, rAmAsset, std::unique_ptr<pkl2016::AssetType>(new pkl2016::AssetType(rPklAsset))), mMetadata(), mSourceFiles(), mFirstProxyImage(), mMetadataExtr() {
	mMetadataExtr.ReadMetadata(mMetadata, rFilePath.absoluteFilePath());
	SetDefaultProxyImages();
	//WR begin
	//New UUID for SourceENcoding
	mSourceEncoding = QUuid::createUuid();
	//new ED with SourceEncoding as ID
	mEssenceDescriptor = new cpl2016::EssenceDescriptorBaseType(ImfXmlHelper::Convert(mSourceEncoding));
	mIsNew = false;
	//WR end
}

AssetMxfTrack::AssetMxfTrack(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::mxf, rFilePath, rId, rAnnotationText), mMetadata(), mSourceFiles(), mFirstProxyImage() {
	mSourceEncoding = QUuid::createUuid();
	mEssenceDescriptor = new cpl2016::EssenceDescriptorBaseType(ImfXmlHelper::Convert(mSourceEncoding));
	//leave ED empty because file does not exist yet on the file system

}

AssetMxfTrack::AssetMxfTrack(const QFileInfo &rFilePath, const Metadata &rMetadata, const UserText &rAnnotationText) :
		Asset(Asset::mxf, rFilePath, rMetadata.assetId, rAnnotationText), mMetadata(rMetadata), mSourceFiles(), mFirstProxyImage() {
			SetDefaultProxyImages();
			mSourceEncoding = QUuid::createUuid();
			mEssenceDescriptor = new cpl2016::EssenceDescriptorBaseType(ImfXmlHelper::Convert(mSourceEncoding));
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
				mMetadataExtr.SetCplEditRate(mCplEditRate);
				mMetadataExtr.ReadMetadata(mMetadata, mSourceFiles.first());
				SetDefaultProxyImages();
				emit AssetModified(this);
			}
			/* -----Denis Manthey----- */


		}
	}
}

void AssetMxfTrack::SetFrameRate(const EditRate &rFrameRate) {

	if(Exists() == false && GetEssenceType() == Metadata::TimedText) {
		mMetadata.editRate = rFrameRate;
		emit AssetModified(this);
	}
}

void AssetMxfTrack::SetSoundfieldGroup(const SoundfieldGroup &rSoundfieldGroup) {

	if(Exists() == false && GetEssenceType() == Metadata::Pcm) {
		mMetadata.soundfieldGroup = rSoundfieldGroup;
		emit AssetModified(this);
	}
}

void AssetMxfTrack::SetDuration(const Duration &rDuration) {

	//if(Exists() == false && GetEssenceType() == Metadata::TimedText) {
	if(GetEssenceType() == Metadata::TimedText) {
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
#ifdef APP5_ACES
		case Metadata::Aces:
#endif
#ifdef CODEC_HTJ2K
		case Metadata::HTJ2K:
#endif
		case Metadata::Jpeg2000:
		case Metadata::ProRes:
			mFirstProxyImage = QImage(":/proxy_film.png");
			break;
		case Metadata::Pcm:
			mFirstProxyImage = QImage(":/proxy_sound.png");
			break;
		case Metadata::TimedText:
			mFirstProxyImage = QImage(":/proxy_text.png");
			break;
		case Metadata::IAB:
			mFirstProxyImage = QImage(":/proxy_iab_sound.png");
			break;
		case Metadata::ISXD:
			mFirstProxyImage = QImage(":/proxy_xml.png");
			break;
		case Metadata::Unknown_Type:
		default:
			mFirstProxyImage = QImage(":/proxy_unknown.png");
			break;
	}
}

//WR begin

/*Error AssetMxfTrack::ExtractEssenceDescriptor(const QString &filePath) {
	QString qresult;
	QProcess *myProcess = new QProcess();
	const QString program = "java";
	QStringList arg;
	Error error;
	arg << "-cp";
	arg << QApplication::applicationDirPath() + QString("/regxmllib/regxmllib.jar");
	arg << "com.sandflow.smpte.tools.RegXMLDump";
	arg << "-ed";
	arg << "-d";
	arg << QApplication::applicationDirPath() + QString("/regxmllib/www-smpte-ra-org-reg-335-2012.xml");
	arg << QApplication::applicationDirPath() + QString("/regxmllib/www-smpte-ra-org-reg-335-2012-13-1-aaf.xml");
	arg << QApplication::applicationDirPath() + QString("/regxmllib/www-smpte-ra-org-reg-395-2014-13-1-aaf.xml");
	arg << QApplication::applicationDirPath() + QString("/regxmllib/www-smpte-ra-org-reg-2003-2012.xml");
	arg << "-i";
	arg << filePath;
	myProcess->start(program, arg);
	myProcess->waitForFinished(-1);
	if (myProcess->exitStatus() == QProcess::NormalExit) {
		if (myProcess->exitCode() != 0) { return error = Error(Error::ExitCodeNotZero); }
	} else {
		return error = Error(Error::ExitStatusError);
	}

	try {
		qresult = myProcess->readAllStandardOutput();
	}
	catch (...) {
		return error = Error(Error::EssenceDescriptorExtraction);
	}

	if(error.IsError() == false) {
		SetEssenceDescriptor(qresult);
	}
	return error;
}*/

Error AssetMxfTrack::ExtractEssenceDescriptor(const QString &filePath) {
	QFile file(filePath);
	if(file.open(QIODevice::ReadOnly) == false) {
		return Error(Error::SourceFileOpenError, file.fileName());
	}
	Error error;

	QList<QString> dicts_fname = QList<QString>()
		<< "www-smpte-ra-org-reg-395-2014-13-1-aaf-phdr.xml"
		<< "www-smpte-ra-org-reg-2003-2012.xml"
		<< "www-smpte-ra-org-reg-335-2012-phdr.xml"
		<< "www-smpte-ra-org-reg-335-2012-13-1-aaf.xml"
		;

	XMLPlatformUtils::Initialize();

	XercesDOMParser *parser = new XercesDOMParser;
	parser->setCreateEntityReferenceNodes(true);
	parser->setDisableDefaultEntityResolution(true);
	parser->setDoNamespaces(true);

	MetaDictionaryCollection mds;

	qDebug() << dicts_fname.size();
	for (int i = 0; i < dicts_fname.size(); i++) {

		QString dict_path = QApplication::applicationDirPath() + QString("/regxmllib/") + dicts_fname[i];
		parser->parse(dict_path.toStdString().c_str());
		DOMDocument *doc = parser->getDocument();
		MetaDictionary *md = new MetaDictionary();
		XMLImporter::fromDOM(*doc, *md);
		mds.addDictionary(md);
	}

	XMLCh tempStr[3] = { chLatin_L, chLatin_S, chNull };
	DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(tempStr);
	DOMLSOutput       *output = ((DOMImplementationLS*)impl)->createLSOutput();

	DOMDocument *doc = impl->createDocument();

	std::ifstream f(filePath.toStdString().c_str(), std::ifstream::in | std::ifstream::binary);

	if (!f.good()) {
		qDebug() << "Can't read file:" << filePath;
		error = Error(Error::EssenceDescriptorExtraction);

	}
	static const rxml::UL ESSENCE_DESCRIPTOR_KEY = "urn:smpte:ul:060e2b34.02010101.0d010101.01012400";
	const rxml::AUID* ed_auid = new rxml::AUID(ESSENCE_DESCRIPTOR_KEY);
	DOMDocumentFragment* frag = MXFFragmentBuilder::fromInputStream(f, mds, NULL, ed_auid, *doc);

	doc->appendChild(frag);


	if(error.IsError() == false) {
		SetEssenceDescriptor(doc);
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


/*void AssetMxfTrack::SetEssenceDescriptor(const QString& qresult) {
	if (!qresult.isEmpty()) {
		cpl2016::EssenceDescriptorBaseType* rEssenceDescriptor = this->GetEssenceDescriptor();
		cpl2016::EssenceDescriptorBaseType::AnySequence &r_any_sequence(rEssenceDescriptor->getAny());
		xercesc::DOMElement * p_dom_element;
		xercesc::DOMNode * node;
		xercesc::XercesDOMParser * parser = new xercesc::XercesDOMParser();
		try {
			unsigned char * strEssenceDescriptor = new unsigned char[qresult.length()];
			memcpy(strEssenceDescriptor, qresult.toStdString().c_str(), qresult.size());
			xercesc::MemBufInputSource src((const XMLByte*)strEssenceDescriptor, qresult.length(), "dummy", false);
			parser->parse(src);
			delete[] strEssenceDescriptor;

			xercesc::DOMDocument * p_dom_document2 = parser->getDocument();

			xercesc::DOMDocument &p_dom_document  = rEssenceDescriptor->getDomDocument();
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
			qDebug() << "Failed to extract essence descriptor"; // from " << mFilePath;
			return;
	  }

	  if (p_dom_element) {
		  //p_dom_element->setAttribute(X("xmlns"), NULL);
		  try {
		  r_any_sequence.push_back(p_dom_element);
		  rEssenceDescriptor->setAny(r_any_sequence);
		  }
		  catch (...) {
				qDebug() << "Failed to extract essence descriptor"; // from " << mFilePath;
				return;
		  }
	  }
	  else
		  qDebug() << "p_dom_element == NULL";

	  delete parser;
	}
}*/

void AssetMxfTrack::SetEssenceDescriptor(const DOMDocument* dom_result) {
	if (dom_result) {
		cpl2016::EssenceDescriptorBaseType* rEssenceDescriptor = this->GetEssenceDescriptor();
		cpl2016::EssenceDescriptorBaseType::AnySequence &r_any_sequence(rEssenceDescriptor->getAny());
		xercesc::DOMElement * p_dom_element;
		xercesc::DOMNode * node;
		try {
			xercesc::DOMDocument &p_dom_document  = rEssenceDescriptor->getDomDocument();
			p_dom_element = dom_result->getDocumentElement();
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
			//delete parser;
			return;
	  }
	  catch (...) {
			qDebug() << "Failed to extract essence descriptor"; // from " << mFilePath;
			return;
	  }

	  if (p_dom_element) {
		  //p_dom_element->setAttribute(X("xmlns"), NULL);
		  try {
		  r_any_sequence.push_back(p_dom_element);
		  rEssenceDescriptor->setAny(r_any_sequence);
		  }
		  catch (...) {
				qDebug() << "Failed to extract essence descriptor"; // from " << mFilePath;
				return;
		  }
	  }
	  else
		  qDebug() << "p_dom_element == NULL";

	  //delete parser;
	}
}
//! Import SCM.
AssetScm::AssetScm(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset, const scm::SidecarCompositionMapType &rSidecarCompositionMap) :
Asset(Asset::scm, rFilePath, rAmAsset, std::unique_ptr<pkl2016::AssetType>(new pkl2016::AssetType(rPklAsset))), mIsNew(false), mData(rSidecarCompositionMap) {

		//mData.setProperties(scm::SidecarCompositionMapType_PropertiesType());
		if(mData.getSigner().present() == true) qWarning() << "Signer is in Packing List not supported.";
		if(mData.getSignature().present() == true) qWarning() << "Signature is in Packing List not supported.";
}

//! Create New SCM.
AssetScm::AssetScm(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::scm, rFilePath, rId, rAnnotationText), mIsNew(true),
			mData(ImfXmlHelper::Convert(rId),
			scm::SidecarCompositionMapType_PropertiesType(ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc())),
			scm::SidecarCompositionMapType_SidecarAssetListType()
			) {
		if(rAnnotationText.IsEmpty() == false) SetAnnotationText(rAnnotationText);

}

const scm::SidecarCompositionMapType& AssetScm::WriteScm() {

	scm::SidecarCompositionMapType::IdType uuid = ImfXmlHelper::Convert(QUuid::createUuid());
	scm::SidecarCompositionMapType::PropertiesType scm_props(ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()));
	scm::SidecarCompositionMapType::SidecarAssetListType::SidecarAssetSequence  scm_asset_sequence;
	scm::SidecarCompositionMapType_SidecarAssetListType scm_asset_list;
	mData.getProperties().setAnnotation(ImfXmlHelper::Convert(this->GetAnnotationText()));
	mData.getProperties().setIssuer(ImfXmlHelper::Convert(this->GetIssuer()));
	for (int i = 0; i < this->GetSidecarCompositionMapEntries().length(); i++) {
		scm::SidecarAssetType_AssociatedCPLListType cpl_list;
		scm::SidecarAssetType_AssociatedCPLListType::CPLIdSequence cpl_sequence;
		for (int j = 0; j < this->GetSidecarCompositionMapEntries()[i].mAssociatedCplAssets.length(); j++) {
			QUuid cpl_id = GetSidecarCompositionMapEntries()[i].mAssociatedCplAssets[j].data()->GetId();
			cpl_sequence.push_back(ImfXmlHelper::Convert(cpl_id));
		}
		for (int j = 0; j < this->GetSidecarCompositionMapEntries()[i].mCplIdsNotInCurrentImp.length(); j++) {
			QUuid cpl_id = GetSidecarCompositionMapEntries()[i].mCplIdsNotInCurrentImp[j];
			cpl_sequence.push_back(ImfXmlHelper::Convert(cpl_id));
		}
		cpl_list.setCPLId(cpl_sequence);
		scm::SidecarAssetType  scm_asset(ImfXmlHelper::Convert(this->GetSidecarCompositionMapEntries()[i].id), cpl_list);
		scm_asset_sequence.push_back(scm_asset);
	}
	scm_asset_list.setSidecarAsset(scm_asset_sequence);
	mData.setSidecarAssetList(scm_asset_list);
	// Update values before serialization.
	mData.getProperties().setIssueDate(ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()));
	// Remove Signature and Signer. We don't support this.
	mData.setSignature(nullptr);
	mData.setSigner(nullptr);
	return mData;
}

bool AssetScm::AddSidecarCompositionMapEntry(const SidecarCompositionMapEntry &rSidecarCompositionMapEntry) {
	bool return_value = true;
	for ( int i=0; i < mSidecarCompositionMapEntries.count(); i++) {
		if (mSidecarCompositionMapEntries.at(i).filepath == rSidecarCompositionMapEntry.filepath ) {
			return_value = false;
			break;
		}
	}
	if (return_value) mSidecarCompositionMapEntries.append(rSidecarCompositionMapEntry);
	return return_value;
}


AssetSidecar::AssetSidecar(const QFileInfo &rFilePath, const am::AssetType &rAmAsset, const pkl2016::AssetType &rPklAsset) :
Asset(Asset::sidecar, rFilePath, rAmAsset, std::unique_ptr<pkl2016::AssetType>(new pkl2016::AssetType(rPklAsset))), mIsNew(false) {

}

AssetSidecar::AssetSidecar(const QFileInfo &rFilePath, const QUuid &rId, const UserText &rAnnotationText /*= QString()*/) :
Asset(Asset::sidecar, rFilePath, rId, rAnnotationText), mIsNew(false) {

}



//WR end
