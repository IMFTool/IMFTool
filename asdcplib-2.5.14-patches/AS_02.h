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
/*! \file    AS_02.h
    \version $Id: AS_02.h,v 1.20 2016/03/09 20:05:25 jhurst Exp $       
    \brief   AS-02 library, public interface

This module implements MXF AS-02 is a set of file access objects that
offer simplified access to files conforming to the standards published
by the SMPTE Media and Packaging Technology Committee 35PM. The file
format, labeled IMF Essence Component (AKA "AS-02" for historical
reasons), is described in the following document:

 o SMPTE 2067-5:2013 IMF Essence Component

The following use cases are supported by the module:

 o Write essence to a plaintext or ciphertext AS-02 file:
     JPEG 2000 codestreams
     PCM audio streams

 o Read essence from a plaintext or ciphertext AS-02 file:
     JPEG 2000 codestreams
     PCM audio streams

 o Read header metadata from an AS-02 file

NOTE: ciphertext support for clip-wrapped PCM is not yet complete.
*/

#ifndef _AS_02_H_
#define _AS_02_H_

#include "Metadata.h"


namespace AS_02
{
  using Kumu::Result_t;

  KM_DECLARE_RESULT(AS02_FORMAT,        -116, "The file format is not proper OP-1a/AS-02.");

  namespace MXF {
    //
    // reads distributed index tables and provides a uniform lookup with
    // translated StreamOffest values (that is, StreamOffest is adjusted
    // to the actual file position
    class AS02IndexReader : public ASDCP::MXF::Partition
    {
      Kumu::ByteString m_IndexSegmentData;
      ui32_t m_Duration;
      ui32_t m_BytesPerEditUnit;

      Result_t InitFromBuffer(const byte_t* p, ui32_t l, const ui64_t& body_offset, const ui64_t& essence_container_offset);

      ASDCP_NO_COPY_CONSTRUCT(AS02IndexReader);
      AS02IndexReader();
	  
    public:
      const ASDCP::Dictionary*&   m_Dict;
      ASDCP::IPrimerLookup *m_Lookup;
    
      AS02IndexReader(const ASDCP::Dictionary*&);
      virtual ~AS02IndexReader();
    
      Result_t InitFromFile(const Kumu::FileReader& reader, const ASDCP::MXF::RIP& rip, const bool has_header_essence);
      ui32_t GetDuration() const;
      void     Dump(FILE* = 0);
      Result_t GetMDObjectByID(const Kumu::UUID&, ASDCP::MXF::InterchangeObject** = 0);
      Result_t GetMDObjectByType(const byte_t*, ASDCP::MXF::InterchangeObject** = 0);
      Result_t GetMDObjectsByType(const byte_t* ObjectID, std::list<ASDCP::MXF::InterchangeObject*>& ObjectList);
      Result_t Lookup(ui32_t frame_num, ASDCP::MXF::IndexTableSegment::IndexEntry&) const;
    };

    
    // Returns size in bytes of a single sample of data described by ADesc
    inline ui32_t CalcSampleSize(const ASDCP::MXF::WaveAudioDescriptor& d)
    {
      return (d.QuantizationBits / 8) * d.ChannelCount;
    }

      // Returns number of samples per frame of data described by ADesc
    inline ui32_t CalcSamplesPerFrame(const ASDCP::MXF::WaveAudioDescriptor& d, const ASDCP::Rational& edit_rate)
    {
      double tmpd = d.AudioSamplingRate.Quotient() / edit_rate.Quotient();
      return (ui32_t)ceil(tmpd);
    }

    // Returns the size in bytes of a frame of data described by ADesc
    inline ui32_t CalcFrameBufferSize(const ASDCP::MXF::WaveAudioDescriptor& d, const ASDCP::Rational& edit_rate)
    {
      return CalcSampleSize(d) * CalcSamplesPerFrame(d, edit_rate);
    }

    // Returns number of frames for data described by ADesc, given a duration in samples and an edit rate
    inline ui32_t CalcFramesFromDurationInSamples(const ui32_t duration_samples, const ASDCP::MXF::WaveAudioDescriptor& d,
						  const ASDCP::Rational& edit_rate)
    {
      ui32_t spf = CalcSamplesPerFrame(d, edit_rate);
      ui32_t frames = duration_samples / spf;
      
      if ( duration_samples % spf != 0 )
	{
	  ++frames;
	}

      return frames;
    }

  } // namespace MXF


