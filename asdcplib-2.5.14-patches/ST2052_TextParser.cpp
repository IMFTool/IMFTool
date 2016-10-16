/*
Copyright (c) 2013-2016, John Hurst
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
/*! \file    ST2052_TimedText.cpp
    \version $Id: ST2052_TextParser.cpp,v 1.5 2016/03/02 18:57:41 jhurst Exp $       
    \brief   AS-DCP library, PCM essence reader and writer implementation
*/

#include "AS_02_internal.h"
#include "KM_xml.h"
#include <openssl/sha.h>

using namespace Kumu;
using namespace ASDCP;

using Kumu::DefaultLogSink;

const char* c_tt_namespace_name = "http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt";


//------------------------------------------------------------------------------------------

//
int const NS_ID_LENGTH = 16;

//
static byte_t s_png_id_prefix[NS_ID_LENGTH] = {
  // RFC 4122 type 5
  // 2067-2 5.4.5 / RFC4122 Appendix C
  0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1,
  0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
};

//
static byte_t s_font_id_prefix[NS_ID_LENGTH] = {
  // RFC 4122 type 5
  // 2067-2 5.4.6
  0xb6, 0xcc, 0x57, 0xa0, 0x87, 0xe7, 0x4e, 0x75,
  0xb1, 0xc3, 0x33, 0x59, 0xf3, 0xae, 0x88, 0x17
};

//
static Kumu::UUID
create_4122_type5_id(const std::string& subject_name, const byte_t* ns_id)
{
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, ns_id, NS_ID_LENGTH);
  SHA1_Update(&ctx, (byte_t*)subject_name.c_str(), subject_name.size());

  const ui32_t sha_len = 20;
  byte_t bin_buf[sha_len];
  SHA1_Final(bin_buf, &ctx);

  // Derive the asset ID from the digest. Make it a type-5 UUID
  byte_t buf[UUID_Length];
  memcpy(buf, bin_buf, UUID_Length);
  buf[6] &= 0x0f; // clear bits 4-7
  buf[6] |= 0x50; // set UUID version 'digest'
  buf[8] &= 0x3f; // clear bits 6&7
  buf[8] |= 0x80; // set bit 7
  return Kumu::UUID(buf);
}

//
static Kumu::UUID
create_png_name_id(const std::string& image_name)
{
  return create_4122_type5_id(image_name, s_png_id_prefix);
}

//
static Kumu::UUID
create_font_name_id(const std::string& font_name)
{
  return create_4122_type5_id(font_name, s_font_id_prefix);
}

//
static Kumu::Mutex sg_default_font_family_list_lock;
static std::set<std::string> sg_default_font_family_list;

static void
setup_default_font_family_list()
{
  sg_default_font_family_list.insert("default");
  sg_default_font_family_list.insert("monospace");
  sg_default_font_family_list.insert("sansSerif");
  sg_default_font_family_list.insert("serif");
  sg_default_font_family_list.insert("monospaceSansSerif");
  sg_default_font_family_list.insert("monospaceSerif");
  sg_default_font_family_list.insert("proportionalSansSerif");
  sg_default_font_family_list.insert("proportionalSerif");
}


//------------------------------------------------------------------------------------------


AS_02::TimedText::Type5UUIDFilenameResolver::Type5UUIDFilenameResolver() {}
AS_02::TimedText::Type5UUIDFilenameResolver::~Type5UUIDFilenameResolver() {}

