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
/*! \file    h__02_Writer.cpp
  \version $Id: h__02_Writer.cpp,v 1.15 2016/12/03 21:26:24 jhurst Exp $
  \brief   MXF file writer base class
*/

#include "AS_02_internal.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

static const ui32_t CBRIndexEntriesPerSegment = 5000;


//------------------------------------------------------------------------------------------
//

AS_02::MXF::AS02IndexWriterVBR::AS02IndexWriterVBR(const ASDCP::Dictionary*& d) :
  Partition(d), m_CurrentSegment(0), m_Dict(d), m_Lookup(0)
{
  BodySID = 0;
  IndexSID = 129;
  MinorVersion = 3;
}

AS_02::MXF::AS02IndexWriterVBR::~AS02IndexWriterVBR() {}

//
Result_t
AS_02::MXF::AS02IndexWriterVBR::WriteToFile(Kumu::FileWriter& Writer)
{
  assert(m_Dict);
  ASDCP::FrameBuffer index_body_buffer;
  ui32_t   index_body_size = m_PacketList->m_List.size() * MaxIndexSegmentSize; // segment-count * max-segment-size
  Result_t result = index_body_buffer.Capacity(index_body_size); 
  ui64_t start_position = 0;

  if ( m_CurrentSegment != 0 )
    {
      m_CurrentSegment->IndexDuration = m_CurrentSegment->IndexEntryArray.size();
      start_position = m_CurrentSegment->IndexStartPosition + m_CurrentSegment->IndexDuration;
      m_CurrentSegment = 0;
    }

  std::list<InterchangeObject*>::iterator pl_i = m_PacketList->m_List.begin();
  for ( ; pl_i != m_PacketList->m_List.end() && KM_SUCCESS(result); pl_i++ )
    {
      InterchangeObject* object = *pl_i;
      object->m_Lookup = m_Lookup;

      ASDCP::FrameBuffer WriteWrapper;
      WriteWrapper.SetData(index_body_buffer.Data() + index_body_buffer.Size(),
			   index_body_buffer.Capacity() - index_body_buffer.Size());
      result = object->WriteToBuffer(WriteWrapper);
      index_body_buffer.Size(index_body_buffer.Size() + WriteWrapper.Size());
      delete *pl_i;
      *pl_i = 0;
    }

  m_PacketList->m_List.clear();

  if ( KM_SUCCESS(result) )
    {
      IndexByteCount = index_body_buffer.Size();
      UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      result = Partition::WriteToFile(Writer, body_ul);
    }

  if ( KM_SUCCESS(result) )
    {
      ui32_t write_count = 0;
      result = Writer.Write(index_body_buffer.RoData(), index_body_buffer.Size(), &write_count);
      assert(write_count == index_body_buffer.Size());
    }

  if ( KM_SUCCESS(result) )
    {
      m_CurrentSegment = new IndexTableSegment(m_Dict);
      assert(m_CurrentSegment);
      AddChildObject(m_CurrentSegment);
      m_CurrentSegment->DeltaEntryArray.push_back(IndexTableSegment::DeltaEntry());
      m_CurrentSegment->IndexEditRate = m_EditRate;
      m_CurrentSegment->IndexStartPosition = start_position;
    }

  return result;
}

//
void
AS_02::MXF::AS02IndexWriterVBR::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  Partition::Dump(stream);

  std::list<InterchangeObject*>::iterator i = m_PacketList->m_List.begin();
  for ( ; i != m_PacketList->m_List.end(); ++i )
    {
      (*i)->Dump(stream);
    }
}

//
ui32_t
AS_02::MXF::AS02IndexWriterVBR::GetDuration() const
{
  ui32_t duration = 0;
  std::list<InterchangeObject*>::const_iterator i;

  for ( i = m_PacketList->m_List.begin(); i != m_PacketList->m_List.end(); ++i )
    {
      IndexTableSegment* segment = dynamic_cast<IndexTableSegment*>(*i);
      if ( segment != 0 )
	{
	  duration += segment->IndexEntryArray.size();
	}
    }

  return duration;
}