  // IMF App 2 edit rates not already exposed in namespace ASDCP
  const ASDCP::Rational EditRate_29_97 = ASDCP::Rational(30000, 1001);
  const ASDCP::Rational EditRate_59_94 = ASDCP::Rational(60000, 1001);

  //---------------------------------------------------------------------------------
  // Accessors in the MXFReader and MXFWriter classes below return these types to
  // provide direct access to MXF metadata structures declared in MXF.h and Metadata.h

  // See ST 2067-5 Sec. x.y.z
  enum IndexStrategy_t
  {
    IS_LEAD,
    IS_FOLLOW,
    IS_FILE_SPECIFIC,
    IS_MAX
  };
 
  namespace JP2K
  { 
    //
    class MXFWriter
    {
      class h__Writer;
      ASDCP::mem_ptr<h__Writer> m_Writer;
      ASDCP_NO_COPY_CONSTRUCT(MXFWriter);
      
    public:
      MXFWriter();
      virtual ~MXFWriter();

      // Warning: direct manipulation of MXF structures can interfere
      // with the normal operation of the wrapper.  Caveat emptor!
      virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
      virtual ASDCP::MXF::RIP& RIP();

      // Open the file for writing. The file must not exist. Returns error if
      // the operation cannot be completed or if nonsensical data is discovered
      // in the essence descriptor.
      Result_t OpenWrite(const std::string& filename, const ASDCP::WriterInfo&,
			 ASDCP::MXF::FileDescriptor* essence_descriptor,
			 ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
			 const ASDCP::Rational& edit_rate, const ui32_t& header_size = 16384,
			 const IndexStrategy_t& strategy = IS_FOLLOW, const ui32_t& partition_space = 10);

      // Writes a frame of essence to the MXF file. If the optional AESEncContext
      // argument is present, the essence is encrypted prior to writing.
      // Fails if the file is not open, is finalized, or an operating system
      // error occurs.
      Result_t WriteFrame(const ASDCP::JP2K::FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

      // Closes the MXF file, writing the index and revised header.
      Result_t Finalize();
    };

    //
    class MXFReader
    {
      class h__Reader;
      ASDCP::mem_ptr<h__Reader> m_Reader;
      ASDCP_NO_COPY_CONSTRUCT(MXFReader);

    public:
      MXFReader();
      virtual ~MXFReader();

      // Warning: direct manipulation of MXF structures can interfere
      // with the normal operation of the wrapper.  Caveat emptor!
      virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
      virtual AS_02::MXF::AS02IndexReader& AS02IndexReader();
      virtual ASDCP::MXF::RIP& RIP();

      // Open the file for reading. The file must exist. Returns error if the
      // operation cannot be completed.
      Result_t OpenRead(const std::string& filename) const;

      // Returns RESULT_INIT if the file is not open.
      Result_t Close() const;

      // Fill a WriterInfo struct with the values from the file's header.
      // Returns RESULT_INIT if the file is not open.
      Result_t FillWriterInfo(ASDCP::WriterInfo&) const;

      // Reads a frame of essence from the MXF file. If the optional AESEncContext
      // argument is present, the essence is decrypted after reading. If the MXF
      // file is encrypted and the AESDecContext argument is NULL, the frame buffer
      // will contain the ciphertext frame data. If the HMACContext argument is
      // not NULL, the HMAC will be calculated (if the file supports it).
      // Returns RESULT_INIT if the file is not open, failure if the frame number is
      // out of range, or if optional decrypt or HAMC operations fail.
      Result_t ReadFrame(ui32_t frame_number, ASDCP::JP2K::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

      // Print debugging information to stream
      void     DumpHeaderMetadata(FILE* = 0) const;
      void     DumpIndex(FILE* = 0) const;
    };
    
  } //namespace JP2K
  

  //---------------------------------------------------------------------------------
  //
  namespace PCM
  {
    // see AS_DCP.h for related data types

