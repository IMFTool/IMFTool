/*
Copyright (c) 2011-2015, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
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
/*! \file    as-02-unwrap.cpp
    \version $Id: as-02-unwrap.cpp,v 1.14 2016/03/09 20:05:26 jhurst Exp $       
    \brief   AS-02 file manipulation utility

  This program extracts picture and sound from AS-02 files.

  For more information about AS-02, please refer to the header file AS_02.h
  For more information about asdcplib, please refer to the header file AS_DCP.h
*/

#include <KM_fileio.h>
#include <AS_02.h>
#include <WavFileWriter.h>

namespace ASDCP {
  Result_t MD_to_PCM_ADesc(ASDCP::MXF::WaveAudioDescriptor* ADescObj, ASDCP::PCM::AudioDescriptor& ADesc);
}

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;

//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "as-02-unwrap";  // program name for messages

// Increment the iterator, test for an additional non-option command line argument.
// Causes the caller to return if there are no remaining arguments or if the next
// argument begins with '-'.
#define TEST_EXTRA_ARG(i,c)						\
  if ( ++i >= argc || argv[(i)][0] == '-' ) {				\
    fprintf(stderr, "Argument not found for option -%c.\n", (c));	\
    return;								\
  }

//
void
banner(FILE* stream = stdout)
{
  fprintf(stream, "\n\
%s (asdcplib %s)\n\n\
Copyright (c) 2011-2015, Robert Scheler, Heiko Sparenberg Fraunhofer IIS, John Hurst\n\n\
asdcplib may be copied only under the terms of the license found at\n\
the top of every file in the asdcplib distribution kit.\n\n\
Specify the -h (help) option for further information about %s\n\n",
	  PROGRAM_NAME, ASDCP::Version(), PROGRAM_NAME);
}

//
void
usage(FILE* stream = stdout)
{
  fprintf(stream, "\
USAGE: %s [-h|-help] [-V]\n\
\n\
       %s [-1|-2] [-b <buffer-size>] [-d <duration>]\n\
       [-f <starting-frame>] [-m] [-p <frame-rate>] [-R] [-s <size>] [-v] [-W]\n\
       [-w] <input-file> [<file-prefix>]\n\n",
	  PROGRAM_NAME, PROGRAM_NAME);

  fprintf(stream, "\
Options:\n\
  -1                - Split Wave essence to mono WAV files during extract.\n\
                      Default is multichannel WAV\n\
  -2                - Split Wave essence to stereo WAV files during extract.\n\
                      Default is multichannel WAV\n\
  -b <buffer-size>  - Specify size in bytes of picture frame buffer\n\
                      Defaults to 4,194,304 (4MB)\n\
  -d <duration>     - Number of frames to process, default all\n\
  -f <start-frame>  - Starting frame number, default 0\n\
  -h | -help        - Show help\n\
  -k <key-string>   - Use key for ciphertext operations\n\
  -m                - verify HMAC values when reading\n\
  -s <size>         - Number of bytes to dump to output when -v is given\n\
  -V                - Show version information\n\
  -v                - Verbose, prints informative messages to stderr\n\
  -W                - Read input file only, do not write destination file\n\
  -w <width>        - Width of numeric element in a series of frame file names\n\
                      (default 6)\n\
  -z                - Fail if j2c inputs have unequal parameters (default)\n\
  -Z                - Ignore unequal parameters in j2c inputs\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\n");
}

