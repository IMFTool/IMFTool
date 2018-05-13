/*
Copyright (c) 2011-2016, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
John Hurst

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
/*! \file    AS_02_JP2K.cpp
  \version $Id: AS_02_JP2K.cpp,v 1.16 2016/12/01 20:12:37 jhurst Exp $
  \brief   AS-02 library, JPEG 2000 essence reader and writer implementation
*/

#include "AS_02_internal.h"

#include <iostream>
#include <iomanip>

using namespace ASDCP;
using namespace ASDCP::JP2K;
using Kumu::GenRandomValue;

//------------------------------------------------------------------------------------------

static std::string JP2K_PACKAGE_LABEL = "File Package: SMPTE ST 422 / ST 2067-5 frame wrapping of JPEG 2000 codestreams";
static std::string PICT_DEF_LABEL = "Image Track";

//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of JPEG 2000 reader


class AS_02::JP2K::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  h__Reader(const Dictionary& d) :
    AS_02::h__AS02Reader(d) {}

  virtual ~h__Reader() {}

  Result_t    OpenRead(const std::string&);
  Result_t    ReadFrame(ui32_t, ASDCP::JP2K::FrameBuffer&, AESDecContext*, HMACContext*);
};

//
Result_t
AS_02::JP2K::MXFReader::h__Reader::OpenRead(const std::string& filename)
{
  Result_t result = OpenMXFRead(filename.c_str());

  if( KM_SUCCESS(result) )
    {
      InterchangeObject* tmp_iobj = 0;

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(CDCIEssenceDescriptor), &tmp_iobj);

      if ( tmp_iobj == 0 )
	{
	  m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), &tmp_iobj);
	}

      if ( tmp_iobj == 0 )
	{
	  DefaultLogSink().Error("RGBAEssenceDescriptor nor CDCIEssenceDescriptor found.\n");
	}

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor), &tmp_iobj);

      if ( tmp_iobj == 0 )
	{
	  DefaultLogSink().Error("JPEG2000PictureSubDescriptor not found.\n");
	}

      std::list<InterchangeObject*> ObjectList;
      m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(Track), ObjectList);

      if ( ObjectList.empty() )
	{
	  DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
	  return RESULT_AS02_FORMAT;
	}
    }

  return result;
}

//
//
Result_t
AS_02::JP2K::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, ASDCP::JP2K::FrameBuffer& FrameBuf,
		      ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_JPEG2000Essence), Ctx, HMAC);
}

//------------------------------------------------------------------------------------------
//

AS_02::JP2K::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultCompositeDict());
}


AS_02::JP2K::MXFReader::~MXFReader()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::JP2K::MXFReader::OP1aHeader()
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
AS_02::JP2K::MXFReader::AS02IndexReader()
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
AS_02::JP2K::MXFReader::RIP()
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
Result_t
AS_02::JP2K::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename);
}

//
Result_t
AS_02::JP2K::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
Result_t
AS_02::JP2K::MXFReader::ReadFrame(ui32_t FrameNum, ASDCP::JP2K::FrameBuffer& FrameBuf,
					   ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
Result_t
AS_02::JP2K::MXFReader::FillWriterInfo(WriterInfo& Info) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      Info = m_Reader->m_Info;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
void
AS_02::JP2K::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->m_HeaderPart.Dump(stream);
    }
}


//
void
AS_02::JP2K::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->m_IndexAccess.Dump(stream);
    }
}

//------------------------------------------------------------------------------------------

//
class AS_02::JP2K::MXFWriter::h__Writer : public AS_02::h__AS02WriterFrame
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;

