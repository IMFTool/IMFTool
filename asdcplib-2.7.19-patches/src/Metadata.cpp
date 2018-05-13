/*
Copyright (c) 2005-2017, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*! \file    Metadata.cpp
    \version $Id: Metadata.cpp,v 1.45 2016/12/10 19:57:45 jhurst Exp $       
    \brief   AS-DCP library, MXF Metadata Sets implementation
*/


#include <KM_mutex.h>
#include "Metadata.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

const ui32_t kl_length = ASDCP::SMPTE_UL_LENGTH + ASDCP::MXF_BER_LENGTH;

//------------------------------------------------------------------------------------------

static InterchangeObject* Preface_Factory(const Dictionary*& Dict) { return new Preface(Dict); }
static InterchangeObject* IndexTableSegment_Factory(const Dictionary*& Dict) { return new IndexTableSegment(Dict); }

static InterchangeObject* Identification_Factory(const Dictionary*& Dict) { return new Identification(Dict); }
static InterchangeObject* ContentStorage_Factory(const Dictionary*& Dict) { return new ContentStorage(Dict); }
static InterchangeObject* EssenceContainerData_Factory(const Dictionary*& Dict) { return new EssenceContainerData(Dict); }
static InterchangeObject* MaterialPackage_Factory(const Dictionary*& Dict) { return new MaterialPackage(Dict); }
static InterchangeObject* SourcePackage_Factory(const Dictionary*& Dict) { return new SourcePackage(Dict); }
static InterchangeObject* StaticTrack_Factory(const Dictionary*& Dict) { return new StaticTrack(Dict); }
static InterchangeObject* Track_Factory(const Dictionary*& Dict) { return new Track(Dict); }
static InterchangeObject* Sequence_Factory(const Dictionary*& Dict) { return new Sequence(Dict); }
static InterchangeObject* SourceClip_Factory(const Dictionary*& Dict) { return new SourceClip(Dict); }
static InterchangeObject* TimecodeComponent_Factory(const Dictionary*& Dict) { return new TimecodeComponent(Dict); }
static InterchangeObject* FileDescriptor_Factory(const Dictionary*& Dict) { return new FileDescriptor(Dict); }
static InterchangeObject* GenericSoundEssenceDescriptor_Factory(const Dictionary*& Dict) { return new GenericSoundEssenceDescriptor(Dict); }
static InterchangeObject* WaveAudioDescriptor_Factory(const Dictionary*& Dict) { return new WaveAudioDescriptor(Dict); }
static InterchangeObject* GenericPictureEssenceDescriptor_Factory(const Dictionary*& Dict) { return new GenericPictureEssenceDescriptor(Dict); }
static InterchangeObject* RGBAEssenceDescriptor_Factory(const Dictionary*& Dict) { return new RGBAEssenceDescriptor(Dict); }
static InterchangeObject* JPEG2000PictureSubDescriptor_Factory(const Dictionary*& Dict) { return new JPEG2000PictureSubDescriptor(Dict); }
static InterchangeObject* CDCIEssenceDescriptor_Factory(const Dictionary*& Dict) { return new CDCIEssenceDescriptor(Dict); }
static InterchangeObject* MPEG2VideoDescriptor_Factory(const Dictionary*& Dict) { return new MPEG2VideoDescriptor(Dict); }
static InterchangeObject* DMSegment_Factory(const Dictionary*& Dict) { return new DMSegment(Dict); }
static InterchangeObject* CryptographicFramework_Factory(const Dictionary*& Dict) { return new CryptographicFramework(Dict); }
static InterchangeObject* CryptographicContext_Factory(const Dictionary*& Dict) { return new CryptographicContext(Dict); }
static InterchangeObject* GenericDataEssenceDescriptor_Factory(const Dictionary*& Dict) { return new GenericDataEssenceDescriptor(Dict); }
static InterchangeObject* TimedTextDescriptor_Factory(const Dictionary*& Dict) { return new TimedTextDescriptor(Dict); }
static InterchangeObject* TimedTextResourceSubDescriptor_Factory(const Dictionary*& Dict) { return new TimedTextResourceSubDescriptor(Dict); }
static InterchangeObject* StereoscopicPictureSubDescriptor_Factory(const Dictionary*& Dict) { return new StereoscopicPictureSubDescriptor(Dict); }
static InterchangeObject* ContainerConstraintSubDescriptor_Factory(const Dictionary*& Dict) { return new ContainerConstraintSubDescriptor(Dict); }
static InterchangeObject* NetworkLocator_Factory(const Dictionary*& Dict) { return new NetworkLocator(Dict); }
static InterchangeObject* MCALabelSubDescriptor_Factory(const Dictionary*& Dict) { return new MCALabelSubDescriptor(Dict); }
static InterchangeObject* AudioChannelLabelSubDescriptor_Factory(const Dictionary*& Dict) { return new AudioChannelLabelSubDescriptor(Dict); }
static InterchangeObject* SoundfieldGroupLabelSubDescriptor_Factory(const Dictionary*& Dict) { return new SoundfieldGroupLabelSubDescriptor(Dict); }
static InterchangeObject* GroupOfSoundfieldGroupsLabelSubDescriptor_Factory(const Dictionary*& Dict) { return new GroupOfSoundfieldGroupsLabelSubDescriptor(Dict); }
static InterchangeObject* DCDataDescriptor_Factory(const Dictionary*& Dict) { return new DCDataDescriptor(Dict); }
static InterchangeObject* PrivateDCDataDescriptor_Factory(const Dictionary*& Dict) { return new PrivateDCDataDescriptor(Dict); }
static InterchangeObject* DolbyAtmosSubDescriptor_Factory(const Dictionary*& Dict) { return new DolbyAtmosSubDescriptor(Dict); }
static InterchangeObject* PHDRMetadataTrackSubDescriptor_Factory(const Dictionary*& Dict) { return new PHDRMetadataTrackSubDescriptor(Dict); }
static InterchangeObject* PIMFDynamicMetadataDescriptor_Factory(const Dictionary*& Dict) { return new PIMFDynamicMetadataDescriptor(Dict); }


void
ASDCP::MXF::Metadata_InitTypes(const Dictionary*& Dict)
{
  assert(Dict);
  SetObjectFactory(Dict->ul(MDD_Preface), Preface_Factory);
  SetObjectFactory(Dict->ul(MDD_IndexTableSegment), IndexTableSegment_Factory);

  SetObjectFactory(Dict->ul(MDD_Identification), Identification_Factory);
  SetObjectFactory(Dict->ul(MDD_ContentStorage), ContentStorage_Factory);
  SetObjectFactory(Dict->ul(MDD_EssenceContainerData), EssenceContainerData_Factory);
  SetObjectFactory(Dict->ul(MDD_MaterialPackage), MaterialPackage_Factory);
  SetObjectFactory(Dict->ul(MDD_SourcePackage), SourcePackage_Factory);
  SetObjectFactory(Dict->ul(MDD_StaticTrack), StaticTrack_Factory);
  SetObjectFactory(Dict->ul(MDD_Track), Track_Factory);
  SetObjectFactory(Dict->ul(MDD_Sequence), Sequence_Factory);
  SetObjectFactory(Dict->ul(MDD_SourceClip), SourceClip_Factory);
  SetObjectFactory(Dict->ul(MDD_TimecodeComponent), TimecodeComponent_Factory);
  SetObjectFactory(Dict->ul(MDD_FileDescriptor), FileDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_GenericSoundEssenceDescriptor), GenericSoundEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_WaveAudioDescriptor), WaveAudioDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_GenericPictureEssenceDescriptor), GenericPictureEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_RGBAEssenceDescriptor), RGBAEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_JPEG2000PictureSubDescriptor), JPEG2000PictureSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_CDCIEssenceDescriptor), CDCIEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_MPEG2VideoDescriptor), MPEG2VideoDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_DMSegment), DMSegment_Factory);
  SetObjectFactory(Dict->ul(MDD_CryptographicFramework), CryptographicFramework_Factory);
  SetObjectFactory(Dict->ul(MDD_CryptographicContext), CryptographicContext_Factory);
  SetObjectFactory(Dict->ul(MDD_GenericDataEssenceDescriptor), GenericDataEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_TimedTextDescriptor), TimedTextDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_TimedTextResourceSubDescriptor), TimedTextResourceSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_StereoscopicPictureSubDescriptor), StereoscopicPictureSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_ContainerConstraintSubDescriptor), ContainerConstraintSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_NetworkLocator), NetworkLocator_Factory);
  SetObjectFactory(Dict->ul(MDD_MCALabelSubDescriptor), MCALabelSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_AudioChannelLabelSubDescriptor), AudioChannelLabelSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_SoundfieldGroupLabelSubDescriptor), SoundfieldGroupLabelSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_GroupOfSoundfieldGroupsLabelSubDescriptor), GroupOfSoundfieldGroupsLabelSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_DCDataDescriptor), DCDataDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_PrivateDCDataDescriptor), PrivateDCDataDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_DolbyAtmosSubDescriptor), DolbyAtmosSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_PHDRMetadataTrackSubDescriptor), PHDRMetadataTrackSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_PIMFDynamicMetadataDescriptor), PIMFDynamicMetadataDescriptor_Factory);
}

//------------------------------------------------------------------------------------------
// KLV Sets



//------------------------------------------------------------------------------------------
// Identification

//

Identification::Identification(const Dictionary*& d) : InterchangeObject(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_Identification);
}

Identification::Identification(const Identification& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_Identification);
  Copy(rhs);
}


//
ASDCP::Result_t
Identification::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ThisGenerationUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, CompanyName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ProductName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ProductVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, VersionString));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ProductUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ModificationDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ToolkitVersion));
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(Identification, Platform));
    Platform.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
Identification::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ThisGenerationUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, CompanyName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ProductName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ProductVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, VersionString));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ProductUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ModificationDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ToolkitVersion));
  if ( ASDCP_SUCCESS(result)  && ! Platform.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(Identification, Platform));
  return result;
}

//
void
Identification::Copy(const Identification& rhs)
{
  InterchangeObject::Copy(rhs);
  ThisGenerationUID = rhs.ThisGenerationUID;
  CompanyName = rhs.CompanyName;
  ProductName = rhs.ProductName;
  ProductVersion = rhs.ProductVersion;
  VersionString = rhs.VersionString;
  ProductUID = rhs.ProductUID;
  ModificationDate = rhs.ModificationDate;
  ToolkitVersion = rhs.ToolkitVersion;
  Platform = rhs.Platform;
}

