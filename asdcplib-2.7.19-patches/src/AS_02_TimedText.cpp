/*
Copyright (c) 2008-2016, John Hurst
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
/*! \file    AS_DCP_TimedText.cpp
    \version $Id: AS_02_TimedText.cpp,v 1.8 2016/12/03 21:26:24 jhurst Exp $       
    \brief   AS-DCP library, PCM essence reader and writer implementation
*/


#include "AS_02_internal.h"
#include "KM_xml.h"
#include <iostream>
#include <iomanip>

using Kumu::GenRandomValue;

static std::string TIMED_TEXT_PACKAGE_LABEL = "File Package: SMPTE ST 429-5 / ST 2067-5 clip wrapping of IMF Timed Text data";
static std::string TIMED_TEXT_DEF_LABEL = "Timed Text Track";


//------------------------------------------------------------------------------------------

static const char*
MIME2str(TimedText::MIMEType_t m)
{
  if ( m == TimedText::MT_PNG )
    return "image/png";

  else if ( m == TimedText::MT_OPENTYPE )
    return "application/x-font-opentype";

  return "application/octet-stream";
}

//------------------------------------------------------------------------------------------

typedef std::map<Kumu::UUID, Kumu::UUID> ResourceMap_t;

class AS_02::TimedText::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
  ASDCP::MXF::TimedTextDescriptor* m_EssenceDescriptor;
  ResourceMap_t             m_ResourceMap;

  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  TimedTextDescriptor m_TDesc;    

  h__Reader(const Dictionary& d) : AS_02::h__AS02Reader(d), m_EssenceDescriptor(0) {
    memset(&m_TDesc.AssetID, 0, UUIDlen);
  }

  virtual ~h__Reader() {}

  Result_t    OpenRead(const std::string&);
  Result_t    MD_to_TimedText_TDesc(TimedTextDescriptor& TDesc);
  Result_t    ReadTimedTextResource(ASDCP::TimedText::FrameBuffer& FrameBuf, AESDecContext* Ctx, HMACContext* HMAC);
  Result_t    ReadAncillaryResource(const Kumu::UUID&, ASDCP::TimedText::FrameBuffer& FrameBuf, AESDecContext* Ctx, HMACContext* HMAC);
};