//
void
AS_02::MXF::AS02IndexWriterVBR::PushIndexEntry(const IndexTableSegment::IndexEntry& Entry)
{
  // do we have an available segment?
  if ( m_CurrentSegment == 0 )
    { // no, set up a new segment
      m_CurrentSegment = new IndexTableSegment(m_Dict);
      assert(m_CurrentSegment);
      AddChildObject(m_CurrentSegment);
      m_CurrentSegment->DeltaEntryArray.push_back(IndexTableSegment::DeltaEntry());
      m_CurrentSegment->IndexEditRate = m_EditRate;
      m_CurrentSegment->IndexStartPosition = 0;
    }

  m_CurrentSegment->IndexEntryArray.push_back(Entry);
}

void
AS_02::MXF::AS02IndexWriterVBR::SetEditRate(const ASDCP::Rational& edit_rate)
{
  m_EditRate = edit_rate;
}



//------------------------------------------------------------------------------------------
//

//
AS_02::h__AS02WriterFrame::h__AS02WriterFrame(const ASDCP::Dictionary& d) :
  h__AS02Writer<AS_02::MXF::AS02IndexWriterVBR>(d), m_IndexStrategy(AS_02::IS_FOLLOW) {}

AS_02::h__AS02WriterFrame::~h__AS02WriterFrame() {}

//
Result_t
AS_02::h__AS02WriterFrame::WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC)
{
  ui64_t this_stream_offset = m_StreamOffset; // m_StreamOffset will be changed by the call to Write_EKLV_Packet

  Result_t result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
				      m_StreamOffset, FrameBuf, EssenceUL, Ctx, HMAC);

  if ( KM_SUCCESS(result) )
    {  
      IndexTableSegment::IndexEntry Entry;
      Entry.StreamOffset = this_stream_offset;
      m_IndexWriter.PushIndexEntry(Entry);
    }

  if ( m_FramesWritten > 1 && ( ( m_FramesWritten + 1 ) % m_PartitionSpace ) == 0 )
    {
      m_IndexWriter.ThisPartition = m_File.Tell();
      m_IndexWriter.WriteToFile(m_File);
      m_RIP.PairArray.push_back(RIP::PartitionPair(0, m_IndexWriter.ThisPartition));

      UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      Partition body_part(m_Dict);
      body_part.MajorVersion = m_HeaderPart.MajorVersion;
      body_part.MinorVersion = m_HeaderPart.MinorVersion;
      body_part.BodySID = 1;
      body_part.OperationalPattern = m_HeaderPart.OperationalPattern;
      body_part.EssenceContainers = m_HeaderPart.EssenceContainers;
      body_part.ThisPartition = m_File.Tell();

      body_part.BodyOffset = m_StreamOffset;
      result = body_part.WriteToFile(m_File, body_ul);
      m_RIP.PairArray.push_back(RIP::PartitionPair(1, body_part.ThisPartition));
    }

  return result;
}


//------------------------------------------------------------------------------------------
//


AS_02::MXF::AS02IndexWriterCBR::AS02IndexWriterCBR(const ASDCP::Dictionary*& d) :
  Partition(d), m_CurrentSegment(0), m_Dict(d), m_Lookup(0), m_Duration(0), m_SampleSize(0)
{
  BodySID = 0;
  IndexSID = 129;
  MinorVersion = 3;
}

AS_02::MXF::AS02IndexWriterCBR::~AS02IndexWriterCBR() {}