    // An AS-02 PCM file is clip-wrapped, but the interface defined below mimics that used
    // for frame-wrapped essence elsewhere in this library.  The concept of frame rate
    // therefore is only relevant to these classes and is not reflected in or affected by
    // the contents of the MXF file.  The frame rate that is set on the writer is only
    // for compatibility with the existing parsers, samples are packed contiguously into
    // the same clip-wrapped packet.  Similarly, the edit rate must be set when initializing
    // the reader to signal the number of samples to be read by each call to ReadFrame();

    //
      class MXFWriter
    {
      class h__Writer;
      ASDCP::mem_ptr<h__Writer> m_Writer;
      ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

    public:
      MXFWriter();
      virtual ~MXFWriter();

      // Warning: direct manipulation of MXF structures can interfere
      // with the normal operation of the wrapper.  Caveat emptor!
      virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
      virtual ASDCP::MXF::RIP& RIP();

      // Open the file for writing. The file must not exist. Returns error if
      // the operation cannot be completed or if nonsensical data is discovered
      // in the essence descriptor.
      Result_t OpenWrite(const std::string& filename, const ASDCP::WriterInfo&,
			 ASDCP::MXF::FileDescriptor* essence_descriptor,
			 ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
			 const ASDCP::Rational& edit_rate, ui32_t HeaderSize = 16384);

      // Writes a frame of essence to the MXF file. If the optional AESEncContext
      // argument is present, the essence is encrypted prior to writing.
      // Fails if the file is not open, is finalized, or an operating system
      // error occurs.
      Result_t WriteFrame(const ASDCP::FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);
      
      // Closes the MXF file, writing the index and revised header.
      Result_t Finalize();
    };

    //
    class MXFReader
    {
      class h__Reader;
      ASDCP::mem_ptr<h__Reader> m_Reader;
      ASDCP_NO_COPY_CONSTRUCT(MXFReader);

    public:
      MXFReader();
      virtual ~MXFReader();

      // Warning: direct manipulation of MXF structures can interfere
      // with the normal operation of the wrapper.  Caveat emptor!
      virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
      virtual AS_02::MXF::AS02IndexReader& AS02IndexReader();
      virtual ASDCP::MXF::RIP& RIP();

      // Open the file for reading. The file must exist. Returns error if the
      // operation cannot be completed.
      Result_t OpenRead(const std::string& filename, const ASDCP::Rational& EditRate) const;

      // Returns RESULT_INIT if the file is not open.
      Result_t Close() const;

      // Fill a WriterInfo struct with the values from the file's header.
      // Returns RESULT_INIT if the file is not open.
      Result_t FillWriterInfo(ASDCP::WriterInfo&) const;

      // Reads a frame of essence from the MXF file. If the optional AESEncContext
      // argument is present, the essence is decrypted after reading. If the MXF
      // file is encrypted and the AESDecContext argument is NULL, the frame buffer
      // will contain the ciphertext frame data. If the HMACContext argument is
      // not NULL, the HMAC will be calculated (if the file supports it).
      // Returns RESULT_INIT if the file is not open, failure if the frame number is
      // out of range, or if optional decrypt or HAMC operations fail.
      Result_t ReadFrame(ui32_t frame_number, ASDCP::PCM::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;
      
      // Print debugging information to stream
      void     DumpHeaderMetadata(FILE* = 0) const;
      void     DumpIndex(FILE* = 0) const;
    };
  } // namespace PCM

  //---------------------------------------------------------------------------------
  //
  namespace TimedText
    {
      using ASDCP::TimedText::TimedTextDescriptor;
      using ASDCP::TimedText::TimedTextResourceDescriptor;
      using ASDCP::TimedText::ResourceList_t;

      //
      class Type5UUIDFilenameResolver : public ASDCP::TimedText::IResourceResolver
	{
	  typedef std::map<Kumu::UUID, std::string> ResourceMap;
	    
	  ResourceMap m_ResourceMap;
	  std::string m_Dirname;
	  KM_NO_COPY_CONSTRUCT(Type5UUIDFilenameResolver);