//
ASDCP::Result_t
AS_02::TimedText::MXFReader::h__Reader::MD_to_TimedText_TDesc(TimedTextDescriptor& TDesc)
{
  assert(m_EssenceDescriptor);
  memset(&m_TDesc.AssetID, 0, UUIDlen);
  ASDCP::MXF::TimedTextDescriptor* TDescObj = (ASDCP::MXF::TimedTextDescriptor*)m_EssenceDescriptor;

  TDesc.EditRate = TDescObj->SampleRate;
  assert(TDescObj->ContainerDuration <= 0xFFFFFFFFL);
  TDesc.ContainerDuration = (ui32_t) TDescObj->ContainerDuration;
  memcpy(TDesc.AssetID, TDescObj->ResourceID.Value(), UUIDlen);
  TDesc.NamespaceName = TDescObj->NamespaceURI;
  TDesc.EncodingName = TDescObj->UCSEncoding;

  Array<Kumu::UUID>::const_iterator sdi = TDescObj->SubDescriptors.begin();
  TimedTextResourceSubDescriptor* DescObject = 0;
  Result_t result = RESULT_OK;

  for ( ; sdi != TDescObj->SubDescriptors.end() && KM_SUCCESS(result); sdi++ )
    {
      InterchangeObject* tmp_iobj = 0;
      result = m_HeaderPart.GetMDObjectByID(*sdi, &tmp_iobj);
      DescObject = static_cast<TimedTextResourceSubDescriptor*>(tmp_iobj);

      if ( KM_SUCCESS(result) )
	{
	  TimedTextResourceDescriptor TmpResource;
	  memcpy(TmpResource.ResourceID, DescObject->AncillaryResourceID.Value(), UUIDlen);

	  if ( DescObject->MIMEMediaType.find("application/x-font-opentype") != std::string::npos
	       || DescObject->MIMEMediaType.find("application/x-opentype") != std::string::npos
	       || DescObject->MIMEMediaType.find("font/opentype") != std::string::npos )
	    {
	      TmpResource.Type = ASDCP::TimedText::MT_OPENTYPE;
	    }
	  else if ( DescObject->MIMEMediaType.find("image/png") != std::string::npos )
	    {
	      TmpResource.Type = ASDCP::TimedText::MT_PNG;
	    }
	  else
	    {
	      TmpResource.Type = ASDCP::TimedText::MT_BIN;
	    }

	  TDesc.ResourceList.push_back(TmpResource);
	  m_ResourceMap.insert(ResourceMap_t::value_type(DescObject->AncillaryResourceID, *sdi));
	}
      else
	{
	  DefaultLogSink().Error("Broken sub-descriptor link\n");
	  return RESULT_FORMAT;
	}
    }

  return result;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFReader::h__Reader::OpenRead(const std::string& filename)
{
  Result_t result = OpenMXFRead(filename.c_str());
  
  if( ASDCP_SUCCESS(result) )
    {
      if ( m_EssenceDescriptor == 0 )
	{
	  InterchangeObject* tmp_iobj = 0;
	  result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(TimedTextDescriptor), &tmp_iobj);
	  m_EssenceDescriptor = static_cast<ASDCP::MXF::TimedTextDescriptor*>(tmp_iobj);
	}

      if( ASDCP_SUCCESS(result) )
	result = MD_to_TimedText_TDesc(m_TDesc);
    }

  return result;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFReader::h__Reader::ReadTimedTextResource(ASDCP::TimedText::FrameBuffer& FrameBuf,
							      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    {
      return RESULT_INIT;
    }

  assert(m_Dict);
  Result_t result = ReadEKLVFrame(0, FrameBuf, m_Dict->ul(MDD_TimedTextEssence), Ctx, HMAC);

 if( ASDCP_SUCCESS(result) )
   {
     FrameBuf.AssetID(m_TDesc.AssetID);
     FrameBuf.MIMEType("text/xml");
   } else {
		  DefaultLogSink().Debug("Index table broken - trying to seek to MDD_TimedTextEssence\n");
		  KLReader reader;
		  m_File.Seek(0);
		  Kumu::fpos_t FilePosition = m_File.Tell();
		  result = reader.ReadKLFromFile(m_File);

		  if ( ! m_File.IsOpen() )
			  return RESULT_INIT;

		  while (!UL(reader.Key()).MatchIgnoreStream(m_Dict->ul(MDD_TimedTextEssence))) {
			  FilePosition += reader.KLLength() + reader.Length();
			  if ( FilePosition >= m_File.Size()) {
				  return RESULT_ENDOFFILE;
			  }

			  result = m_File.Seek(FilePosition);
			  result = reader.ReadKLFromFile(m_File);
		  }

		  DefaultLogSink().Debug("Found Timed Text Essence\n");
		  result = m_File.Read(FrameBuf.Data(), reader.Length());
		  FrameBuf.Size(reader.Length());

	   }

 return result;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFReader::h__Reader::ReadAncillaryResource(const Kumu::UUID& uuid,
							      ASDCP::TimedText::FrameBuffer& FrameBuf,
							      AESDecContext* Ctx, HMACContext* HMAC)
{
  ResourceMap_t::const_iterator ri = m_ResourceMap.find(uuid);
  if ( ri == m_ResourceMap.end() )
    {
      char buf[64];
      DefaultLogSink().Error("No such resource: %s\n", uuid.EncodeHex(buf, 64));
      return RESULT_RANGE;
    }

  TimedTextResourceSubDescriptor* DescObject = 0;
  // get the subdescriptor
  InterchangeObject* tmp_iobj = 0;
  Result_t result = m_HeaderPart.GetMDObjectByID((*ri).second, &tmp_iobj);
  DescObject = static_cast<TimedTextResourceSubDescriptor*>(tmp_iobj);

  if ( KM_SUCCESS(result) )
    {
      RIP::const_pair_iterator pi;
      RIP::PartitionPair TmpPair;
      ui32_t sequence = 0;

      // Look up the partition start in the RIP using the SID.
      // Count the sequence length in because this is the sequence
      // value needed to  complete the HMAC.
      for ( pi = m_RIP.PairArray.begin(); pi != m_RIP.PairArray.end(); ++pi, ++sequence )
	{
	  if ( (*pi).BodySID == DescObject->EssenceStreamID )
	    {
	      TmpPair = *pi;
	      break;
	    }
	}

      if ( TmpPair.ByteOffset == 0 )
	{
	  DefaultLogSink().Error("Body SID not found in RIP set: %d\n", DescObject->EssenceStreamID);
	  return RESULT_FORMAT;
	}

      if ( KM_SUCCESS(result) )
	{
	  FrameBuf.AssetID(uuid.Value());
	  FrameBuf.MIMEType(DescObject->MIMEMediaType);

	  // seek tp the start of the partition
	  if ( (Kumu::fpos_t)TmpPair.ByteOffset != m_LastPosition )
	    {
	      m_LastPosition = TmpPair.ByteOffset;
	      result = m_File.Seek(TmpPair.ByteOffset);
	    }

	  // read the partition header
	  ASDCP::MXF::Partition GSPart(m_Dict);
	  result = GSPart.InitFromFile(m_File);

	  if( ASDCP_SUCCESS(result) )
	    {
	      // check the SID
	      if ( DescObject->EssenceStreamID != GSPart.BodySID )
		{
		  char buf[64];
		  DefaultLogSink().Error("Generic stream partition body differs: %s\n", uuid.EncodeHex(buf, 64));
		  return RESULT_FORMAT;
		}

	      // read the essence packet
	      assert(m_Dict);
	      if( ASDCP_SUCCESS(result) )
		result = ReadEKLVPacket(0, sequence, FrameBuf, m_Dict->ul(MDD_GenericStream_DataElement), Ctx, HMAC);
	    }
	}
    }

  return result;
}


//------------------------------------------------------------------------------------------

AS_02::TimedText::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultSMPTEDict());
}