//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;     // true if the given options are in error or not complete
  bool   key_flag;       // true if an encryption key was given
  bool   read_hmac;      // true if HMAC values are to be validated
  bool   split_wav;      // true if PCM is to be extracted to stereo WAV files
  bool   mono_wav;       // true if PCM is to be extracted to mono WAV files
  bool   verbose_flag;   // true if the verbose option was selected
  ui32_t fb_dump_size;   // number of bytes of frame buffer to dump
  bool   no_write_flag;  // true if no output files are to be written
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  bool   stereo_image_flag; // if true, expect stereoscopic JP2K input (left eye first)
  ui32_t number_width;   // number of digits in a serialized filename (for JPEG extract)
  ui32_t start_frame;    // frame number to begin processing
  ui32_t duration;       // number of frames to be processed
  bool   duration_flag;  // true if duration argument given
  bool   j2c_pedantic;   // passed to JP2K::SequenceParser::OpenRead
  ui32_t picture_rate;   // fps of picture when wrapping PCM
  ui32_t fb_size;        // size of picture frame buffer
  Rational edit_rate;    // frame buffer size for reading clip-wrapped PCM
  const char* file_prefix; // filename pre for files written by the extract mode
  byte_t key_value[KeyLen];  // value of given encryption key (when key_flag is true)
  byte_t key_id_value[UUIDlen];// value of given key ID (when key_id_flag is true)
  PCM::ChannelFormat_t channel_fmt; // audio channel arrangement
  const char* input_filename;
  std::string prefix_buffer;

  //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), key_flag(false), read_hmac(false), split_wav(false),
    mono_wav(false), verbose_flag(false), fb_dump_size(0), no_write_flag(false),
    version_flag(false), help_flag(false), number_width(6),
    start_frame(0), duration(0xffffffff), duration_flag(false), j2c_pedantic(true),
    picture_rate(24), fb_size(FRAME_BUFFER_SIZE), file_prefix(0),
    input_filename(0)
  {
    memset(key_value, 0, KeyLen);
    memset(key_id_value, 0, UUIDlen);

    for ( int i = 1; i < argc; ++i )
      {

	if ( (strcmp( argv[i], "-help") == 0) )
	  {
	    help_flag = true;
	    continue;
	  }
         
	if ( argv[i][0] == '-'
	     && ( isalpha(argv[i][1]) || isdigit(argv[i][1]) )
	     && argv[i][2] == 0 )
	  {
	    switch ( argv[i][1] )
	      {
	      case '1': mono_wav = true; break;
	      case '2': split_wav = true; break;

	      case 'b':
		TEST_EXTRA_ARG(i, 'b');
		fb_size = Kumu::xabs(strtol(argv[i], 0, 10));

		if ( verbose_flag )
		  fprintf(stderr, "Frame Buffer size: %u bytes.\n", fb_size);

		break;

	      case 'd':
		TEST_EXTRA_ARG(i, 'd');
		duration_flag = true;
		duration = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'f':
		TEST_EXTRA_ARG(i, 'f');
		start_frame = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'h': help_flag = true; break;
	      case 'm': read_hmac = true; break;

	      case 'p':
		TEST_EXTRA_ARG(i, 'p');
		picture_rate = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 's':
		TEST_EXTRA_ARG(i, 's');
		fb_dump_size = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'V': version_flag = true; break;
	      case 'v': verbose_flag = true; break;
	      case 'W': no_write_flag = true; break;

	      case 'w':
		TEST_EXTRA_ARG(i, 'w');
		number_width = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'Z': j2c_pedantic = false; break;
	      case 'z': j2c_pedantic = true; break;

	      default:
		fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		return;
	      }
	  }
	else
	  {
	    if ( argv[i][0] != '-' )
	      {
		if ( input_filename == 0 )
		  {
		    input_filename = argv[i];
		  }
		else if ( file_prefix == 0 )
		  {
		    file_prefix = argv[i];
		  }
	      }
	    else
	      {
		fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
		return;
	      }
	  }
      }

    if ( help_flag || version_flag )
      return;
    
    if ( input_filename == 0 )
      {
	fputs("At least one filename argument is required.\n", stderr);
	return;
      }

    if ( file_prefix == 0 )
      {
	prefix_buffer = Kumu::PathSetExtension(input_filename, "") + "_";
	file_prefix = prefix_buffer.c_str();
      }

    error_flag = false;
  }
};


//------------------------------------------------------------------------------------------
// JPEG 2000 essence