//
void
Identification::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ThisGenerationUID", ThisGenerationUID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "CompanyName", CompanyName.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ProductName", ProductName.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ProductVersion", ProductVersion.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "VersionString", VersionString.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ProductUID", ProductUID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ModificationDate", ModificationDate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ToolkitVersion", ToolkitVersion.EncodeString(identbuf, IdentBufferLen));
  if ( ! Platform.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "Platform", Platform.get().EncodeString(identbuf, IdentBufferLen));
  }
}

//
ASDCP::Result_t
Identification::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
Identification::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// ContentStorage

//

ContentStorage::ContentStorage(const Dictionary*& d) : InterchangeObject(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_ContentStorage);
}

ContentStorage::ContentStorage(const ContentStorage& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_ContentStorage);
  Copy(rhs);
}


//
ASDCP::Result_t
ContentStorage::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(ContentStorage, Packages));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(ContentStorage, EssenceContainerData));
  return result;
}

//
ASDCP::Result_t
ContentStorage::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(ContentStorage, Packages));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(ContentStorage, EssenceContainerData));
  return result;
}

//
void
ContentStorage::Copy(const ContentStorage& rhs)
{
  InterchangeObject::Copy(rhs);
  Packages = rhs.Packages;
  EssenceContainerData = rhs.EssenceContainerData;
}

//
void
ContentStorage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s:\n",  "Packages");
  Packages.Dump(stream);
  fprintf(stream, "  %22s:\n",  "EssenceContainerData");
  EssenceContainerData.Dump(stream);
}

//
ASDCP::Result_t
ContentStorage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ContentStorage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// EssenceContainerData

//

EssenceContainerData::EssenceContainerData(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), BodySID(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_EssenceContainerData);
}

EssenceContainerData::EssenceContainerData(const EssenceContainerData& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_EssenceContainerData);
  Copy(rhs);
}


//
ASDCP::Result_t
EssenceContainerData::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(EssenceContainerData, LinkedPackageUID));
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(EssenceContainerData, IndexSID));
    IndexSID.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(EssenceContainerData, BodySID));
  return result;
}

//
ASDCP::Result_t
EssenceContainerData::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(EssenceContainerData, LinkedPackageUID));
  if ( ASDCP_SUCCESS(result)  && ! IndexSID.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(EssenceContainerData, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(EssenceContainerData, BodySID));
  return result;
}

//
void
EssenceContainerData::Copy(const EssenceContainerData& rhs)
{
  InterchangeObject::Copy(rhs);
  LinkedPackageUID = rhs.LinkedPackageUID;
  IndexSID = rhs.IndexSID;
  BodySID = rhs.BodySID;
}

//
void
EssenceContainerData::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "LinkedPackageUID", LinkedPackageUID.EncodeString(identbuf, IdentBufferLen));
  if ( ! IndexSID.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "IndexSID", IndexSID.get());
  }
  fprintf(stream, "  %22s = %d\n",  "BodySID", BodySID);
}

//
ASDCP::Result_t
EssenceContainerData::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
EssenceContainerData::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericPackage

//
GenericPackage::GenericPackage(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}

GenericPackage::GenericPackage(const GenericPackage& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  Copy(rhs);
}


//
ASDCP::Result_t
GenericPackage::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPackage, PackageUID));
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericPackage, Name));
    Name.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPackage, PackageCreationDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPackage, PackageModifiedDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPackage, Tracks));
  return result;
}

//
ASDCP::Result_t
GenericPackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPackage, PackageUID));
  if ( ASDCP_SUCCESS(result)  && ! Name.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericPackage, Name));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPackage, PackageCreationDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPackage, PackageModifiedDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPackage, Tracks));
  return result;
}

//
void
GenericPackage::Copy(const GenericPackage& rhs)
{
  InterchangeObject::Copy(rhs);
  PackageUID = rhs.PackageUID;
  Name = rhs.Name;
  PackageCreationDate = rhs.PackageCreationDate;
  PackageModifiedDate = rhs.PackageModifiedDate;
  Tracks = rhs.Tracks;
}

//
void
GenericPackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "PackageUID", PackageUID.EncodeString(identbuf, IdentBufferLen));
  if ( ! Name.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "Name", Name.get().EncodeString(identbuf, IdentBufferLen));
  }
  fprintf(stream, "  %22s = %s\n",  "PackageCreationDate", PackageCreationDate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "PackageModifiedDate", PackageModifiedDate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s:\n",  "Tracks");
  Tracks.Dump(stream);
}


//------------------------------------------------------------------------------------------
// MaterialPackage

//

MaterialPackage::MaterialPackage(const Dictionary*& d) : GenericPackage(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_MaterialPackage);
}

MaterialPackage::MaterialPackage(const MaterialPackage& rhs) : GenericPackage(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_MaterialPackage);
  Copy(rhs);
}


//
ASDCP::Result_t
MaterialPackage::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MaterialPackage, PackageMarker));
    PackageMarker.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
MaterialPackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPackage::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result)  && ! PackageMarker.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MaterialPackage, PackageMarker));
  return result;
}

//
void
MaterialPackage::Copy(const MaterialPackage& rhs)
{
  GenericPackage::Copy(rhs);
  PackageMarker = rhs.PackageMarker;
}

//
void
MaterialPackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPackage::Dump(stream);
  if ( ! PackageMarker.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "PackageMarker", PackageMarker.get().EncodeString(identbuf, IdentBufferLen));
  }
}

//
ASDCP::Result_t
MaterialPackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
MaterialPackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SourcePackage

//

SourcePackage::SourcePackage(const Dictionary*& d) : GenericPackage(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_SourcePackage);
}

SourcePackage::SourcePackage(const SourcePackage& rhs) : GenericPackage(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_SourcePackage);
  Copy(rhs);
}


//
ASDCP::Result_t
SourcePackage::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(SourcePackage, Descriptor));
  return result;
}

//
ASDCP::Result_t
SourcePackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPackage::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(SourcePackage, Descriptor));
  return result;
}

//
void
SourcePackage::Copy(const SourcePackage& rhs)
{
  GenericPackage::Copy(rhs);
  Descriptor = rhs.Descriptor;
}

//
void
SourcePackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPackage::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "Descriptor", Descriptor.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
SourcePackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
SourcePackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericTrack

//
GenericTrack::GenericTrack(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), TrackID(0), TrackNumber(0) {}

GenericTrack::GenericTrack(const GenericTrack& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  Copy(rhs);
}


//
ASDCP::Result_t
GenericTrack::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericTrack, TrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericTrack, TrackNumber));
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericTrack, TrackName));
    TrackName.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericTrack, Sequence));
    Sequence.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
GenericTrack::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericTrack, TrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericTrack, TrackNumber));
  if ( ASDCP_SUCCESS(result)  && ! TrackName.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericTrack, TrackName));
  if ( ASDCP_SUCCESS(result)  && ! Sequence.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericTrack, Sequence));
  return result;
}

//
void
GenericTrack::Copy(const GenericTrack& rhs)
{
  InterchangeObject::Copy(rhs);
  TrackID = rhs.TrackID;
  TrackNumber = rhs.TrackNumber;
  TrackName = rhs.TrackName;
  Sequence = rhs.Sequence;
}

//
void
GenericTrack::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "TrackID", TrackID);
  fprintf(stream, "  %22s = %d\n",  "TrackNumber", TrackNumber);
  if ( ! TrackName.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "TrackName", TrackName.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! Sequence.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "Sequence", Sequence.get().EncodeString(identbuf, IdentBufferLen));
  }
}


//------------------------------------------------------------------------------------------
// StaticTrack

//

StaticTrack::StaticTrack(const Dictionary*& d) : GenericTrack(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_StaticTrack);
}

StaticTrack::StaticTrack(const StaticTrack& rhs) : GenericTrack(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_StaticTrack);
  Copy(rhs);
}


//
ASDCP::Result_t
StaticTrack::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericTrack::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
StaticTrack::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericTrack::WriteToTLVSet(TLVSet);
  return result;
}

//
void
StaticTrack::Copy(const StaticTrack& rhs)
{
  GenericTrack::Copy(rhs);
}

//
void
StaticTrack::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericTrack::Dump(stream);
}

//
ASDCP::Result_t
StaticTrack::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
StaticTrack::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// Track

//

Track::Track(const Dictionary*& d) : GenericTrack(d), m_Dict(d), Origin(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_Track);
}

Track::Track(const Track& rhs) : GenericTrack(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_Track);
  Copy(rhs);
}


//
ASDCP::Result_t
Track::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericTrack::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Track, EditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(Track, Origin));
  return result;
}

//
ASDCP::Result_t
Track::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericTrack::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Track, EditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(Track, Origin));
  return result;
}

//
void
Track::Copy(const Track& rhs)
{
  GenericTrack::Copy(rhs);
  EditRate = rhs.EditRate;
  Origin = rhs.Origin;
}

//
void
Track::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericTrack::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "EditRate", EditRate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "Origin", i64sz(Origin, identbuf));
}

//
ASDCP::Result_t
Track::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
Track::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// StructuralComponent

//
StructuralComponent::StructuralComponent(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}

StructuralComponent::StructuralComponent(const StructuralComponent& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  Copy(rhs);
}


//
ASDCP::Result_t
StructuralComponent::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(StructuralComponent, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi64(OBJ_READ_ARGS_OPT(StructuralComponent, Duration));
    Duration.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
StructuralComponent::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(StructuralComponent, DataDefinition));
  if ( ASDCP_SUCCESS(result)  && ! Duration.empty() ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS_OPT(StructuralComponent, Duration));
  return result;
}

//
void
StructuralComponent::Copy(const StructuralComponent& rhs)
{
  InterchangeObject::Copy(rhs);
  DataDefinition = rhs.DataDefinition;
  Duration = rhs.Duration;
}

//
void
StructuralComponent::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "DataDefinition", DataDefinition.EncodeString(identbuf, IdentBufferLen));
  if ( ! Duration.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "Duration", i64sz(Duration.get(), identbuf));
  }
}


//------------------------------------------------------------------------------------------
// Sequence

//

Sequence::Sequence(const Dictionary*& d) : StructuralComponent(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_Sequence);
}

Sequence::Sequence(const Sequence& rhs) : StructuralComponent(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_Sequence);
  Copy(rhs);
}


//
ASDCP::Result_t
Sequence::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Sequence, StructuralComponents));
  return result;
}

//
ASDCP::Result_t
Sequence::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Sequence, StructuralComponents));
  return result;
}

//
void
Sequence::Copy(const Sequence& rhs)
{
  StructuralComponent::Copy(rhs);
  StructuralComponents = rhs.StructuralComponents;
}

//
void
Sequence::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fprintf(stream, "  %22s:\n",  "StructuralComponents");
  StructuralComponents.Dump(stream);
}

//
ASDCP::Result_t
Sequence::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
Sequence::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SourceClip

//