public:
  byte_t            m_EssenceUL[SMPTE_UL_LENGTH];

  h__Writer(const Dictionary& d) : h__AS02WriterFrame(d), m_EssenceSubDescriptor(0) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer(){}

  Result_t OpenWrite(const std::string&, ASDCP::MXF::FileDescriptor* essence_descriptor,
		     ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
		     const AS_02::IndexStrategy_t& IndexStrategy,
		     const ui32_t& PartitionSpace, const ui32_t& HeaderSize);
  Result_t SetSourceStream(const std::string& label, const ASDCP::Rational& edit_rate);
  Result_t WriteFrame(const ASDCP::JP2K::FrameBuffer&, ASDCP::AESEncContext*, ASDCP::HMACContext*);
  Result_t Finalize();
};


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
Result_t
AS_02::JP2K::MXFWriter::h__Writer::OpenWrite(const std::string& filename,
					     ASDCP::MXF::FileDescriptor* essence_descriptor,
					     ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
					     const AS_02::IndexStrategy_t& IndexStrategy,
					     const ui32_t& PartitionSpace_sec, const ui32_t& HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    {
      KM_RESULT_STATE_HERE();
	return RESULT_STATE;
    }

  if ( m_IndexStrategy != AS_02::IS_FOLLOW )
    {
      DefaultLogSink().Error("Only strategy IS_FOLLOW is supported at this time.\n");
      return Kumu::RESULT_NOTIMPL;
    }

  Result_t result = m_File.OpenWrite(filename.c_str());

  if ( KM_SUCCESS(result) )
    {
      m_IndexStrategy = IndexStrategy;
      m_PartitionSpace = PartitionSpace_sec; // later converted to edit units by SetSourceStream()
      m_HeaderSize = HeaderSize;

      if ( essence_descriptor->GetUL() != UL(m_Dict->ul(MDD_RGBAEssenceDescriptor))
	   && essence_descriptor->GetUL() != UL(m_Dict->ul(MDD_CDCIEssenceDescriptor)) )
	{
	  DefaultLogSink().Error("Essence descriptor is not a RGBAEssenceDescriptor or CDCIEssenceDescriptor.\n");
	  essence_descriptor->Dump();
	  return RESULT_AS02_FORMAT;
	}

      m_EssenceDescriptor = essence_descriptor;

      ASDCP::MXF::InterchangeObject_list_t::iterator i;
      for ( i = essence_sub_descriptor_list.begin(); i != essence_sub_descriptor_list.end(); ++i )
	{
	  if ( (*i)->GetUL() != UL(m_Dict->ul(MDD_JPEG2000PictureSubDescriptor)) )
	    {
	      DefaultLogSink().Error("Essence sub-descriptor is not a JPEG2000PictureSubDescriptor.\n");
	      (*i)->Dump();
	    }

	  m_EssenceSubDescriptorList.push_back(*i);
	  GenRandomValue((*i)->InstanceUID);
	  m_EssenceDescriptor->SubDescriptors.push_back((*i)->InstanceUID);
	  *i = 0; // parent will only free the ones we don't keep
	}

      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the first jpeg codestream stream.
Result_t
AS_02::JP2K::MXFWriter::h__Writer::SetSourceStream(const std::string& label, const ASDCP::Rational& edit_rate)
{
  assert(m_Dict);
  if ( ! m_State.Test_INIT() )
    {
      KM_RESULT_STATE_HERE();
	return RESULT_STATE;
    }

  memcpy(m_EssenceUL, m_Dict->ul(MDD_JPEG2000Essence), SMPTE_UL_LENGTH);
  m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
  Result_t result = m_State.Goto_READY();

  if ( KM_SUCCESS(result) )
    {
      UL wrapping_label = UL(m_Dict->ul(MDD_MXFGCP1FrameWrappedPictureElement));

      CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(m_EssenceDescriptor);
      if ( cdci_descriptor )
	{
	  if ( cdci_descriptor->FrameLayout ) // 0 == progressive, 1 == interlace
	    {
	      wrapping_label = UL(m_Dict->ul(MDD_MXFGCI1FrameWrappedPictureElement));
	    }
	}

      result = WriteAS02Header(label, wrapping_label,
			       PICT_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_PictureDataDef)),
			       edit_rate, derive_timecode_rate_from_edit_rate(edit_rate));

      if ( KM_SUCCESS(result) )
	{
	  this->m_IndexWriter.SetPrimerLookup(&this->m_HeaderPart.m_Primer);
	  this->m_IndexWriter.SetEditRate(m_EssenceDescriptor->SampleRate);
	}
    }

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
Result_t
AS_02::JP2K::MXFWriter::h__Writer::WriteFrame(const ASDCP::JP2K::FrameBuffer& FrameBuf,
					      AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( FrameBuf.Size() == 0 )
    {
      DefaultLogSink().Error("The frame buffer size is zero.\n");
      return RESULT_PARAM;
    }

  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    {
      result = m_State.Goto_RUNNING(); // first time through
    }

  if ( KM_SUCCESS(result) )
    {
      result = WriteEKLVPacket(FrameBuf, m_EssenceUL, Ctx, HMAC);
      m_FramesWritten++;
    }

  return result;
}

// Closes the MXF file, writing the index and other closing information.
//
Result_t
AS_02::JP2K::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    {
      KM_RESULT_STATE_HERE();
	return RESULT_STATE;
    }

  Result_t result = m_State.Goto_FINAL();

  if ( KM_SUCCESS(result) )
    {
      result = WriteAS02Footer();
    }

  return result;
}


//------------------------------------------------------------------------------------------



AS_02::JP2K::MXFWriter::MXFWriter()
{
}

AS_02::JP2K::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::JP2K::MXFWriter::OP1aHeader()
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
AS_02::JP2K::MXFWriter::RIP()
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
Result_t
AS_02::JP2K::MXFWriter::OpenWrite(const std::string& filename, const ASDCP::WriterInfo& Info,
				  ASDCP::MXF::FileDescriptor* essence_descriptor,
				  ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
				  const ASDCP::Rational& edit_rate, const ui32_t& header_size,
				  const IndexStrategy_t& strategy, const ui32_t& partition_space)
{
  if ( essence_descriptor == 0 )
    {
      DefaultLogSink().Error("Essence descriptor object required.\n");
      return RESULT_PARAM;
    }

  m_Writer = new AS_02::JP2K::MXFWriter::h__Writer(DefaultSMPTEDict());
  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, essence_descriptor, essence_sub_descriptor_list,
					strategy, partition_space, header_size);

  if ( KM_SUCCESS(result) )
    result = m_Writer->SetSourceStream(JP2K_PACKAGE_LABEL, edit_rate);

  if ( KM_FAILURE(result) )
    m_Writer.release();

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
Result_t 
AS_02::JP2K::MXFWriter::WriteFrame(const ASDCP::JP2K::FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
Result_t
AS_02::JP2K::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//
// end AS_02_JP2K.cpp
//