AS_02::TimedText::MXFReader::~MXFReader()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::TimedText::MXFReader::OP1aHeader()
{
  if ( m_Reader.empty() )
    {
      assert(g_OP1aHeader);
      return *g_OP1aHeader;
    }

  return m_Reader->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
AS_02::MXF::AS02IndexReader&
AS_02::TimedText::MXFReader::AS02IndexReader()
{
  if ( m_Reader.empty() )
    {
      assert(g_AS02IndexReader);
      return *g_AS02IndexReader;
    }

  return m_Reader->m_IndexAccess;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
AS_02::TimedText::MXFReader::RIP()
{
  if ( m_Reader.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Reader->m_RIP;
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
ASDCP::Result_t
AS_02::TimedText::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename);
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
AS_02::TimedText::MXFReader::FillTimedTextDescriptor(TimedText::TimedTextDescriptor& TDesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      TDesc = m_Reader->m_TDesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
AS_02::TimedText::MXFReader::FillWriterInfo(WriterInfo& Info) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      Info = m_Reader->m_Info;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFReader::ReadTimedTextResource(std::string& s, AESDecContext* Ctx, HMACContext* HMAC) const
{
  ASDCP::TimedText::FrameBuffer FrameBuf(8*Kumu::Megabyte);

  Result_t result = ReadTimedTextResource(FrameBuf, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
    s.assign((char*)FrameBuf.Data(), FrameBuf.Size());

  return result;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFReader::ReadTimedTextResource(ASDCP::TimedText::FrameBuffer& FrameBuf,
						   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadTimedTextResource(FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFReader::ReadAncillaryResource(const Kumu::UUID& uuid, ASDCP::TimedText::FrameBuffer& FrameBuf,
						   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadAncillaryResource(uuid, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


//
void
AS_02::TimedText::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
AS_02::TimedText::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}

//
ASDCP::Result_t
AS_02::TimedText::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------


//
class AS_02::TimedText::MXFWriter::h__Writer : public AS_02::h__AS02WriterClip
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  TimedTextDescriptor m_TDesc;
  byte_t m_EssenceUL[SMPTE_UL_LENGTH];
  ui32_t m_EssenceStreamID;
  ASDCP::Rational m_EditRate;

  h__Writer(const Dictionary& d) : AS_02::h__AS02WriterClip(d), m_EssenceStreamID(10)
  {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer() {}

  Result_t OpenWrite(const std::string&, ui32_t HeaderSize);
  Result_t SetSourceStream(const ASDCP::TimedText::TimedTextDescriptor&);
  Result_t WriteTimedTextResource(const std::string& XMLDoc, AESEncContext* = 0, HMACContext* = 0);
  Result_t WriteAncillaryResource(const ASDCP::TimedText::FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
  Result_t TimedText_TDesc_to_MD(ASDCP::TimedText::TimedTextDescriptor& TDesc);
};

//
ASDCP::Result_t
AS_02::TimedText::MXFWriter::h__Writer::TimedText_TDesc_to_MD(TimedText::TimedTextDescriptor& TDesc)
{
  assert(m_EssenceDescriptor);
  ASDCP::MXF::TimedTextDescriptor* TDescObj = (ASDCP::MXF::TimedTextDescriptor*)m_EssenceDescriptor;

  TDescObj->SampleRate = TDesc.EditRate;
  TDescObj->ContainerDuration = TDesc.ContainerDuration;
  TDescObj->ResourceID.Set(TDesc.AssetID);
  TDescObj->NamespaceURI = TDesc.NamespaceName;
  TDescObj->UCSEncoding = TDesc.EncodingName;

  return RESULT_OK;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFWriter::h__Writer::OpenWrite(const std::string& filename, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    {
      KM_RESULT_STATE_HERE();
      return RESULT_STATE;
    }

  Result_t result = m_File.OpenWrite(filename.c_str());

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = new ASDCP::MXF::TimedTextDescriptor(m_Dict);
      result = m_State.Goto_INIT();
    }

  return result;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFWriter::h__Writer::SetSourceStream(ASDCP::TimedText::TimedTextDescriptor const& TDesc)
{
  if ( ! m_State.Test_INIT() )
    {
      KM_RESULT_STATE_HERE();
      return RESULT_STATE;
    }

  assert(m_Dict);
  m_TDesc = TDesc;

  Result_t result = TimedText_TDesc_to_MD(m_TDesc);

  if ( KM_SUCCESS(result) )
    {
      ResourceList_t::const_iterator i;
      for ( i = m_TDesc.ResourceList.begin() ; i != m_TDesc.ResourceList.end(); ++i )
	{
	  TimedTextResourceSubDescriptor* resourceSubdescriptor = new TimedTextResourceSubDescriptor(m_Dict);
	  GenRandomValue(resourceSubdescriptor->InstanceUID);
	  resourceSubdescriptor->AncillaryResourceID.Set((*i).ResourceID);
	  resourceSubdescriptor->MIMEMediaType = MIME2str((*i).Type);
	  resourceSubdescriptor->EssenceStreamID = m_EssenceStreamID++;
	  m_EssenceSubDescriptorList.push_back((FileDescriptor*)resourceSubdescriptor);
	  m_EssenceDescriptor->SubDescriptors.push_back(resourceSubdescriptor->InstanceUID);

	  // 72 == sizeof K, L, instanceuid, uuid + sizeof int32 + tag/len * 4
	  m_HeaderSize += ( resourceSubdescriptor->MIMEMediaType.ArchiveLength() * 2 /*ArchiveLength is broken*/ ) + 72;
	}
    }  

  //Reset m_EssenceStreamID to 10 for later usage in WriteAncillaryResource
  m_EssenceStreamID = 10;
  assert(m_Dict);

  if ( KM_SUCCESS(result) )
    {
      memcpy(m_EssenceUL, m_Dict->ul(MDD_TimedTextEssence), SMPTE_UL_LENGTH);
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

  if ( KM_SUCCESS(result) )
    {
      m_EditRate = TDesc.EditRate;
      result = WriteAS02Header(TIMED_TEXT_PACKAGE_LABEL, UL(m_Dict->ul(MDD_TimedTextWrappingClip)),
			       "Data Track", UL(m_EssenceUL), UL(m_Dict->ul(MDD_DataDataDef)),
			       TDesc.EditRate, derive_timecode_rate_from_edit_rate(TDesc.EditRate));
    }

  return result;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFWriter::h__Writer::WriteTimedTextResource(const std::string& XMLDoc,
							       ASDCP::AESEncContext* Ctx, ASDCP::HMACContext* HMAC)
{
  ASDCP::FrameBuffer segment_buffer;
  IndexTableSegment::IndexEntry index_entry;
  Result_t result = m_State.Goto_RUNNING();

  if ( KM_SUCCESS(result) )
    {
      // TODO: make sure it's XML

      ui32_t str_size = XMLDoc.size();
      ASDCP::TimedText::FrameBuffer FrameBuf(str_size);
      
      memcpy(FrameBuf.Data(), XMLDoc.c_str(), str_size);
      FrameBuf.Size(str_size);
      index_entry.StreamOffset = m_StreamOffset;
      
      result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
				 m_StreamOffset, FrameBuf, m_EssenceUL, Ctx, HMAC);
    }

  if ( KM_SUCCESS(result) )
    {
      // encode the index table
      IndexTableSegment::DeltaEntry nil_delta_entry;
      IndexTableSegment segment(m_Dict);
      segment.m_Lookup = &m_HeaderPart.m_Primer;
      GenRandomValue(segment.InstanceUID);

      segment.DeltaEntryArray.push_back(nil_delta_entry);
      segment.IndexEditRate = m_EditRate;
      segment.IndexStartPosition = 0;
      segment.IndexDuration = -1;
      segment.IndexEntryArray.push_back(index_entry);

      result = segment_buffer.Capacity(MaxIndexSegmentSize); // segment-count * max-segment-size

      if ( KM_SUCCESS(result) )
	{
	  result = segment.WriteToBuffer(segment_buffer);
	}
    }

  if ( KM_SUCCESS(result) )
    {
      // create an index partition header
      Kumu::fpos_t here = m_File.Tell();
      assert(m_Dict);

      ASDCP::MXF::Partition partition(m_Dict);
      partition.MajorVersion = m_HeaderPart.MajorVersion;
      partition.MinorVersion = m_HeaderPart.MinorVersion;
      partition.ThisPartition = here;
      partition.BodySID = 0;
      partition.IndexSID = 129;
      partition.IndexByteCount = segment_buffer.Size();
      partition.PreviousPartition = m_RIP.PairArray.back().ByteOffset;
      partition.OperationalPattern = m_HeaderPart.OperationalPattern;

      m_RIP.PairArray.push_back(RIP::PartitionPair(0, here));
      partition.EssenceContainers = m_HeaderPart.EssenceContainers;
      UL TmpUL(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      result = partition.WriteToFile(m_File, TmpUL);
    }
  
  if ( KM_SUCCESS(result) )
    {
      // write the encoded index table
      ui32_t write_count = 0;
      result = m_File.Write(segment_buffer.RoData(), segment_buffer.Size(), &write_count);
      assert(write_count == segment_buffer.Size());
    }

  if ( KM_SUCCESS(result) )
    {
      m_FramesWritten++;
    }

  return result;
}


//
ASDCP::Result_t
AS_02::TimedText::MXFWriter::h__Writer::WriteAncillaryResource(const ASDCP::TimedText::FrameBuffer& FrameBuf,
							       ASDCP::AESEncContext* Ctx, ASDCP::HMACContext* HMAC)
{
  if ( ! m_State.Test_RUNNING() )
    {
      KM_RESULT_STATE_HERE();
      return RESULT_STATE;
    }

  Kumu::fpos_t here = m_File.Tell();
  assert(m_Dict);

  // create generic stream partition header
  static UL GenericStream_DataElement(m_Dict->ul(MDD_GenericStream_DataElement));
  ASDCP::MXF::Partition GSPart(m_Dict);

  GSPart.MajorVersion = m_HeaderPart.MajorVersion;
  GSPart.MinorVersion = m_HeaderPart.MinorVersion;
  GSPart.ThisPartition = here;
  GSPart.PreviousPartition = m_RIP.PairArray.back().ByteOffset;
  GSPart.BodySID = m_EssenceStreamID;
  GSPart.OperationalPattern = m_HeaderPart.OperationalPattern;

  m_RIP.PairArray.push_back(RIP::PartitionPair(m_EssenceStreamID++, here));
  GSPart.EssenceContainers = m_HeaderPart.EssenceContainers;
  UL TmpUL(m_Dict->ul(MDD_GenericStreamPartition));
  Result_t result = GSPart.WriteToFile(m_File, TmpUL);

  if ( KM_SUCCESS(result) )
    {
      result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
				 m_StreamOffset, FrameBuf, GenericStream_DataElement.Value(), Ctx, HMAC);
    }

  m_FramesWritten++;
  return result;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    {
      DefaultLogSink().Error("Cannot finalize file, the primary essence resource has not been written.\n");
      return RESULT_STATE;
    }

  m_FramesWritten = m_TDesc.ContainerDuration;

  Result_t result = m_State.Goto_FINAL();

  if ( KM_SUCCESS(result) )
    {
      result = WriteAS02Footer();
    }

  return result;
}


//------------------------------------------------------------------------------------------

AS_02::TimedText::MXFWriter::MXFWriter()
{
}

AS_02::TimedText::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::TimedText::MXFWriter::OP1aHeader()
{
  if ( m_Writer.empty() )
    {
      assert(g_OP1aHeader);
      return *g_OP1aHeader;
    }

  return m_Writer->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
AS_02::TimedText::MXFWriter::RIP()
{
  if ( m_Writer.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Writer->m_RIP;
}

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
AS_02::TimedText::MXFWriter::OpenWrite(const std::string& filename, const WriterInfo& Info,
				       const TimedTextDescriptor& TDesc, ui32_t HeaderSize)
{
  if ( Info.LabelSetType != LS_MXF_SMPTE )
    {
      DefaultLogSink().Error("Timed Text support requires LS_MXF_SMPTE\n");
      return RESULT_FORMAT;
    }

  m_Writer = new h__Writer(DefaultSMPTEDict());
  m_Writer->m_Info = Info;
  
  Result_t result = m_Writer->OpenWrite(filename, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->SetSourceStream(TDesc);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}

//
ASDCP::Result_t
AS_02::TimedText::MXFWriter::WriteTimedTextResource(const std::string& XMLDoc, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteTimedTextResource(XMLDoc, Ctx, HMAC);
}

//
ASDCP::Result_t
AS_02::TimedText::MXFWriter::WriteAncillaryResource(const ASDCP::TimedText::FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteAncillaryResource(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
AS_02::TimedText::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}



//
// end AS_02_timedText.cpp
//