// Read one or more plaintext JPEG 2000 codestreams from a plaintext ASDCP file
// Read one or more plaintext JPEG 2000 codestreams from a ciphertext ASDCP file
// Read one or more ciphertext JPEG 2000 codestreams from a ciphertext ASDCP file
//
Result_t
read_JP2K_file(CommandOptions& Options)
{
  AESDecContext*     Context = 0;
  HMACContext*       HMAC = 0;
  AS_02::JP2K::MXFReader    Reader;
  JP2K::FrameBuffer  FrameBuffer(Options.fb_size);
  ui32_t             frame_count = 0;

  Result_t result = Reader.OpenRead(Options.input_filename);

  if ( ASDCP_SUCCESS(result) )
    {
      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	}

      ASDCP::MXF::RGBAEssenceDescriptor *rgba_descriptor = 0;
      ASDCP::MXF::CDCIEssenceDescriptor *cdci_descriptor = 0;

      result = Reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_RGBAEssenceDescriptor),
						     reinterpret_cast<MXF::InterchangeObject**>(&rgba_descriptor));

      if ( KM_SUCCESS(result) )
	{
	  assert(rgba_descriptor);
	  frame_count = rgba_descriptor->ContainerDuration;

	  if ( Options.verbose_flag )
	    {
	      rgba_descriptor->Dump();
	    }
	}
      else
	{
	  result = Reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_CDCIEssenceDescriptor),
							 reinterpret_cast<MXF::InterchangeObject**>(&cdci_descriptor));

	  if ( KM_SUCCESS(result) )
	    {
	      assert(cdci_descriptor);
	      frame_count = cdci_descriptor->ContainerDuration;

	      if ( Options.verbose_flag )
		{
		  cdci_descriptor->Dump();
		}
	    }
	  else
	    {
	      fprintf(stderr, "File does not contain an essence descriptor.\n");
	      frame_count = Reader.AS02IndexReader().GetDuration();
	    }
	}

      if ( frame_count == 0 )
	{
	  frame_count = Reader.AS02IndexReader().GetDuration();
	}

      if ( frame_count == 0 )
	{
	  fprintf(stderr, "Unable to determine file duration.\n");
	  return RESULT_FAIL;
	}
    }

  if ( ASDCP_SUCCESS(result) && Options.key_flag )
    {
      Context = new AESDecContext;
      result = Context->InitKey(Options.key_value);

      if ( ASDCP_SUCCESS(result) && Options.read_hmac )
	{
	  WriterInfo Info;
	  Reader.FillWriterInfo(Info);

	  if ( Info.UsesHMAC )
	    {
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	  else
	    {
	      fputs("File does not contain HMAC values, ignoring -m option.\n", stderr);
	    }
	}
    }

  ui32_t last_frame = Options.start_frame + ( Options.duration ? Options.duration : frame_count);
  if ( last_frame > frame_count )
    last_frame = frame_count;

  char name_format[64];
  snprintf(name_format,  64, "%%s%%0%du.j2c", Options.number_width);

  for ( ui32_t i = Options.start_frame; ASDCP_SUCCESS(result) && i < last_frame; i++ )
    {
      result = Reader.ReadFrame(i, FrameBuffer, Context, HMAC);

      char filename[1024];
      snprintf(filename, 1024, name_format, Options.file_prefix, i);

      if ( ASDCP_SUCCESS(result) && Options.verbose_flag )
	{
	  printf("Frame %d, %d bytes", i, FrameBuffer.Size());

	  if ( ! Options.no_write_flag )
	    {
	      printf(" -> %s", filename);
	    }

	  printf("\n");
	}

      if ( ASDCP_SUCCESS(result)  && ( ! Options.no_write_flag ) )
	{
	  Kumu::FileWriter OutFile;
	  ui32_t write_count;
	  result = OutFile.OpenWrite(filename);

	  if ( ASDCP_SUCCESS(result) )
	    result = OutFile.Write(FrameBuffer.Data(), FrameBuffer.Size(), &write_count);

	  if ( ASDCP_SUCCESS(result) && Options.verbose_flag )
	    {
	      FrameBuffer.Dump(stderr, Options.fb_dump_size);
	    }
	}
    }

  return result;
}

//------------------------------------------------------------------------------------------
// PCM essence