SourceClip::SourceClip(const Dictionary*& d) : StructuralComponent(d), m_Dict(d), StartPosition(0), SourceTrackID(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_SourceClip);
}

SourceClip::SourceClip(const SourceClip& rhs) : StructuralComponent(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_SourceClip);
  Copy(rhs);
}


//
ASDCP::Result_t
SourceClip::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(SourceClip, StartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(SourceClip, SourcePackageID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(SourceClip, SourceTrackID));
  return result;
}

//
ASDCP::Result_t
SourceClip::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(SourceClip, StartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(SourceClip, SourcePackageID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(SourceClip, SourceTrackID));
  return result;
}

//
void
SourceClip::Copy(const SourceClip& rhs)
{
  StructuralComponent::Copy(rhs);
  StartPosition = rhs.StartPosition;
  SourcePackageID = rhs.SourcePackageID;
  SourceTrackID = rhs.SourceTrackID;
}

//
void
SourceClip::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "StartPosition", i64sz(StartPosition, identbuf));
  fprintf(stream, "  %22s = %s\n",  "SourcePackageID", SourcePackageID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "SourceTrackID", SourceTrackID);
}

//
ASDCP::Result_t
SourceClip::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
SourceClip::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimecodeComponent

//

TimecodeComponent::TimecodeComponent(const Dictionary*& d) : StructuralComponent(d), m_Dict(d), RoundedTimecodeBase(0), StartTimecode(0), DropFrame(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_TimecodeComponent);
}

TimecodeComponent::TimecodeComponent(const TimecodeComponent& rhs) : StructuralComponent(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_TimecodeComponent);
  Copy(rhs);
}


//
ASDCP::Result_t
TimecodeComponent::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(TimecodeComponent, RoundedTimecodeBase));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(TimecodeComponent, StartTimecode));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(TimecodeComponent, DropFrame));
  return result;
}

//
ASDCP::Result_t
TimecodeComponent::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(TimecodeComponent, RoundedTimecodeBase));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(TimecodeComponent, StartTimecode));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(TimecodeComponent, DropFrame));
  return result;
}

//
void
TimecodeComponent::Copy(const TimecodeComponent& rhs)
{
  StructuralComponent::Copy(rhs);
  RoundedTimecodeBase = rhs.RoundedTimecodeBase;
  StartTimecode = rhs.StartTimecode;
  DropFrame = rhs.DropFrame;
}

//
void
TimecodeComponent::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "RoundedTimecodeBase", RoundedTimecodeBase);
  fprintf(stream, "  %22s = %s\n",  "StartTimecode", i64sz(StartTimecode, identbuf));
  fprintf(stream, "  %22s = %d\n",  "DropFrame", DropFrame);
}

//
ASDCP::Result_t
TimecodeComponent::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
TimecodeComponent::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericDescriptor

//
GenericDescriptor::GenericDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}

GenericDescriptor::GenericDescriptor(const GenericDescriptor& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  Copy(rhs);
}


//
ASDCP::Result_t
GenericDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, Locators));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, SubDescriptors));
  return result;
}

//
ASDCP::Result_t
GenericDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDescriptor, Locators));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDescriptor, SubDescriptors));
  return result;
}

//
void
GenericDescriptor::Copy(const GenericDescriptor& rhs)
{
  InterchangeObject::Copy(rhs);
  Locators = rhs.Locators;
  SubDescriptors = rhs.SubDescriptors;
}

//
void
GenericDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s:\n",  "Locators");
  Locators.Dump(stream);
  fprintf(stream, "  %22s:\n",  "SubDescriptors");
  SubDescriptors.Dump(stream);
}


//------------------------------------------------------------------------------------------
// FileDescriptor

//

FileDescriptor::FileDescriptor(const Dictionary*& d) : GenericDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_FileDescriptor);
}

FileDescriptor::FileDescriptor(const FileDescriptor& rhs) : GenericDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_FileDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
FileDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(FileDescriptor, LinkedTrackID));
    LinkedTrackID.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(FileDescriptor, SampleRate));
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi64(OBJ_READ_ARGS_OPT(FileDescriptor, ContainerDuration));
    ContainerDuration.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(FileDescriptor, EssenceContainer));
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(FileDescriptor, Codec));
    Codec.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
FileDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result)  && ! LinkedTrackID.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(FileDescriptor, LinkedTrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(FileDescriptor, SampleRate));
  if ( ASDCP_SUCCESS(result)  && ! ContainerDuration.empty() ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS_OPT(FileDescriptor, ContainerDuration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(FileDescriptor, EssenceContainer));
  if ( ASDCP_SUCCESS(result)  && ! Codec.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(FileDescriptor, Codec));
  return result;
}

//
void
FileDescriptor::Copy(const FileDescriptor& rhs)
{
  GenericDescriptor::Copy(rhs);
  LinkedTrackID = rhs.LinkedTrackID;
  SampleRate = rhs.SampleRate;
  ContainerDuration = rhs.ContainerDuration;
  EssenceContainer = rhs.EssenceContainer;
  Codec = rhs.Codec;
}

//
void
FileDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericDescriptor::Dump(stream);
  if ( ! LinkedTrackID.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "LinkedTrackID", LinkedTrackID.get());
  }
  fprintf(stream, "  %22s = %s\n",  "SampleRate", SampleRate.EncodeString(identbuf, IdentBufferLen));
  if ( ! ContainerDuration.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "ContainerDuration", i64sz(ContainerDuration.get(), identbuf));
  }
  fprintf(stream, "  %22s = %s\n",  "EssenceContainer", EssenceContainer.EncodeString(identbuf, IdentBufferLen));
  if ( ! Codec.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "Codec", Codec.get().EncodeString(identbuf, IdentBufferLen));
  }
}

//
ASDCP::Result_t
FileDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
FileDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericSoundEssenceDescriptor

//

GenericSoundEssenceDescriptor::GenericSoundEssenceDescriptor(const Dictionary*& d) : FileDescriptor(d), m_Dict(d), Locked(0), ChannelCount(0), QuantizationBits(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_GenericSoundEssenceDescriptor);
}

GenericSoundEssenceDescriptor::GenericSoundEssenceDescriptor(const GenericSoundEssenceDescriptor& rhs) : FileDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_GenericSoundEssenceDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
GenericSoundEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, AudioSamplingRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, Locked));
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(GenericSoundEssenceDescriptor, AudioRefLevel));
    AudioRefLevel.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(GenericSoundEssenceDescriptor, ElectroSpatialFormulation));
    ElectroSpatialFormulation.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, ChannelCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, QuantizationBits));
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(GenericSoundEssenceDescriptor, DialNorm));
    DialNorm.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, SoundEssenceCoding));
  return result;
}

//
ASDCP::Result_t
GenericSoundEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, AudioSamplingRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, Locked));
  if ( ASDCP_SUCCESS(result)  && ! AudioRefLevel.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(GenericSoundEssenceDescriptor, AudioRefLevel));
  if ( ASDCP_SUCCESS(result)  && ! ElectroSpatialFormulation.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(GenericSoundEssenceDescriptor, ElectroSpatialFormulation));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, ChannelCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, QuantizationBits));
  if ( ASDCP_SUCCESS(result)  && ! DialNorm.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(GenericSoundEssenceDescriptor, DialNorm));
  if ( ASDCP_SUCCESS(result) && SoundEssenceCoding.HasValue()) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, SoundEssenceCoding));
  return result;
}

//
void
GenericSoundEssenceDescriptor::Copy(const GenericSoundEssenceDescriptor& rhs)
{
  FileDescriptor::Copy(rhs);
  AudioSamplingRate = rhs.AudioSamplingRate;
  Locked = rhs.Locked;
  AudioRefLevel = rhs.AudioRefLevel;
  ElectroSpatialFormulation = rhs.ElectroSpatialFormulation;
  ChannelCount = rhs.ChannelCount;
  QuantizationBits = rhs.QuantizationBits;
  DialNorm = rhs.DialNorm;
  SoundEssenceCoding = rhs.SoundEssenceCoding;
}

//
void
GenericSoundEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "AudioSamplingRate", AudioSamplingRate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "Locked", Locked);
  if ( ! AudioRefLevel.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "AudioRefLevel", AudioRefLevel.get());
  }
  if ( ! ElectroSpatialFormulation.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ElectroSpatialFormulation", ElectroSpatialFormulation.get());
  }
  fprintf(stream, "  %22s = %d\n",  "ChannelCount", ChannelCount);
  fprintf(stream, "  %22s = %d\n",  "QuantizationBits", QuantizationBits);
  if ( ! DialNorm.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "DialNorm", DialNorm.get());
  }
  fprintf(stream, "  %22s = %s\n",  "SoundEssenceCoding", SoundEssenceCoding.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
GenericSoundEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GenericSoundEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// WaveAudioDescriptor

//

WaveAudioDescriptor::WaveAudioDescriptor(const Dictionary*& d) : GenericSoundEssenceDescriptor(d), m_Dict(d), BlockAlign(0), AvgBps(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_WaveAudioDescriptor);
}

WaveAudioDescriptor::WaveAudioDescriptor(const WaveAudioDescriptor& rhs) : GenericSoundEssenceDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_WaveAudioDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
WaveAudioDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericSoundEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(WaveAudioDescriptor, BlockAlign));
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(WaveAudioDescriptor, SequenceOffset));
    SequenceOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(WaveAudioDescriptor, AvgBps));
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(WaveAudioDescriptor, ChannelAssignment));
    ChannelAssignment.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(WaveAudioDescriptor, ReferenceImageEditRate));
    ReferenceImageEditRate.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(WaveAudioDescriptor, ReferenceAudioAlignmentLevel));
    ReferenceAudioAlignmentLevel.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
WaveAudioDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericSoundEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(WaveAudioDescriptor, BlockAlign));
  if ( ASDCP_SUCCESS(result)  && ! SequenceOffset.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(WaveAudioDescriptor, SequenceOffset));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(WaveAudioDescriptor, AvgBps));
  if ( ASDCP_SUCCESS(result)  && ! ChannelAssignment.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(WaveAudioDescriptor, ChannelAssignment));
  if ( ASDCP_SUCCESS(result)  && ! ReferenceImageEditRate.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(WaveAudioDescriptor, ReferenceImageEditRate));
  if ( ASDCP_SUCCESS(result)  && ! ReferenceAudioAlignmentLevel.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(WaveAudioDescriptor, ReferenceAudioAlignmentLevel));
  return result;
}

//
void
WaveAudioDescriptor::Copy(const WaveAudioDescriptor& rhs)
{
  GenericSoundEssenceDescriptor::Copy(rhs);
  BlockAlign = rhs.BlockAlign;
  SequenceOffset = rhs.SequenceOffset;
  AvgBps = rhs.AvgBps;
  ChannelAssignment = rhs.ChannelAssignment;
  ReferenceImageEditRate = rhs.ReferenceImageEditRate;
  ReferenceAudioAlignmentLevel = rhs.ReferenceAudioAlignmentLevel;
}