//
Result_t
AS_02::MXF::AS02IndexWriterCBR::WriteToFile(Kumu::FileWriter& Writer)
{
  assert(m_Dict);
  ASDCP::FrameBuffer index_body_buffer;
  ui32_t   index_body_size = MaxIndexSegmentSize; // segment-count * max-segment-size
  Result_t result = index_body_buffer.Capacity(index_body_size); 

  m_CurrentSegment = new IndexTableSegment(m_Dict);
  assert(m_CurrentSegment);
  m_CurrentSegment->m_Lookup = m_Lookup;
  m_CurrentSegment->IndexEditRate = m_EditRate;
  m_CurrentSegment->IndexStartPosition = 0;
  m_CurrentSegment->IndexDuration = m_Duration;
  m_CurrentSegment->EditUnitByteCount = m_SampleSize;
  AddChildObject(m_CurrentSegment);

  ASDCP::FrameBuffer WriteWrapper;
  WriteWrapper.SetData(index_body_buffer.Data() + index_body_buffer.Size(),
		       index_body_buffer.Capacity() - index_body_buffer.Size());

  result = m_CurrentSegment->WriteToBuffer(WriteWrapper);
  index_body_buffer.Size(index_body_buffer.Size() + WriteWrapper.Size());
  delete m_CurrentSegment;
  m_CurrentSegment = 0;
  m_PacketList->m_List.clear();

  if ( KM_SUCCESS(result) )
    {
      IndexByteCount = index_body_buffer.Size();
      UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      result = Partition::WriteToFile(Writer, body_ul);
    }

  if ( KM_SUCCESS(result) )
    {
      ui32_t write_count = 0;
      result = Writer.Write(index_body_buffer.RoData(), index_body_buffer.Size(), &write_count);
      assert(write_count == index_body_buffer.Size());
    }

  return result;
}

//
ui32_t
AS_02::MXF::AS02IndexWriterCBR::GetDuration() const
{
  return m_Duration;
}

//
void
AS_02::MXF::AS02IndexWriterCBR::SetEditRate(const ASDCP::Rational& edit_rate, const ui32_t& sample_size)
{
  m_EditRate = edit_rate;
  m_SampleSize = sample_size;
}


//------------------------------------------------------------------------------------------
//

//
AS_02::h__AS02WriterClip::h__AS02WriterClip(const ASDCP::Dictionary& d) :
  h__AS02Writer<AS_02::MXF::AS02IndexWriterCBR>(d),
  m_ECStart(0), m_ClipStart(0), m_IndexStrategy(AS_02::IS_FOLLOW) {}

AS_02::h__AS02WriterClip::~h__AS02WriterClip() {}

//
bool
AS_02::h__AS02WriterClip::HasOpenClip() const
{
  return m_ClipStart != 0;
}

//
Result_t
AS_02::h__AS02WriterClip::StartClip(const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( Ctx != 0 )
    {
      DefaultLogSink().Error("Encryption not yet supported for PCM clip-wrap.\n");
      return RESULT_STATE;
    }

  if ( m_ClipStart != 0 )
    {
      DefaultLogSink().Error("Cannot open clip, clip already open.\n");
      return RESULT_STATE;
    }

  m_ClipStart = m_File.Tell();
  byte_t clip_buffer[24] = {0};
  memcpy(clip_buffer, EssenceUL, 16);
  bool check = Kumu::write_BER(clip_buffer+16, 0, 8);
  assert(check);
  return m_File.Write(clip_buffer, 24);
}

//
Result_t
AS_02::h__AS02WriterClip::WriteClipBlock(const ASDCP::FrameBuffer& FrameBuf)
{
  if ( m_ClipStart == 0 )
    {
      DefaultLogSink().Error("Cannot write clip block, no clip open.\n");
      return RESULT_STATE;
    }

  return m_File.Write(FrameBuf.RoData(), FrameBuf.Size());
}

//
Result_t
AS_02::h__AS02WriterClip::FinalizeClip(ui32_t bytes_per_frame)
{
  if ( m_ClipStart == 0 )
    {
      DefaultLogSink().Error("Cannot close clip, clip not open.\n");
      return RESULT_STATE;
    }

  ui64_t current_position = m_File.Tell();
  Result_t result = m_File.Seek(m_ClipStart+16);

  if ( KM_SUCCESS(result) )
    {
      byte_t clip_buffer[8] = {0};
      ui64_t size = static_cast<ui64_t>(m_FramesWritten) * bytes_per_frame;
      bool check = Kumu::write_BER(clip_buffer, size, 8);
      assert(check);
      result = m_File.Write(clip_buffer, 8);
    }

  if ( KM_SUCCESS(result) )
    {
      result = m_File.Seek(current_position);
      m_ClipStart = 0;
    }
  
  return result;
}



//
// end h__02_Writer.cpp
//