	public:
	  Type5UUIDFilenameResolver();
	  virtual ~Type5UUIDFilenameResolver();
	  Result_t OpenRead(const std::string& dirname);
	  Result_t ResolveRID(const byte_t* uuid, ASDCP::TimedText::FrameBuffer& FrameBuf) const;
	  Result_t PngNameToType5UUID(const std::string& png_name, Kumu::UUID& uuid);
	  Result_t FontNameToType5UUID(const std::string& font_name, Kumu::UUID& uuid);
	};
      
      //
      class ST2052_TextParser
	{
	  class h__TextParser;
	  ASDCP::mem_ptr<h__TextParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(ST2052_TextParser);

	public:
	  ST2052_TextParser();
	  virtual ~ST2052_TextParser();

	  // Opens an XML file for reading, parses data to provide a complete
	  // set of stream metadata for the MXFWriter below.
	  Result_t OpenRead(const std::string& filename) const;

	  // Parse an XML string 
	  Result_t OpenRead(const std::string& xml_doc, const std::string& filename) const;

	  // Fill a TimedTextDescriptor struct with the values from the file's contents.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillTimedTextDescriptor(ASDCP::TimedText::TimedTextDescriptor&) const;

	  // Reads the complete Timed Text Resource into the given string.
	  Result_t ReadTimedTextResource(std::string&) const;

	  // Reads the Ancillary Resource having the given ID. Fails if the buffer
	  // is too small or the resource does not exist. The optional Resolver
	  // argument can be provided which will be used to retrieve the resource
	  // having a particulat UUID. If a Resolver is not supplied, the default
	  // internal resolver will return the contents of the file having the UUID
	  // as the filename. The filename must exist in the same directory as the
	  // XML file opened with OpenRead().
	  Result_t ReadAncillaryResource(const Kumu::UUID&, ASDCP::TimedText::FrameBuffer&,
					 const ASDCP::TimedText::IResourceResolver* Resolver = 0) const;
	};

      //
      class MXFWriter
	{
	  class h__Writer;
	  ASDCP::mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
	  virtual ASDCP::MXF::RIP& RIP();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const std::string& filename, const ASDCP::WriterInfo&,
			     const ASDCP::TimedText::TimedTextDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes the Timed-Text Resource to the MXF file. The file must be UTF-8
	  // encoded. If the optional AESEncContext argument is present, the essence
	  // is encrypted prior to writing. Fails if the file is not open, is finalized,
	  // or an operating system error occurs.
	  // This method may only be called once, and it must be called before any
	  // call to WriteAncillaryResource(). RESULT_STATE will be returned if these
	  // conditions are not met.
	  Result_t WriteTimedTextResource(const std::string& XMLDoc, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

	  // Writes an Ancillary Resource to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs. RESULT_STATE will be returned if the method is called before
	  // WriteTimedTextResource()
	  Result_t WriteAncillaryResource(const ASDCP::TimedText::FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

      //
      class MXFReader
	{
	  class h__Reader;
	  ASDCP::mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
	  virtual AS_02::MXF::AS02IndexReader& AS02IndexReader();
	  virtual ASDCP::MXF::RIP& RIP();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const std::string& filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill a TimedTextDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillTimedTextDescriptor(ASDCP::TimedText::TimedTextDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(ASDCP::WriterInfo&) const;

	  // Reads the complete Timed Text Resource into the given string. Fails if the resource
	  // is encrypted and AESDecContext is NULL (use the following method to retrieve the
	  // raw ciphertet block).
	  Result_t ReadTimedTextResource(std::string&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

	  // Reads the complete Timed Text Resource from the MXF file. If the optional AESEncContext
	  // argument is present, the resource is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadTimedTextResource(ASDCP::TimedText::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

	  // Reads the timed-text resource having the given UUID from the MXF file. If the
	  // optional AESEncContext argument is present, the resource is decrypted after
	  // reading. If the MXF file is encrypted and the AESDecContext argument is NULL,
	  // the frame buffer will contain the ciphertext frame data. If the HMACContext
	  // argument is not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadAncillaryResource(const Kumu::UUID&, ASDCP::TimedText::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};
    } // namespace TimedText


} // namespace AS_02

#endif // _AS_02_H_

//
// end AS_02.h
//