//
void
WaveAudioDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericSoundEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "BlockAlign", BlockAlign);
  if ( ! SequenceOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "SequenceOffset", SequenceOffset.get());
  }
  fprintf(stream, "  %22s = %d\n",  "AvgBps", AvgBps);
  if ( ! ChannelAssignment.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "ChannelAssignment", ChannelAssignment.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! ReferenceImageEditRate.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "ReferenceImageEditRate", ReferenceImageEditRate.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! ReferenceAudioAlignmentLevel.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ReferenceAudioAlignmentLevel", ReferenceAudioAlignmentLevel.get());
  }
}

//
ASDCP::Result_t
WaveAudioDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
WaveAudioDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericPictureEssenceDescriptor

//

GenericPictureEssenceDescriptor::GenericPictureEssenceDescriptor(const Dictionary*& d) : FileDescriptor(d), m_Dict(d), FrameLayout(0), StoredWidth(0), StoredHeight(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_GenericPictureEssenceDescriptor);
}

GenericPictureEssenceDescriptor::GenericPictureEssenceDescriptor(const GenericPictureEssenceDescriptor& rhs) : FileDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_GenericPictureEssenceDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
GenericPictureEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, SignalStandard));
    SignalStandard.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, FrameLayout));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, StoredWidth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, StoredHeight));
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, StoredF2Offset));
    StoredF2Offset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, SampledWidth));
    SampledWidth.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, SampledHeight));
    SampledHeight.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, SampledXOffset));
    SampledXOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, SampledYOffset));
    SampledYOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayHeight));
    DisplayHeight.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayWidth));
    DisplayWidth.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayXOffset));
    DisplayXOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayYOffset));
    DisplayYOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayF2Offset));
    DisplayF2Offset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, AspectRatio));
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveFormatDescriptor));
    ActiveFormatDescriptor.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, AlphaTransparency));
    AlphaTransparency.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, TransferCharacteristic));
    TransferCharacteristic.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, ImageAlignmentOffset));
    ImageAlignmentOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, ImageStartOffset));
    ImageStartOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, ImageEndOffset));
    ImageEndOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, FieldDominance));
    FieldDominance.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, PictureEssenceCoding));
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, CodingEquations));
    CodingEquations.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, ColorPrimaries));
    ColorPrimaries.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, AlternativeCenterCuts));
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveWidth));
    ActiveWidth.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveHeight));
    ActiveHeight.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveXOffset));
    ActiveXOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveYOffset));
    ActiveYOffset.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, VideoLineMap));
    VideoLineMap.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, MasteringDisplayPrimaries));
    MasteringDisplayPrimaries.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, MasteringDisplayWhitePointChromaticity));
    MasteringDisplayWhitePointChromaticity.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, MasteringDisplayMaximumLuminance));
    MasteringDisplayMaximumLuminance.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(GenericPictureEssenceDescriptor, MasteringDisplayMinimumLuminance));
    MasteringDisplayMinimumLuminance.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
GenericPictureEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result)  && ! SignalStandard.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, SignalStandard));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, FrameLayout));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, StoredWidth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, StoredHeight));
  if ( ASDCP_SUCCESS(result)  && ! StoredF2Offset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, StoredF2Offset));
  if ( ASDCP_SUCCESS(result)  && ! SampledWidth.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, SampledWidth));
  if ( ASDCP_SUCCESS(result)  && ! SampledHeight.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, SampledHeight));
  if ( ASDCP_SUCCESS(result)  && ! SampledXOffset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, SampledXOffset));
  if ( ASDCP_SUCCESS(result)  && ! SampledYOffset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, SampledYOffset));
  if ( ASDCP_SUCCESS(result)  && ! DisplayHeight.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayHeight));
  if ( ASDCP_SUCCESS(result)  && ! DisplayWidth.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayWidth));
  if ( ASDCP_SUCCESS(result)  && ! DisplayXOffset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayXOffset));
  if ( ASDCP_SUCCESS(result)  && ! DisplayYOffset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayYOffset));
  if ( ASDCP_SUCCESS(result)  && ! DisplayF2Offset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, DisplayF2Offset));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, AspectRatio));
  if ( ASDCP_SUCCESS(result)  && ! ActiveFormatDescriptor.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveFormatDescriptor));
  if ( ASDCP_SUCCESS(result)  && ! AlphaTransparency.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, AlphaTransparency));
  if ( ASDCP_SUCCESS(result)  && ! TransferCharacteristic.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, TransferCharacteristic));
  if ( ASDCP_SUCCESS(result)  && ! ImageAlignmentOffset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, ImageAlignmentOffset));
  if ( ASDCP_SUCCESS(result)  && ! ImageStartOffset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, ImageStartOffset));
  if ( ASDCP_SUCCESS(result)  && ! ImageEndOffset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, ImageEndOffset));
  if ( ASDCP_SUCCESS(result)  && ! FieldDominance.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, FieldDominance));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, PictureEssenceCoding));
  if ( ASDCP_SUCCESS(result)  && ! CodingEquations.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, CodingEquations));
  if ( ASDCP_SUCCESS(result)  && ! ColorPrimaries.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, ColorPrimaries));
  if ( ASDCP_SUCCESS(result)  && ! AlternativeCenterCuts.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, AlternativeCenterCuts));
  if ( ASDCP_SUCCESS(result)  && ! ActiveWidth.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveWidth));
  if ( ASDCP_SUCCESS(result)  && ! ActiveHeight.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveHeight));
  if ( ASDCP_SUCCESS(result)  && ! ActiveXOffset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveXOffset));
  if ( ASDCP_SUCCESS(result)  && ! ActiveYOffset.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, ActiveYOffset));
  if ( ASDCP_SUCCESS(result)  && ! VideoLineMap.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, VideoLineMap));
  if ( ASDCP_SUCCESS(result)  && ! MasteringDisplayPrimaries.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, MasteringDisplayPrimaries));
  if ( ASDCP_SUCCESS(result)  && ! MasteringDisplayWhitePointChromaticity.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, MasteringDisplayWhitePointChromaticity));
  if ( ASDCP_SUCCESS(result)  && ! MasteringDisplayMaximumLuminance.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, MasteringDisplayMaximumLuminance));
  if ( ASDCP_SUCCESS(result)  && ! MasteringDisplayMinimumLuminance.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(GenericPictureEssenceDescriptor, MasteringDisplayMinimumLuminance));
  return result;
}

//
void
GenericPictureEssenceDescriptor::Copy(const GenericPictureEssenceDescriptor& rhs)
{
  FileDescriptor::Copy(rhs);
  SignalStandard = rhs.SignalStandard;
  FrameLayout = rhs.FrameLayout;
  StoredWidth = rhs.StoredWidth;
  StoredHeight = rhs.StoredHeight;
  StoredF2Offset = rhs.StoredF2Offset;
  SampledWidth = rhs.SampledWidth;
  SampledHeight = rhs.SampledHeight;
  SampledXOffset = rhs.SampledXOffset;
  SampledYOffset = rhs.SampledYOffset;
  DisplayHeight = rhs.DisplayHeight;
  DisplayWidth = rhs.DisplayWidth;
  DisplayXOffset = rhs.DisplayXOffset;
  DisplayYOffset = rhs.DisplayYOffset;
  DisplayF2Offset = rhs.DisplayF2Offset;
  AspectRatio = rhs.AspectRatio;
  ActiveFormatDescriptor = rhs.ActiveFormatDescriptor;
  AlphaTransparency = rhs.AlphaTransparency;
  TransferCharacteristic = rhs.TransferCharacteristic;
  ImageAlignmentOffset = rhs.ImageAlignmentOffset;
  ImageStartOffset = rhs.ImageStartOffset;
  ImageEndOffset = rhs.ImageEndOffset;
  FieldDominance = rhs.FieldDominance;
  PictureEssenceCoding = rhs.PictureEssenceCoding;
  CodingEquations = rhs.CodingEquations;
  ColorPrimaries = rhs.ColorPrimaries;
  AlternativeCenterCuts = rhs.AlternativeCenterCuts;
  ActiveWidth = rhs.ActiveWidth;
  ActiveHeight = rhs.ActiveHeight;
  ActiveXOffset = rhs.ActiveXOffset;
  ActiveYOffset = rhs.ActiveYOffset;
  VideoLineMap = rhs.VideoLineMap;
  MasteringDisplayPrimaries = rhs.MasteringDisplayPrimaries;
  MasteringDisplayWhitePointChromaticity = rhs.MasteringDisplayWhitePointChromaticity;
  MasteringDisplayMaximumLuminance = rhs.MasteringDisplayMaximumLuminance;
  MasteringDisplayMinimumLuminance = rhs.MasteringDisplayMinimumLuminance;
}