// Read one or more plaintext PCM audio streams from a plaintext ASDCP file
// Read one or more plaintext PCM audio streams from a ciphertext ASDCP file
// Read one or more ciphertext PCM audio streams from a ciphertext ASDCP file
//
Result_t
read_PCM_file(CommandOptions& Options)
{
  AESDecContext*     Context = 0;
  HMACContext*       HMAC = 0;
  AS_02::PCM::MXFReader     Reader;
  PCM::FrameBuffer   FrameBuffer;
  WavFileWriter      OutWave;
  ui32_t last_frame = 0;
  ASDCP::MXF::WaveAudioDescriptor *wave_descriptor = 0;

  if ( Options.edit_rate == Rational(0,0) ) // todo, make this available to the CLI
    {
      Options.edit_rate = EditRate_24;
    }

  Result_t result = Reader.OpenRead(Options.input_filename, Options.edit_rate);

  if ( KM_SUCCESS(result) )
    {
      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	}
      
      ASDCP::MXF::InterchangeObject* tmp_obj = 0;

      result = Reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_WaveAudioDescriptor), &tmp_obj);

      if ( KM_SUCCESS(result) )
	{
	  wave_descriptor = dynamic_cast<ASDCP::MXF::WaveAudioDescriptor*>(tmp_obj);

	  if ( wave_descriptor == 0 )
	    {
	      fprintf(stderr, "File does not contain an essence descriptor.\n");
	      return RESULT_FAIL;
	    }
      
	  if ( Options.verbose_flag )
	    {
	      wave_descriptor->Dump();
	    }

	  if ( wave_descriptor->ContainerDuration.get() == 0 )
	    {
	      fprintf(stderr, "ContainerDuration not set in file descriptor, attempting to use index duration.\n");
	      last_frame = Reader.AS02IndexReader().GetDuration();
	    }
	  else
	    {
	      last_frame = wave_descriptor->ContainerDuration;
	    }

	  if ( last_frame == 0 )
	    {
	      fprintf(stderr, "ContainerDuration not set in index, attempting to use Duration from SourceClip.\n");
	      result = Reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_SourceClip), &tmp_obj);
	      if ( KM_SUCCESS(result))
		{
	    	  ASDCP::MXF::SourceClip *sourceClip = dynamic_cast<ASDCP::MXF::SourceClip*>(tmp_obj);
	    	  if ( ! sourceClip->Duration.empty() )
		    {
		      last_frame = sourceClip->Duration;
		    }
		}
	    }

	  if ( last_frame == 0 )
	    {
	      fprintf(stderr, "Unable to determine file duration.\n");
	      return RESULT_FAIL;
	    }

	  assert(wave_descriptor);
	  FrameBuffer.Capacity(AS_02::MXF::CalcFrameBufferSize(*wave_descriptor, Options.edit_rate));
	  last_frame = AS_02::MXF::CalcFramesFromDurationInSamples(last_frame, *wave_descriptor, Options.edit_rate);
	}
    }

  if ( ASDCP_SUCCESS(result) )
    {
      if ( Options.duration > 0 && Options.duration < last_frame )
	last_frame = Options.duration;

      if ( Options.start_frame > 0 )
	{
	  if ( Options.start_frame > last_frame )
	    {
	      fprintf(stderr, "Start value greater than file duration.\n");
	      return RESULT_FAIL;
	    }

	  last_frame = Kumu::xmin(Options.start_frame + last_frame, last_frame);
	}

      last_frame = last_frame - Options.start_frame;

      PCM::AudioDescriptor ADesc;

      result = MD_to_PCM_ADesc(wave_descriptor, ADesc);

      if ( ASDCP_SUCCESS(result) )
	{
	  ADesc.ContainerDuration = last_frame;
	  ADesc.EditRate = Options.edit_rate;

	  result = OutWave.OpenWrite(ADesc, Options.file_prefix,
				     ( Options.split_wav ? WavFileWriter::ST_STEREO : 
				       ( Options.mono_wav ? WavFileWriter::ST_MONO : WavFileWriter::ST_NONE ) ));
	}
    }

  if ( ASDCP_SUCCESS(result) && Options.key_flag )
    {
      Context = new AESDecContext;
      result = Context->InitKey(Options.key_value);

      if ( ASDCP_SUCCESS(result) && Options.read_hmac )
	{
	  WriterInfo Info;
	  Reader.FillWriterInfo(Info);

	  if ( Info.UsesHMAC )
	    {
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	  else
	    {
	      fputs("File does not contain HMAC values, ignoring -m option.\n", stderr);
	    }
	}
    }

  for ( ui32_t i = Options.start_frame; ASDCP_SUCCESS(result) && i < last_frame; i++ )
    {
      result = Reader.ReadFrame(i, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( Options.verbose_flag )
	    {
	      FrameBuffer.FrameNumber(i);
	      FrameBuffer.Dump(stderr, Options.fb_dump_size);
	    }

	  if ( FrameBuffer.Size() != FrameBuffer.Capacity() )
	    {
	      fprintf(stderr, "Last frame is incomplete, padding with zeros.\n");
	      // actually, it has already been zeroed for us, we just need to recognize the appropriate size
	      FrameBuffer.Size(FrameBuffer.Capacity());
	    }

	  result = OutWave.WriteFrame(FrameBuffer);
	}
    }

  return result;
}


//------------------------------------------------------------------------------------------
// TimedText essence