const byte_t PNGMagic[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
const byte_t OpenTypeMagic[5] = { 0x4f, 0x54, 0x54, 0x4f, 0x00 };
const byte_t TrueTypeMagic[5] = { 0x00, 0x01, 0x00, 0x00, 0x00 };

//
Result_t
AS_02::TimedText::Type5UUIDFilenameResolver::OpenRead(const std::string& dirname)
{
  DirScannerEx dir_reader;
  DirectoryEntryType_t ft;
  std::string next_item;
  std::string abs_dirname = PathMakeCanonical(dirname);
  byte_t read_buffer[16];

  if ( abs_dirname.empty() )
    {
      abs_dirname = ".";
    }

  Result_t result = dir_reader.Open(abs_dirname);

  if ( KM_SUCCESS(result) )
    {
      while ( KM_SUCCESS(dir_reader.GetNext(next_item, ft)) )
        {
          if ( next_item[0] == '.' ) continue; // no hidden files
	  std::string tmp_path = PathJoin(abs_dirname, next_item);

	  if ( ft == DET_FILE )
	    {
	      FileReader reader;
	      Result_t read_result = reader.OpenRead(tmp_path);

	      if ( KM_SUCCESS(read_result) )
		{
		  read_result = reader.Read(read_buffer, 16);
		}

	      if ( KM_SUCCESS(read_result) )
		{
		  // is it PNG?
		  if ( memcmp(read_buffer, PNGMagic, sizeof(PNGMagic)) == 0 )
		    {
		      UUID asset_id = create_png_name_id(next_item);
		      m_ResourceMap.insert(ResourceMap::value_type(asset_id, next_item));
		    }
		  // is it a font?
		  else if ( memcmp(read_buffer, OpenTypeMagic, sizeof(OpenTypeMagic)) == 0
			    || memcmp(read_buffer, TrueTypeMagic, sizeof(TrueTypeMagic)) == 0 )
		    {
		      std::string font_root_name = PathSetExtension(next_item, "");
		      UUID asset_id = create_font_name_id(font_root_name);
		      m_ResourceMap.insert(ResourceMap::value_type(asset_id, next_item));
		    }
		}
	    }
	}
    }

  return result;
}

//
Result_t
AS_02::TimedText::Type5UUIDFilenameResolver::ResolveRID(const byte_t* uuid, ASDCP::TimedText::FrameBuffer& FrameBuf) const
{
  Kumu::UUID tmp_id(uuid);
  char buf[64];

  ResourceMap::const_iterator i = m_ResourceMap.find(tmp_id);

  if ( i == m_ResourceMap.end() )
    {
      DefaultLogSink().Debug("Missing timed-text resource \"%s\"\n", tmp_id.EncodeHex(buf, 64));
      return RESULT_NOT_FOUND;
    }

  FileReader Reader;

  DefaultLogSink().Debug("Retrieving resource %s from file %s\n", tmp_id.EncodeHex(buf, 64), i->second.c_str());

  Result_t result = Reader.OpenRead(i->second.c_str());

  if ( KM_SUCCESS(result) )
    {
      ui32_t read_count, read_size = Reader.Size();
      result = FrameBuf.Capacity(read_size);
      
      if ( KM_SUCCESS(result) )
	result = Reader.Read(FrameBuf.Data(), read_size, &read_count);
      
      if ( KM_SUCCESS(result) )
	FrameBuf.Size(read_count);
    }

  return result;
}

Result_t
AS_02::TimedText::Type5UUIDFilenameResolver::PngNameToType5UUID(const std::string& name, Kumu::UUID& uuid){
	if (name.find("/") == std::string::npos) {
	      DefaultLogSink(). Error("PNG file name must not contain a path.\n");
	      return RESULT_READFAIL;
	}
	uuid = create_png_name_id(name);
	return RESULT_OK;
}

Result_t
AS_02::TimedText::Type5UUIDFilenameResolver::FontNameToType5UUID(const std::string& name, Kumu::UUID& uuid){
	if (name.find("/") == std::string::npos) {
	      DefaultLogSink(). Error("Font file name must not contain a path.\n");
	      return RESULT_READFAIL;
	}
	uuid = create_font_name_id(name);
	return RESULT_OK;
}

//------------------------------------------------------------------------------------------

typedef std::map<Kumu::UUID, ASDCP::TimedText::MIMEType_t> ResourceTypeMap_t;

class AS_02::TimedText::ST2052_TextParser::h__TextParser
{
  XMLElement  m_Root;
  ResourceTypeMap_t m_ResourceTypes;
  Result_t OpenRead();

  ASDCP_NO_COPY_CONSTRUCT(h__TextParser);

public:
  std::string m_Filename;
  std::string m_XMLDoc;
  TimedTextDescriptor  m_TDesc;
  ASDCP::mem_ptr<ASDCP::TimedText::IResourceResolver> m_DefaultResolver;

  h__TextParser() : m_Root("**ParserRoot**")
  {
    memset(&m_TDesc.AssetID, 0, UUIDlen);
  }

  ~h__TextParser() {}

  ASDCP::TimedText::IResourceResolver* GetDefaultResolver()
  {
    if ( m_DefaultResolver.empty() )
      {
	AS_02::TimedText::Type5UUIDFilenameResolver *resolver = new AS_02::TimedText::Type5UUIDFilenameResolver;
	resolver->OpenRead(PathDirname(m_Filename));
	m_DefaultResolver = resolver;
      }
    
    return m_DefaultResolver;
  }

  Result_t OpenRead(const std::string& filename);
  Result_t OpenRead(const std::string& xml_doc, const std::string& filename);
  Result_t ReadAncillaryResource(const byte_t *uuid, ASDCP::TimedText::FrameBuffer& FrameBuf,
				 const ASDCP::TimedText::IResourceResolver& Resolver) const;
};

//
Result_t
AS_02::TimedText::ST2052_TextParser::h__TextParser::OpenRead(const std::string& filename)
{
  Result_t result = ReadFileIntoString(filename, m_XMLDoc);

  if ( KM_SUCCESS(result) )
    {
      m_Filename = filename;
      result = OpenRead();
    }

  return result;
}

//
Result_t
AS_02::TimedText::ST2052_TextParser::h__TextParser::OpenRead(const std::string& xml_doc, const std::string& filename)
{
  m_XMLDoc = xml_doc;
  m_Filename = filename;
  return OpenRead();
}

//
template <class VisitorType>
bool
apply_visitor(const XMLElement& element, VisitorType& visitor)
{
  const ElementList& l = element.GetChildren();
  ElementList::const_iterator i;

  for ( i = l.begin(); i != l.end(); ++i )
    {
      if ( ! visitor.Element(**i) )
	{
	  return false;
	}

      if ( ! apply_visitor(**i, visitor) )
	{
	  return false;
	}
    }

  return true;
}

//
class AttributeVisitor
{
  std::string attr_name;

public:
  AttributeVisitor(const std::string& n) : attr_name(n) {}
  std::set<std::string> value_list;

  bool Element(const XMLElement& e)
  {
    const AttributeList& l = e.GetAttributes();
    AttributeList::const_iterator i;
 
    for ( i = l.begin(); i != l.end(); ++i )
      {
	if ( i->name == attr_name )
	  {
	    value_list.insert(i->value);
	  }
      }

    return true;
  }
};

//
Result_t
AS_02::TimedText::ST2052_TextParser::h__TextParser::OpenRead()
{
  setup_default_font_family_list();

  if ( ! m_Root.ParseString(m_XMLDoc.c_str()) )
    {
      DefaultLogSink(). Error("ST 2052-1 document is not well-formed.\n");
      return RESULT_FORMAT;
    }

  m_TDesc.EncodingName = "UTF-8"; // the XML parser demands UTF-8
  m_TDesc.ResourceList.clear();
  m_TDesc.ContainerDuration = 0;
  const XMLNamespace* ns = m_Root.Namespace();

  if ( ns == 0 )
    {
      DefaultLogSink(). Warn("Document has no namespace name, assuming %s\n", c_tt_namespace_name);
      m_TDesc.NamespaceName = c_tt_namespace_name;
    }
  else
    {
      m_TDesc.NamespaceName = ns->Name();
    }

  AttributeVisitor png_visitor("backgroundImage");
  apply_visitor(m_Root, png_visitor);
  std::set<std::string>::const_iterator i;

  for ( i = png_visitor.value_list.begin(); i != png_visitor.value_list.end(); ++i )
    {
      UUID asset_id = create_png_name_id(*i);
      TimedTextResourceDescriptor png_resource;
      memcpy(png_resource.ResourceID, asset_id.Value(), UUIDlen);
      png_resource.Type = ASDCP::TimedText::MT_PNG;
      m_TDesc.ResourceList.push_back(png_resource);
      m_ResourceTypes.insert(ResourceTypeMap_t::value_type(UUID(png_resource.ResourceID),
							   ASDCP::TimedText::MT_PNG));
    }

  AttributeVisitor font_visitor("fontFamily");
  apply_visitor(m_Root, font_visitor);
  char buf[64];

  for ( i = font_visitor.value_list.begin(); i != font_visitor.value_list.end(); ++i )
    {
      UUID font_id = create_font_name_id(*i);

      if ( PathIsFile(font_id.EncodeHex(buf, 64))
	   || PathIsFile(*i+".ttf")
	   || PathIsFile(*i+".otf") )
	{
	  TimedTextResourceDescriptor font_resource;
	  memcpy(font_resource.ResourceID, font_id.Value(), UUIDlen);
	  font_resource.Type = ASDCP::TimedText::MT_OPENTYPE;
	  m_TDesc.ResourceList.push_back(font_resource);
	  m_ResourceTypes.insert(ResourceTypeMap_t::value_type(UUID(font_resource.ResourceID),
							       ASDCP::TimedText::MT_OPENTYPE));
	}
      else
	{
	  AutoMutex l(sg_default_font_family_list_lock);
	  if ( sg_default_font_family_list.find(*i) == sg_default_font_family_list.end() )
	    {
	      DefaultLogSink(). Error("Unable to locate external font resource \"%s\".\n", i->c_str());
	      return RESULT_FORMAT;
	    }
	}
    }

  return RESULT_OK;
}

//
Result_t
AS_02::TimedText::ST2052_TextParser::h__TextParser::ReadAncillaryResource(const byte_t* uuid, ASDCP::TimedText::FrameBuffer& FrameBuf,
									  const ASDCP::TimedText::IResourceResolver& Resolver) const
{
  FrameBuf.AssetID(uuid);
  UUID TmpID(uuid);
  char buf[64];

  ResourceTypeMap_t::const_iterator rmi = m_ResourceTypes.find(TmpID);

  if ( rmi == m_ResourceTypes.end() )
    {
      DefaultLogSink().Error("Unknown ancillary resource id: %s\n", TmpID.EncodeHex(buf, 64));
      return RESULT_RANGE;
    }

  Result_t result = Resolver.ResolveRID(uuid, FrameBuf);

  if ( KM_SUCCESS(result) )
    {
      if ( (*rmi).second == ASDCP::TimedText::MT_PNG )
	{
	  FrameBuf.MIMEType("image/png");
	}    
      else if ( (*rmi).second == ASDCP::TimedText::MT_OPENTYPE )
	{
	  FrameBuf.MIMEType("application/x-font-opentype");
	}
      else
	{
	  FrameBuf.MIMEType("application/octet-stream");
	}
    }

  return result;
}



//------------------------------------------------------------------------------------------

AS_02::TimedText::ST2052_TextParser::ST2052_TextParser()
{
}

AS_02::TimedText::ST2052_TextParser::~ST2052_TextParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
AS_02::TimedText::ST2052_TextParser::OpenRead(const std::string& filename) const
{
  const_cast<AS_02::TimedText::ST2052_TextParser*>(this)->m_Parser = new h__TextParser;

  Result_t result = m_Parser->OpenRead(filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<AS_02::TimedText::ST2052_TextParser*>(this)->m_Parser = 0;

  return result;
}

// Parses an XML document to provide a complete set of stream metadata for the MXFWriter below.
Result_t
AS_02::TimedText::ST2052_TextParser::OpenRead(const std::string& xml_doc, const std::string& filename) const
{
  const_cast<AS_02::TimedText::ST2052_TextParser*>(this)->m_Parser = new h__TextParser;

  Result_t result = m_Parser->OpenRead(xml_doc, filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<AS_02::TimedText::ST2052_TextParser*>(this)->m_Parser = 0;

  return result;
}

//
ASDCP::Result_t
AS_02::TimedText::ST2052_TextParser::FillTimedTextDescriptor(TimedTextDescriptor& TDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  TDesc = m_Parser->m_TDesc;
  return RESULT_OK;
}

// Reads the complete Timed Text Resource into the given string.
ASDCP::Result_t
AS_02::TimedText::ST2052_TextParser::ReadTimedTextResource(std::string& s) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  s = m_Parser->m_XMLDoc;
  return RESULT_OK;
}

//
ASDCP::Result_t
AS_02::TimedText::ST2052_TextParser::ReadAncillaryResource(const Kumu::UUID& uuid, ASDCP::TimedText::FrameBuffer& FrameBuf,
							   const ASDCP::TimedText::IResourceResolver* Resolver) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  if ( Resolver == 0 )
    Resolver = m_Parser->GetDefaultResolver();

  return m_Parser->ReadAncillaryResource(uuid.Value(), FrameBuf, *Resolver);
}


//
// end ST2052_TextParser.cpp
//