//
void
GenericPictureEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  if ( ! SignalStandard.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "SignalStandard", SignalStandard.get());
  }
  fprintf(stream, "  %22s = %d\n",  "FrameLayout", FrameLayout);
  fprintf(stream, "  %22s = %d\n",  "StoredWidth", StoredWidth);
  fprintf(stream, "  %22s = %d\n",  "StoredHeight", StoredHeight);
  if ( ! StoredF2Offset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "StoredF2Offset", StoredF2Offset.get());
  }
  if ( ! SampledWidth.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "SampledWidth", SampledWidth.get());
  }
  if ( ! SampledHeight.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "SampledHeight", SampledHeight.get());
  }
  if ( ! SampledXOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "SampledXOffset", SampledXOffset.get());
  }
  if ( ! SampledYOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "SampledYOffset", SampledYOffset.get());
  }
  if ( ! DisplayHeight.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "DisplayHeight", DisplayHeight.get());
  }
  if ( ! DisplayWidth.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "DisplayWidth", DisplayWidth.get());
  }
  if ( ! DisplayXOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "DisplayXOffset", DisplayXOffset.get());
  }
  if ( ! DisplayYOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "DisplayYOffset", DisplayYOffset.get());
  }
  if ( ! DisplayF2Offset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "DisplayF2Offset", DisplayF2Offset.get());
  }
  fprintf(stream, "  %22s = %s\n",  "AspectRatio", AspectRatio.EncodeString(identbuf, IdentBufferLen));
  if ( ! ActiveFormatDescriptor.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ActiveFormatDescriptor", ActiveFormatDescriptor.get());
  }
  if ( ! AlphaTransparency.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "AlphaTransparency", AlphaTransparency.get());
  }
  if ( ! TransferCharacteristic.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "TransferCharacteristic", TransferCharacteristic.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! ImageAlignmentOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ImageAlignmentOffset", ImageAlignmentOffset.get());
  }
  if ( ! ImageStartOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ImageStartOffset", ImageStartOffset.get());
  }
  if ( ! ImageEndOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ImageEndOffset", ImageEndOffset.get());
  }
  if ( ! FieldDominance.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "FieldDominance", FieldDominance.get());
  }
  fprintf(stream, "  %22s = %s\n",  "PictureEssenceCoding", PictureEssenceCoding.EncodeString(identbuf, IdentBufferLen));
  if ( ! CodingEquations.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "CodingEquations", CodingEquations.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! ColorPrimaries.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "ColorPrimaries", ColorPrimaries.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! AlternativeCenterCuts.empty() ) {
    fprintf(stream, "  %22s:\n",  "AlternativeCenterCuts");
  AlternativeCenterCuts.get().Dump(stream);
  }
  if ( ! ActiveWidth.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ActiveWidth", ActiveWidth.get());
  }
  if ( ! ActiveHeight.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ActiveHeight", ActiveHeight.get());
  }
  if ( ! ActiveXOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ActiveXOffset", ActiveXOffset.get());
  }
  if ( ! ActiveYOffset.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ActiveYOffset", ActiveYOffset.get());
  }
  if ( ! VideoLineMap.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "VideoLineMap", VideoLineMap.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MasteringDisplayPrimaries.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MasteringDisplayPrimaries", MasteringDisplayPrimaries.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MasteringDisplayWhitePointChromaticity.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MasteringDisplayWhitePointChromaticity", MasteringDisplayWhitePointChromaticity.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MasteringDisplayMaximumLuminance.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "MasteringDisplayMaximumLuminance", MasteringDisplayMaximumLuminance.get());
  }
  if ( ! MasteringDisplayMinimumLuminance.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "MasteringDisplayMinimumLuminance", MasteringDisplayMinimumLuminance.get());
  }
}

//
ASDCP::Result_t
GenericPictureEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GenericPictureEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// RGBAEssenceDescriptor

//

RGBAEssenceDescriptor::RGBAEssenceDescriptor(const Dictionary*& d) : GenericPictureEssenceDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_RGBAEssenceDescriptor);
}

RGBAEssenceDescriptor::RGBAEssenceDescriptor(const RGBAEssenceDescriptor& rhs) : GenericPictureEssenceDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_RGBAEssenceDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
RGBAEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPictureEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(RGBAEssenceDescriptor, ComponentMaxRef));
    ComponentMaxRef.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(RGBAEssenceDescriptor, ComponentMinRef));
    ComponentMinRef.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(RGBAEssenceDescriptor, AlphaMinRef));
    AlphaMinRef.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(RGBAEssenceDescriptor, AlphaMaxRef));
    AlphaMaxRef.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(RGBAEssenceDescriptor, ScanningDirection));
    ScanningDirection.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(RGBAEssenceDescriptor, PixelLayout));
  return result;
}

//
ASDCP::Result_t
RGBAEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPictureEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result)  && ! ComponentMaxRef.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(RGBAEssenceDescriptor, ComponentMaxRef));
  if ( ASDCP_SUCCESS(result)  && ! ComponentMinRef.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(RGBAEssenceDescriptor, ComponentMinRef));
  if ( ASDCP_SUCCESS(result)  && ! AlphaMinRef.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(RGBAEssenceDescriptor, AlphaMinRef));
  if ( ASDCP_SUCCESS(result)  && ! AlphaMaxRef.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(RGBAEssenceDescriptor, AlphaMaxRef));
  if ( ASDCP_SUCCESS(result)  && ! ScanningDirection.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(RGBAEssenceDescriptor, ScanningDirection));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(RGBAEssenceDescriptor, PixelLayout));
  return result;
}

//
void
RGBAEssenceDescriptor::Copy(const RGBAEssenceDescriptor& rhs)
{
  GenericPictureEssenceDescriptor::Copy(rhs);
  ComponentMaxRef = rhs.ComponentMaxRef;
  ComponentMinRef = rhs.ComponentMinRef;
  AlphaMinRef = rhs.AlphaMinRef;
  AlphaMaxRef = rhs.AlphaMaxRef;
  ScanningDirection = rhs.ScanningDirection;
  PixelLayout = rhs.PixelLayout;
}

//
void
RGBAEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPictureEssenceDescriptor::Dump(stream);
  if ( ! ComponentMaxRef.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ComponentMaxRef", ComponentMaxRef.get());
  }
  if ( ! ComponentMinRef.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ComponentMinRef", ComponentMinRef.get());
  }
  if ( ! AlphaMinRef.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "AlphaMinRef", AlphaMinRef.get());
  }
  if ( ! AlphaMaxRef.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "AlphaMaxRef", AlphaMaxRef.get());
  }
  if ( ! ScanningDirection.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ScanningDirection", ScanningDirection.get());
  }
  fprintf(stream, "  %22s = %s\n",  "PixelLayout", PixelLayout.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
RGBAEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
RGBAEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// JPEG2000PictureSubDescriptor

//

JPEG2000PictureSubDescriptor::JPEG2000PictureSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), Rsize(0), Xsize(0), Ysize(0), XOsize(0), YOsize(0), XTsize(0), YTsize(0), XTOsize(0), YTOsize(0), Csize(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_JPEG2000PictureSubDescriptor);
}

JPEG2000PictureSubDescriptor::JPEG2000PictureSubDescriptor(const JPEG2000PictureSubDescriptor& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_JPEG2000PictureSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, Rsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, Xsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, Ysize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, XOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, YOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, XTsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, YTsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, XTOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, YTOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, Csize));
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(JPEG2000PictureSubDescriptor, PictureComponentSizing));
    PictureComponentSizing.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(JPEG2000PictureSubDescriptor, CodingStyleDefault));
    CodingStyleDefault.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(JPEG2000PictureSubDescriptor, QuantizationDefault));
    QuantizationDefault.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(JPEG2000PictureSubDescriptor, J2CLayout));
    J2CLayout.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, Rsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, Xsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, Ysize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, XOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, YOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, XTsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, YTsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, XTOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, YTOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, Csize));
  if ( ASDCP_SUCCESS(result)  && ! PictureComponentSizing.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(JPEG2000PictureSubDescriptor, PictureComponentSizing));
  if ( ASDCP_SUCCESS(result)  && ! CodingStyleDefault.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(JPEG2000PictureSubDescriptor, CodingStyleDefault));
  if ( ASDCP_SUCCESS(result)  && ! QuantizationDefault.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(JPEG2000PictureSubDescriptor, QuantizationDefault));
  if ( ASDCP_SUCCESS(result)  && ! J2CLayout.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(JPEG2000PictureSubDescriptor, J2CLayout));
  return result;
}

//
void
JPEG2000PictureSubDescriptor::Copy(const JPEG2000PictureSubDescriptor& rhs)
{
  InterchangeObject::Copy(rhs);
  Rsize = rhs.Rsize;
  Xsize = rhs.Xsize;
  Ysize = rhs.Ysize;
  XOsize = rhs.XOsize;
  YOsize = rhs.YOsize;
  XTsize = rhs.XTsize;
  YTsize = rhs.YTsize;
  XTOsize = rhs.XTOsize;
  YTOsize = rhs.YTOsize;
  Csize = rhs.Csize;
  PictureComponentSizing = rhs.PictureComponentSizing;
  CodingStyleDefault = rhs.CodingStyleDefault;
  QuantizationDefault = rhs.QuantizationDefault;
  J2CLayout = rhs.J2CLayout;
}

//
void
JPEG2000PictureSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "Rsize", Rsize);
  fprintf(stream, "  %22s = %d\n",  "Xsize", Xsize);
  fprintf(stream, "  %22s = %d\n",  "Ysize", Ysize);
  fprintf(stream, "  %22s = %d\n",  "XOsize", XOsize);
  fprintf(stream, "  %22s = %d\n",  "YOsize", YOsize);
  fprintf(stream, "  %22s = %d\n",  "XTsize", XTsize);
  fprintf(stream, "  %22s = %d\n",  "YTsize", YTsize);
  fprintf(stream, "  %22s = %d\n",  "XTOsize", XTOsize);
  fprintf(stream, "  %22s = %d\n",  "YTOsize", YTOsize);
  fprintf(stream, "  %22s = %d\n",  "Csize", Csize);
  if ( ! PictureComponentSizing.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "PictureComponentSizing", PictureComponentSizing.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! CodingStyleDefault.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "CodingStyleDefault", CodingStyleDefault.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! QuantizationDefault.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "QuantizationDefault", QuantizationDefault.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! J2CLayout.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "J2CLayout", J2CLayout.get().EncodeString(identbuf, IdentBufferLen));
  }
}

//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CDCIEssenceDescriptor

//

CDCIEssenceDescriptor::CDCIEssenceDescriptor(const Dictionary*& d) : GenericPictureEssenceDescriptor(d), m_Dict(d), ComponentDepth(0), HorizontalSubsampling(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_CDCIEssenceDescriptor);
}

CDCIEssenceDescriptor::CDCIEssenceDescriptor(const CDCIEssenceDescriptor& rhs) : GenericPictureEssenceDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_CDCIEssenceDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
CDCIEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPictureEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, ComponentDepth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, HorizontalSubsampling));
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(CDCIEssenceDescriptor, VerticalSubsampling));
    VerticalSubsampling.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(CDCIEssenceDescriptor, ColorSiting));
    ColorSiting.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(CDCIEssenceDescriptor, ReversedByteOrder));
    ReversedByteOrder.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi16(OBJ_READ_ARGS_OPT(CDCIEssenceDescriptor, PaddingBits));
    PaddingBits.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(CDCIEssenceDescriptor, AlphaSampleDepth));
    AlphaSampleDepth.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(CDCIEssenceDescriptor, BlackRefLevel));
    BlackRefLevel.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(CDCIEssenceDescriptor, WhiteReflevel));
    WhiteReflevel.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(CDCIEssenceDescriptor, ColorRange));
    ColorRange.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
CDCIEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPictureEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, ComponentDepth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, HorizontalSubsampling));
  if ( ASDCP_SUCCESS(result)  && ! VerticalSubsampling.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(CDCIEssenceDescriptor, VerticalSubsampling));
  if ( ASDCP_SUCCESS(result)  && ! ColorSiting.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(CDCIEssenceDescriptor, ColorSiting));
  if ( ASDCP_SUCCESS(result)  && ! ReversedByteOrder.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(CDCIEssenceDescriptor, ReversedByteOrder));
  if ( ASDCP_SUCCESS(result)  && ! PaddingBits.empty() ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS_OPT(CDCIEssenceDescriptor, PaddingBits));
  if ( ASDCP_SUCCESS(result)  && ! AlphaSampleDepth.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(CDCIEssenceDescriptor, AlphaSampleDepth));
  if ( ASDCP_SUCCESS(result)  && ! BlackRefLevel.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(CDCIEssenceDescriptor, BlackRefLevel));
  if ( ASDCP_SUCCESS(result)  && ! WhiteReflevel.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(CDCIEssenceDescriptor, WhiteReflevel));
  if ( ASDCP_SUCCESS(result)  && ! ColorRange.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(CDCIEssenceDescriptor, ColorRange));
  return result;
}