// Read one or more timed text streams from a plaintext AS-02 file
//
Result_t
read_timed_text_file(CommandOptions& Options)
{
  AESDecContext*     Context = 0;
  HMACContext*       HMAC = 0;
  AS_02::TimedText::MXFReader     Reader;
  TimedText::FrameBuffer   FrameBuffer(Options.fb_size);
  //ASDCP::TimedText::FrameBuffer   FrameBuffer(Options.fb_size);
  AS_02::TimedText::TimedTextDescriptor TDesc;
  ASDCP::MXF::TimedTextDescriptor *tt_descriptor = 0;

  Result_t result = Reader.OpenRead(Options.input_filename);

  if ( ASDCP_SUCCESS(result) )
    {
      result = Reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_TimedTextDescriptor),
						     reinterpret_cast<MXF::InterchangeObject**>(&tt_descriptor));
    if ( Options.verbose_flag ) {
    	tt_descriptor->Dump();
    }


  if ( ASDCP_FAILURE(result) )
    return result;

  std::string XMLDoc;
  std::string out_path = Kumu::PathDirname(Options.file_prefix);
  ui32_t write_count;
  char buf[64];
  TimedText::ResourceList_t::const_iterator ri;

  result = Reader.ReadTimedTextResource(XMLDoc);

  if ( ASDCP_SUCCESS(result) )
    {
      Reader.FillTimedTextDescriptor(TDesc);
      FrameBuffer.Capacity(Options.fb_size);

      if ( Options.verbose_flag )
	TimedText::DescriptorDump(TDesc);
    }

  if ( ASDCP_SUCCESS(result) && ( ! Options.no_write_flag ) )
    {
      Kumu::FileWriter Writer;
      result = Writer.OpenWrite(Options.file_prefix);

      if ( ASDCP_SUCCESS(result) )
	result = Writer.Write(reinterpret_cast<const byte_t*>(XMLDoc.c_str()), XMLDoc.size(), &write_count);
    }

  for ( ri = TDesc.ResourceList.begin() ; ri != TDesc.ResourceList.end() && ASDCP_SUCCESS(result); ri++ )
    {
      result = Reader.ReadAncillaryResource(ri->ResourceID, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) && ( ! Options.no_write_flag ) )
	{
	  Kumu::FileWriter Writer;
	  if (out_path != "") {
		  result = Writer.OpenWrite(Kumu::PathJoin(out_path, Kumu::UUID(ri->ResourceID).EncodeHex(buf, 64)).c_str());
	  } else {
		  // Workaround for a bug in Kumu::PathJoin
		  result = Writer.OpenWrite(Kumu::UUID(ri->ResourceID).EncodeHex(buf, 64));
	  }

	  if ( ASDCP_SUCCESS(result) )
	    result = Writer.Write(FrameBuffer.RoData(), FrameBuffer.Size(), &write_count);

	      if ( Options.verbose_flag )
		FrameBuffer.Dump(stderr, Options.fb_dump_size);
	}
    }
    }
  return result;
}


//
int
main(int argc, const char** argv)
{
  char     str_buf[64];
  CommandOptions Options(argc, argv);

  if ( Options.version_flag )
    banner();

  if ( Options.help_flag )
    usage();

  if ( Options.version_flag || Options.help_flag )
    return 0;

  if ( Options.error_flag )
    {
      fprintf(stderr, "There was a problem. Type %s -h for help.\n", PROGRAM_NAME);
      return 3;
    }

  EssenceType_t EssenceType;
  Result_t result = ASDCP::EssenceType(Options.input_filename, EssenceType);

  if ( ASDCP_SUCCESS(result) )
    {
      switch ( EssenceType )
	{
	case ESS_AS02_JPEG_2000:
	  result = read_JP2K_file(Options);
	  break;

	case ESS_AS02_PCM_24b_48k:
	case ESS_AS02_PCM_24b_96k:
	  result = read_PCM_file(Options);
	  break;

	case ESS_AS02_TIMED_TEXT:
	  result = read_timed_text_file(Options);
	  break;

	default:
	  fprintf(stderr, "%s: Unknown file type, not AS-02 essence.\n", Options.input_filename);
	  return 5;
	}
    }

  if ( ASDCP_FAILURE(result) )
    {
      fputs("Program stopped on error.\n", stderr);

      if ( result != RESULT_FAIL )
	{
	  fputs(result, stderr);
	  fputc('\n', stderr);
	}

      return 1;
    }

  return 0;
}


//
// end as-02-unwrap.cpp
//