//
void
CDCIEssenceDescriptor::Copy(const CDCIEssenceDescriptor& rhs)
{
  GenericPictureEssenceDescriptor::Copy(rhs);
  ComponentDepth = rhs.ComponentDepth;
  HorizontalSubsampling = rhs.HorizontalSubsampling;
  VerticalSubsampling = rhs.VerticalSubsampling;
  ColorSiting = rhs.ColorSiting;
  ReversedByteOrder = rhs.ReversedByteOrder;
  PaddingBits = rhs.PaddingBits;
  AlphaSampleDepth = rhs.AlphaSampleDepth;
  BlackRefLevel = rhs.BlackRefLevel;
  WhiteReflevel = rhs.WhiteReflevel;
  ColorRange = rhs.ColorRange;
}

//
void
CDCIEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPictureEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "ComponentDepth", ComponentDepth);
  fprintf(stream, "  %22s = %d\n",  "HorizontalSubsampling", HorizontalSubsampling);
  if ( ! VerticalSubsampling.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "VerticalSubsampling", VerticalSubsampling.get());
  }
  if ( ! ColorSiting.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ColorSiting", ColorSiting.get());
  }
  if ( ! ReversedByteOrder.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ReversedByteOrder", ReversedByteOrder.get());
  }
  if ( ! PaddingBits.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "PaddingBits", PaddingBits.get());
  }
  if ( ! AlphaSampleDepth.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "AlphaSampleDepth", AlphaSampleDepth.get());
  }
  if ( ! BlackRefLevel.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "BlackRefLevel", BlackRefLevel.get());
  }
  if ( ! WhiteReflevel.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "WhiteReflevel", WhiteReflevel.get());
  }
  if ( ! ColorRange.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ColorRange", ColorRange.get());
  }
}

//
ASDCP::Result_t
CDCIEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
CDCIEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// MPEG2VideoDescriptor

//

MPEG2VideoDescriptor::MPEG2VideoDescriptor(const Dictionary*& d) : CDCIEssenceDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_MPEG2VideoDescriptor);
}

MPEG2VideoDescriptor::MPEG2VideoDescriptor(const MPEG2VideoDescriptor& rhs) : CDCIEssenceDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_MPEG2VideoDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
MPEG2VideoDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = CDCIEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, SingleSequence));
    SingleSequence.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, ConstantBFrames));
    ConstantBFrames.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, CodedContentType));
    CodedContentType.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, LowDelay));
    LowDelay.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, ClosedGOP));
    ClosedGOP.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, IdenticalGOP));
    IdenticalGOP.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, MaxGOP));
    MaxGOP.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, BPictureCount));
    BPictureCount.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, BitRate));
    BitRate.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi8(OBJ_READ_ARGS_OPT(MPEG2VideoDescriptor, ProfileAndLevel));
    ProfileAndLevel.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
MPEG2VideoDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = CDCIEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result)  && ! SingleSequence.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, SingleSequence));
  if ( ASDCP_SUCCESS(result)  && ! ConstantBFrames.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, ConstantBFrames));
  if ( ASDCP_SUCCESS(result)  && ! CodedContentType.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, CodedContentType));
  if ( ASDCP_SUCCESS(result)  && ! LowDelay.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, LowDelay));
  if ( ASDCP_SUCCESS(result)  && ! ClosedGOP.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, ClosedGOP));
  if ( ASDCP_SUCCESS(result)  && ! IdenticalGOP.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, IdenticalGOP));
  if ( ASDCP_SUCCESS(result)  && ! MaxGOP.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, MaxGOP));
  if ( ASDCP_SUCCESS(result)  && ! BPictureCount.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, BPictureCount));
  if ( ASDCP_SUCCESS(result)  && ! BitRate.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, BitRate));
  if ( ASDCP_SUCCESS(result)  && ! ProfileAndLevel.empty() ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS_OPT(MPEG2VideoDescriptor, ProfileAndLevel));
  return result;
}

//
void
MPEG2VideoDescriptor::Copy(const MPEG2VideoDescriptor& rhs)
{
  CDCIEssenceDescriptor::Copy(rhs);
  SingleSequence = rhs.SingleSequence;
  ConstantBFrames = rhs.ConstantBFrames;
  CodedContentType = rhs.CodedContentType;
  LowDelay = rhs.LowDelay;
  ClosedGOP = rhs.ClosedGOP;
  IdenticalGOP = rhs.IdenticalGOP;
  MaxGOP = rhs.MaxGOP;
  BPictureCount = rhs.BPictureCount;
  BitRate = rhs.BitRate;
  ProfileAndLevel = rhs.ProfileAndLevel;
}

//
void
MPEG2VideoDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  CDCIEssenceDescriptor::Dump(stream);
  if ( ! SingleSequence.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "SingleSequence", SingleSequence.get());
  }
  if ( ! ConstantBFrames.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ConstantBFrames", ConstantBFrames.get());
  }
  if ( ! CodedContentType.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "CodedContentType", CodedContentType.get());
  }
  if ( ! LowDelay.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "LowDelay", LowDelay.get());
  }
  if ( ! ClosedGOP.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ClosedGOP", ClosedGOP.get());
  }
  if ( ! IdenticalGOP.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "IdenticalGOP", IdenticalGOP.get());
  }
  if ( ! MaxGOP.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "MaxGOP", MaxGOP.get());
  }
  if ( ! BPictureCount.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "BPictureCount", BPictureCount.get());
  }
  if ( ! BitRate.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "BitRate", BitRate.get());
  }
  if ( ! ProfileAndLevel.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "ProfileAndLevel", ProfileAndLevel.get());
  }
}

//
ASDCP::Result_t
MPEG2VideoDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
MPEG2VideoDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// DMSegment

//

DMSegment::DMSegment(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), EventStartPosition(0), Duration(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_DMSegment);
}

DMSegment::DMSegment(const DMSegment& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_DMSegment);
  Copy(rhs);
}


//
ASDCP::Result_t
DMSegment::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(DMSegment, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(DMSegment, EventStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(DMSegment, Duration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(DMSegment, EventComment));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(DMSegment, DMFramework));
  return result;
}

//
ASDCP::Result_t
DMSegment::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(DMSegment, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(DMSegment, EventStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(DMSegment, Duration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(DMSegment, EventComment));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(DMSegment, DMFramework));
  return result;
}

//
void
DMSegment::Copy(const DMSegment& rhs)
{
  InterchangeObject::Copy(rhs);
  DataDefinition = rhs.DataDefinition;
  EventStartPosition = rhs.EventStartPosition;
  Duration = rhs.Duration;
  EventComment = rhs.EventComment;
  DMFramework = rhs.DMFramework;
}

//
void
DMSegment::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "DataDefinition", DataDefinition.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "EventStartPosition", i64sz(EventStartPosition, identbuf));
  fprintf(stream, "  %22s = %s\n",  "Duration", i64sz(Duration, identbuf));
  fprintf(stream, "  %22s = %s\n",  "EventComment", EventComment.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "DMFramework", DMFramework.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
DMSegment::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
DMSegment::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CryptographicFramework

//

CryptographicFramework::CryptographicFramework(const Dictionary*& d) : InterchangeObject(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_CryptographicFramework);
}

CryptographicFramework::CryptographicFramework(const CryptographicFramework& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_CryptographicFramework);
  Copy(rhs);
}


//
ASDCP::Result_t
CryptographicFramework::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicFramework, ContextSR));
  return result;
}

//
ASDCP::Result_t
CryptographicFramework::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicFramework, ContextSR));
  return result;
}

//
void
CryptographicFramework::Copy(const CryptographicFramework& rhs)
{
  InterchangeObject::Copy(rhs);
  ContextSR = rhs.ContextSR;
}

//
void
CryptographicFramework::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ContextSR", ContextSR.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
CryptographicFramework::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
CryptographicFramework::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CryptographicContext

//

CryptographicContext::CryptographicContext(const Dictionary*& d) : InterchangeObject(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_CryptographicContext);
}

CryptographicContext::CryptographicContext(const CryptographicContext& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_CryptographicContext);
  Copy(rhs);
}


//
ASDCP::Result_t
CryptographicContext::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, ContextID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, SourceEssenceContainer));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, CipherAlgorithm));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, MICAlgorithm));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, CryptographicKeyID));
  return result;
}

//
ASDCP::Result_t
CryptographicContext::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, ContextID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, SourceEssenceContainer));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, CipherAlgorithm));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, MICAlgorithm));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, CryptographicKeyID));
  return result;
}

//
void
CryptographicContext::Copy(const CryptographicContext& rhs)
{
  InterchangeObject::Copy(rhs);
  ContextID = rhs.ContextID;
  SourceEssenceContainer = rhs.SourceEssenceContainer;
  CipherAlgorithm = rhs.CipherAlgorithm;
  MICAlgorithm = rhs.MICAlgorithm;
  CryptographicKeyID = rhs.CryptographicKeyID;
}

//
void
CryptographicContext::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ContextID", ContextID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "SourceEssenceContainer", SourceEssenceContainer.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "CipherAlgorithm", CipherAlgorithm.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "MICAlgorithm", MICAlgorithm.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "CryptographicKeyID", CryptographicKeyID.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
CryptographicContext::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
CryptographicContext::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericDataEssenceDescriptor

//

GenericDataEssenceDescriptor::GenericDataEssenceDescriptor(const Dictionary*& d) : FileDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_GenericDataEssenceDescriptor);
}

GenericDataEssenceDescriptor::GenericDataEssenceDescriptor(const GenericDataEssenceDescriptor& rhs) : FileDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_GenericDataEssenceDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
GenericDataEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDataEssenceDescriptor, DataEssenceCoding));
  return result;
}

//
ASDCP::Result_t
GenericDataEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) && DataEssenceCoding.HasValue() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDataEssenceDescriptor, DataEssenceCoding));
  return result;
}

//
void
GenericDataEssenceDescriptor::Copy(const GenericDataEssenceDescriptor& rhs)
{
  FileDescriptor::Copy(rhs);
  DataEssenceCoding = rhs.DataEssenceCoding;
}

//
void
GenericDataEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "DataEssenceCoding", DataEssenceCoding.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
GenericDataEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GenericDataEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimedTextDescriptor

//

TimedTextDescriptor::TimedTextDescriptor(const Dictionary*& d) : GenericDataEssenceDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_TimedTextDescriptor);
}

TimedTextDescriptor::TimedTextDescriptor(const TimedTextDescriptor& rhs) : GenericDataEssenceDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_TimedTextDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
TimedTextDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextDescriptor, ResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextDescriptor, UCSEncoding));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextDescriptor, NamespaceURI));
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(TimedTextDescriptor, RFC5646LanguageTagList));
    RFC5646LanguageTagList.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
TimedTextDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextDescriptor, ResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextDescriptor, UCSEncoding));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextDescriptor, NamespaceURI));
  if ( ASDCP_SUCCESS(result)  && ! RFC5646LanguageTagList.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(TimedTextDescriptor, RFC5646LanguageTagList));
  return result;
}

//
void
TimedTextDescriptor::Copy(const TimedTextDescriptor& rhs)
{
  GenericDataEssenceDescriptor::Copy(rhs);
  ResourceID = rhs.ResourceID;
  UCSEncoding = rhs.UCSEncoding;
  NamespaceURI = rhs.NamespaceURI;
  RFC5646LanguageTagList = rhs.RFC5646LanguageTagList;
}

//
void
TimedTextDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericDataEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ResourceID", ResourceID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "UCSEncoding", UCSEncoding.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "NamespaceURI", NamespaceURI.EncodeString(identbuf, IdentBufferLen));
  if ( ! RFC5646LanguageTagList.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "RFC5646LanguageTagList", RFC5646LanguageTagList.get().EncodeString(identbuf, IdentBufferLen));
  }
}

//
ASDCP::Result_t
TimedTextDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
TimedTextDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimedTextResourceSubDescriptor

//

TimedTextResourceSubDescriptor::TimedTextResourceSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), EssenceStreamID(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_TimedTextResourceSubDescriptor);
}

TimedTextResourceSubDescriptor::TimedTextResourceSubDescriptor(const TimedTextResourceSubDescriptor& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_TimedTextResourceSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
TimedTextResourceSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextResourceSubDescriptor, AncillaryResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextResourceSubDescriptor, MIMEMediaType));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(TimedTextResourceSubDescriptor, EssenceStreamID));
  return result;
}

//
ASDCP::Result_t
TimedTextResourceSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextResourceSubDescriptor, AncillaryResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextResourceSubDescriptor, MIMEMediaType));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(TimedTextResourceSubDescriptor, EssenceStreamID));
  return result;
}

//
void
TimedTextResourceSubDescriptor::Copy(const TimedTextResourceSubDescriptor& rhs)
{
  InterchangeObject::Copy(rhs);
  AncillaryResourceID = rhs.AncillaryResourceID;
  MIMEMediaType = rhs.MIMEMediaType;
  EssenceStreamID = rhs.EssenceStreamID;
}

//
void
TimedTextResourceSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "AncillaryResourceID", AncillaryResourceID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "MIMEMediaType", MIMEMediaType.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "EssenceStreamID", EssenceStreamID);
}

//
ASDCP::Result_t
TimedTextResourceSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
TimedTextResourceSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// StereoscopicPictureSubDescriptor

//

StereoscopicPictureSubDescriptor::StereoscopicPictureSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_StereoscopicPictureSubDescriptor);
}

StereoscopicPictureSubDescriptor::StereoscopicPictureSubDescriptor(const StereoscopicPictureSubDescriptor& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_StereoscopicPictureSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  return result;
}

//
void
StereoscopicPictureSubDescriptor::Copy(const StereoscopicPictureSubDescriptor& rhs)
{
  InterchangeObject::Copy(rhs);
}

//
void
StereoscopicPictureSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
}

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// ContainerConstraintSubDescriptor

//

ContainerConstraintSubDescriptor::ContainerConstraintSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_ContainerConstraintSubDescriptor);
}

ContainerConstraintSubDescriptor::ContainerConstraintSubDescriptor(const ContainerConstraintSubDescriptor& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_ContainerConstraintSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
ContainerConstraintSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
ContainerConstraintSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  return result;
}

//
void
ContainerConstraintSubDescriptor::Copy(const ContainerConstraintSubDescriptor& rhs)
{
  InterchangeObject::Copy(rhs);
}

//
void
ContainerConstraintSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
}

//
ASDCP::Result_t
ContainerConstraintSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ContainerConstraintSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// NetworkLocator

//

NetworkLocator::NetworkLocator(const Dictionary*& d) : InterchangeObject(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_NetworkLocator);
}

NetworkLocator::NetworkLocator(const NetworkLocator& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_NetworkLocator);
  Copy(rhs);
}


//
ASDCP::Result_t
NetworkLocator::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(NetworkLocator, URLString));
  return result;
}

//
ASDCP::Result_t
NetworkLocator::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(NetworkLocator, URLString));
  return result;
}

//
void
NetworkLocator::Copy(const NetworkLocator& rhs)
{
  InterchangeObject::Copy(rhs);
  URLString = rhs.URLString;
}

//
void
NetworkLocator::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "URLString", URLString.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
NetworkLocator::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
NetworkLocator::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// MCALabelSubDescriptor

//

MCALabelSubDescriptor::MCALabelSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_MCALabelSubDescriptor);
}

MCALabelSubDescriptor::MCALabelSubDescriptor(const MCALabelSubDescriptor& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_MCALabelSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
MCALabelSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(MCALabelSubDescriptor, MCALabelDictionaryID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(MCALabelSubDescriptor, MCALinkID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(MCALabelSubDescriptor, MCATagSymbol));
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCATagName));
    MCATagName.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) { 
    result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCAChannelID));
    MCAChannelID.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, RFC5646SpokenLanguage));
    RFC5646SpokenLanguage.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCATitle));
    MCATitle.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCATitleVersion));
    MCATitleVersion.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCATitleSubVersion));
    MCATitleSubVersion.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCAEpisode));
    MCAEpisode.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCAPartitionKind));
    MCAPartitionKind.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCAPartitionNumber));
    MCAPartitionNumber.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCAAudioContentKind));
    MCAAudioContentKind.set_has_value( result == RESULT_OK );
  }
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(MCALabelSubDescriptor, MCAAudioElementKind));
    MCAAudioElementKind.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
MCALabelSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(MCALabelSubDescriptor, MCALabelDictionaryID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(MCALabelSubDescriptor, MCALinkID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(MCALabelSubDescriptor, MCATagSymbol));
  if ( ASDCP_SUCCESS(result)  && ! MCATagName.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCATagName));
  if ( ASDCP_SUCCESS(result)  && ! MCAChannelID.empty() ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCAChannelID));
  if ( ASDCP_SUCCESS(result)  && ! RFC5646SpokenLanguage.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, RFC5646SpokenLanguage));
  if ( ASDCP_SUCCESS(result)  && ! MCATitle.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCATitle));
  if ( ASDCP_SUCCESS(result)  && ! MCATitleVersion.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCATitleVersion));
  if ( ASDCP_SUCCESS(result)  && ! MCATitleSubVersion.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCATitleSubVersion));
  if ( ASDCP_SUCCESS(result)  && ! MCAEpisode.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCAEpisode));
  if ( ASDCP_SUCCESS(result)  && ! MCAPartitionKind.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCAPartitionKind));
  if ( ASDCP_SUCCESS(result)  && ! MCAPartitionNumber.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCAPartitionNumber));
  if ( ASDCP_SUCCESS(result)  && ! MCAAudioContentKind.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCAAudioContentKind));
  if ( ASDCP_SUCCESS(result)  && ! MCAAudioElementKind.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(MCALabelSubDescriptor, MCAAudioElementKind));
  return result;
}

//
void
MCALabelSubDescriptor::Copy(const MCALabelSubDescriptor& rhs)
{
  InterchangeObject::Copy(rhs);
  MCALabelDictionaryID = rhs.MCALabelDictionaryID;
  MCALinkID = rhs.MCALinkID;
  MCATagSymbol = rhs.MCATagSymbol;
  MCATagName = rhs.MCATagName;
  MCAChannelID = rhs.MCAChannelID;
  RFC5646SpokenLanguage = rhs.RFC5646SpokenLanguage;
  MCATitle = rhs.MCATitle;
  MCATitleVersion = rhs.MCATitleVersion;
  MCATitleSubVersion = rhs.MCATitleSubVersion;
  MCAEpisode = rhs.MCAEpisode;
  MCAPartitionKind = rhs.MCAPartitionKind;
  MCAPartitionNumber = rhs.MCAPartitionNumber;
  MCAAudioContentKind = rhs.MCAAudioContentKind;
  MCAAudioElementKind = rhs.MCAAudioElementKind;
}

//
void
MCALabelSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "MCALabelDictionaryID", MCALabelDictionaryID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "MCALinkID", MCALinkID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "MCATagSymbol", MCATagSymbol.EncodeString(identbuf, IdentBufferLen));
  if ( ! MCATagName.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MCATagName", MCATagName.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MCAChannelID.empty() ) {
    fprintf(stream, "  %22s = %d\n",  "MCAChannelID", MCAChannelID.get());
  }
  if ( ! RFC5646SpokenLanguage.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "RFC5646SpokenLanguage", RFC5646SpokenLanguage.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MCATitle.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MCATitle", MCATitle.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MCATitleVersion.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MCATitleVersion", MCATitleVersion.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MCATitleSubVersion.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MCATitleSubVersion", MCATitleSubVersion.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MCAEpisode.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MCAEpisode", MCAEpisode.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MCAPartitionKind.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MCAPartitionKind", MCAPartitionKind.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MCAPartitionNumber.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MCAPartitionNumber", MCAPartitionNumber.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MCAAudioContentKind.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MCAAudioContentKind", MCAAudioContentKind.get().EncodeString(identbuf, IdentBufferLen));
  }
  if ( ! MCAAudioElementKind.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "MCAAudioElementKind", MCAAudioElementKind.get().EncodeString(identbuf, IdentBufferLen));
  }
}

//
ASDCP::Result_t
MCALabelSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
MCALabelSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// AudioChannelLabelSubDescriptor

//

AudioChannelLabelSubDescriptor::AudioChannelLabelSubDescriptor(const Dictionary*& d) : MCALabelSubDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_AudioChannelLabelSubDescriptor);
}

AudioChannelLabelSubDescriptor::AudioChannelLabelSubDescriptor(const AudioChannelLabelSubDescriptor& rhs) : MCALabelSubDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_AudioChannelLabelSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
AudioChannelLabelSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(AudioChannelLabelSubDescriptor, SoundfieldGroupLinkID));
    SoundfieldGroupLinkID.set_has_value( result == RESULT_OK );
  }
  return result;
}

//
ASDCP::Result_t
AudioChannelLabelSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result)  && ! SoundfieldGroupLinkID.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(AudioChannelLabelSubDescriptor, SoundfieldGroupLinkID));
  return result;
}

//
void
AudioChannelLabelSubDescriptor::Copy(const AudioChannelLabelSubDescriptor& rhs)
{
  MCALabelSubDescriptor::Copy(rhs);
  SoundfieldGroupLinkID = rhs.SoundfieldGroupLinkID;
}

//
void
AudioChannelLabelSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  MCALabelSubDescriptor::Dump(stream);
  if ( ! SoundfieldGroupLinkID.empty() ) {
    fprintf(stream, "  %22s = %s\n",  "SoundfieldGroupLinkID", SoundfieldGroupLinkID.get().EncodeString(identbuf, IdentBufferLen));
  }
}

//
ASDCP::Result_t
AudioChannelLabelSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
AudioChannelLabelSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SoundfieldGroupLabelSubDescriptor

//

SoundfieldGroupLabelSubDescriptor::SoundfieldGroupLabelSubDescriptor(const Dictionary*& d) : MCALabelSubDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_SoundfieldGroupLabelSubDescriptor);
}

SoundfieldGroupLabelSubDescriptor::SoundfieldGroupLabelSubDescriptor(const SoundfieldGroupLabelSubDescriptor& rhs) : MCALabelSubDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_SoundfieldGroupLabelSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
SoundfieldGroupLabelSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) {
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(SoundfieldGroupLabelSubDescriptor, GroupOfSoundfieldGroupsLinkID));
  }
  return result;
}

//
ASDCP::Result_t
SoundfieldGroupLabelSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result)  && ! GroupOfSoundfieldGroupsLinkID.empty() ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(SoundfieldGroupLabelSubDescriptor, GroupOfSoundfieldGroupsLinkID));
  return result;
}

//
void
SoundfieldGroupLabelSubDescriptor::Copy(const SoundfieldGroupLabelSubDescriptor& rhs)
{
  MCALabelSubDescriptor::Copy(rhs);
  GroupOfSoundfieldGroupsLinkID = rhs.GroupOfSoundfieldGroupsLinkID;
}

//
void
SoundfieldGroupLabelSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  MCALabelSubDescriptor::Dump(stream);
  if ( ! GroupOfSoundfieldGroupsLinkID.empty() ) {
    fprintf(stream, "  %22s:\n",  "GroupOfSoundfieldGroupsLinkID");
  GroupOfSoundfieldGroupsLinkID.get().Dump(stream);
  }
}

//
ASDCP::Result_t
SoundfieldGroupLabelSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
SoundfieldGroupLabelSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GroupOfSoundfieldGroupsLabelSubDescriptor

//

GroupOfSoundfieldGroupsLabelSubDescriptor::GroupOfSoundfieldGroupsLabelSubDescriptor(const Dictionary*& d) : MCALabelSubDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_GroupOfSoundfieldGroupsLabelSubDescriptor);
}

GroupOfSoundfieldGroupsLabelSubDescriptor::GroupOfSoundfieldGroupsLabelSubDescriptor(const GroupOfSoundfieldGroupsLabelSubDescriptor& rhs) : MCALabelSubDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_GroupOfSoundfieldGroupsLabelSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
GroupOfSoundfieldGroupsLabelSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
GroupOfSoundfieldGroupsLabelSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::WriteToTLVSet(TLVSet);
  return result;
}

//
void
GroupOfSoundfieldGroupsLabelSubDescriptor::Copy(const GroupOfSoundfieldGroupsLabelSubDescriptor& rhs)
{
  MCALabelSubDescriptor::Copy(rhs);
}

//
void
GroupOfSoundfieldGroupsLabelSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  MCALabelSubDescriptor::Dump(stream);
}

//
ASDCP::Result_t
GroupOfSoundfieldGroupsLabelSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GroupOfSoundfieldGroupsLabelSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// DCDataDescriptor

//

DCDataDescriptor::DCDataDescriptor(const Dictionary*& d) : GenericDataEssenceDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_DCDataDescriptor);
}

DCDataDescriptor::DCDataDescriptor(const DCDataDescriptor& rhs) : GenericDataEssenceDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_DCDataDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
DCDataDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
DCDataDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::WriteToTLVSet(TLVSet);
  return result;
}

//
void
DCDataDescriptor::Copy(const DCDataDescriptor& rhs)
{
  GenericDataEssenceDescriptor::Copy(rhs);
}

//
void
DCDataDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericDataEssenceDescriptor::Dump(stream);
}

//
ASDCP::Result_t
DCDataDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
DCDataDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// PrivateDCDataDescriptor

//

PrivateDCDataDescriptor::PrivateDCDataDescriptor(const Dictionary*& d) : GenericDataEssenceDescriptor(d), m_Dict(d)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_PrivateDCDataDescriptor);
}

PrivateDCDataDescriptor::PrivateDCDataDescriptor(const PrivateDCDataDescriptor& rhs) : GenericDataEssenceDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_PrivateDCDataDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
PrivateDCDataDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
PrivateDCDataDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::WriteToTLVSet(TLVSet);
  return result;
}

//
void
PrivateDCDataDescriptor::Copy(const PrivateDCDataDescriptor& rhs)
{
  GenericDataEssenceDescriptor::Copy(rhs);
}

//
void
PrivateDCDataDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericDataEssenceDescriptor::Dump(stream);
}

//
ASDCP::Result_t
PrivateDCDataDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
PrivateDCDataDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// DolbyAtmosSubDescriptor

//

DolbyAtmosSubDescriptor::DolbyAtmosSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), FirstFrame(0), MaxChannelCount(0), MaxObjectCount(0), AtmosVersion(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_DolbyAtmosSubDescriptor);
}

DolbyAtmosSubDescriptor::DolbyAtmosSubDescriptor(const DolbyAtmosSubDescriptor& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_DolbyAtmosSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
DolbyAtmosSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(DolbyAtmosSubDescriptor, AtmosID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(DolbyAtmosSubDescriptor, FirstFrame));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(DolbyAtmosSubDescriptor, MaxChannelCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(DolbyAtmosSubDescriptor, MaxObjectCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(DolbyAtmosSubDescriptor, AtmosVersion));
  return result;
}

//
ASDCP::Result_t
DolbyAtmosSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(DolbyAtmosSubDescriptor, AtmosID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(DolbyAtmosSubDescriptor, FirstFrame));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(DolbyAtmosSubDescriptor, MaxChannelCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(DolbyAtmosSubDescriptor, MaxObjectCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(DolbyAtmosSubDescriptor, AtmosVersion));
  return result;
}

//
void
DolbyAtmosSubDescriptor::Copy(const DolbyAtmosSubDescriptor& rhs)
{
  InterchangeObject::Copy(rhs);
  AtmosID = rhs.AtmosID;
  FirstFrame = rhs.FirstFrame;
  MaxChannelCount = rhs.MaxChannelCount;
  MaxObjectCount = rhs.MaxObjectCount;
  AtmosVersion = rhs.AtmosVersion;
}

//
void
DolbyAtmosSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "AtmosID", AtmosID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "FirstFrame", FirstFrame);
  fprintf(stream, "  %22s = %d\n",  "MaxChannelCount", MaxChannelCount);
  fprintf(stream, "  %22s = %d\n",  "MaxObjectCount", MaxObjectCount);
  fprintf(stream, "  %22s = %d\n",  "AtmosVersion", AtmosVersion);
}

//
ASDCP::Result_t
DolbyAtmosSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
DolbyAtmosSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// PHDRMetadataTrackSubDescriptor

//

PHDRMetadataTrackSubDescriptor::PHDRMetadataTrackSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), SourceTrackID(0), SimplePayloadSID(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_PHDRMetadataTrackSubDescriptor);
}

PHDRMetadataTrackSubDescriptor::PHDRMetadataTrackSubDescriptor(const PHDRMetadataTrackSubDescriptor& rhs) : InterchangeObject(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_PHDRMetadataTrackSubDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
PHDRMetadataTrackSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(PHDRMetadataTrackSubDescriptor, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(PHDRMetadataTrackSubDescriptor, SourceTrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(PHDRMetadataTrackSubDescriptor, SimplePayloadSID));
  return result;
}

//
ASDCP::Result_t
PHDRMetadataTrackSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(PHDRMetadataTrackSubDescriptor, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(PHDRMetadataTrackSubDescriptor, SourceTrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(PHDRMetadataTrackSubDescriptor, SimplePayloadSID));
  return result;
}

//
void
PHDRMetadataTrackSubDescriptor::Copy(const PHDRMetadataTrackSubDescriptor& rhs)
{
  InterchangeObject::Copy(rhs);
  DataDefinition = rhs.DataDefinition;
  SourceTrackID = rhs.SourceTrackID;
  SimplePayloadSID = rhs.SimplePayloadSID;
}

//
void
PHDRMetadataTrackSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "DataDefinition", DataDefinition.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "SourceTrackID", SourceTrackID);
  fprintf(stream, "  %22s = %d\n",  "SimplePayloadSID", SimplePayloadSID);
}

//
ASDCP::Result_t
PHDRMetadataTrackSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
PHDRMetadataTrackSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// PIMFDynamicMetadataDescriptor

//

PIMFDynamicMetadataDescriptor::PIMFDynamicMetadataDescriptor(const Dictionary*& d) : GenericDataEssenceDescriptor(d), m_Dict(d), GlobalPayloadSID(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_PIMFDynamicMetadataDescriptor);
}

PIMFDynamicMetadataDescriptor::PIMFDynamicMetadataDescriptor(const PIMFDynamicMetadataDescriptor& rhs) : GenericDataEssenceDescriptor(rhs.m_Dict), m_Dict(rhs.m_Dict)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_PIMFDynamicMetadataDescriptor);
  Copy(rhs);
}


//
ASDCP::Result_t
PIMFDynamicMetadataDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(PIMFDynamicMetadataDescriptor, GlobalPayloadSID));
  return result;
}

//
ASDCP::Result_t
PIMFDynamicMetadataDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(PIMFDynamicMetadataDescriptor, GlobalPayloadSID));
  return result;
}

//
void
PIMFDynamicMetadataDescriptor::Copy(const PIMFDynamicMetadataDescriptor& rhs)
{
  GenericDataEssenceDescriptor::Copy(rhs);
  GlobalPayloadSID = rhs.GlobalPayloadSID;
}

//
void
PIMFDynamicMetadataDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericDataEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "GlobalPayloadSID", GlobalPayloadSID);
}

//
ASDCP::Result_t
PIMFDynamicMetadataDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
PIMFDynamicMetadataDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//
// end Metadata.cpp
//
