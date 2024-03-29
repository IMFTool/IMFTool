// Copyright (c) 2005-2014 Code Synthesis Tools CC
//
// This program was generated by CodeSynthesis XSD, an XML Schema to
// C++ data binding compiler.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
//
// In addition, as a special exception, Code Synthesis Tools CC gives
// permission to link this program with the Xerces-C++ library (or with
// modified versions of Xerces-C++ that use the same license as Xerces-C++),
// and distribute linked combinations including the two. You must obey
// the GNU General Public License version 2 in all respects for all of
// the code used other than Xerces-C++. If you modify this copy of the
// program, you may extend this exception to your version of the program,
// but you are not obligated to do so. If you do not wish to do so, delete
// this exception statement from your version.
//
// Furthermore, Code Synthesis Tools CC makes a special exception for
// the Free/Libre and Open Source Software (FLOSS) which is described
// in the accompanying FLOSSE file.
//

#ifndef SMPTE_429_9_2007_AM_H
#define SMPTE_429_9_2007_AM_H

#ifndef XSD_CXX11
#define XSD_CXX11
#endif

#ifndef XSD_USE_CHAR
#define XSD_USE_CHAR
#endif

#ifndef XSD_CXX_TREE_USE_CHAR
#define XSD_CXX_TREE_USE_CHAR
#endif

// Begin prologue.
//
//
// End prologue.

#include <xsd/cxx/config.hxx>

#if (XSD_INT_VERSION != 4000000L)
#error XSD runtime version mismatch
#endif

#include <xsd/cxx/pre.hxx>

#include <xsd/cxx/xml/char-utf8.hxx>

#include <xsd/cxx/tree/exceptions.hxx>
#include <xsd/cxx/tree/elements.hxx>
#include <xsd/cxx/tree/types.hxx>

#include <xsd/cxx/xml/error-handler.hxx>

#include <xsd/cxx/xml/dom/auto-ptr.hxx>

#include <xsd/cxx/tree/parsing.hxx>
#include <xsd/cxx/tree/parsing/byte.hxx>
#include <xsd/cxx/tree/parsing/unsigned-byte.hxx>
#include <xsd/cxx/tree/parsing/short.hxx>
#include <xsd/cxx/tree/parsing/unsigned-short.hxx>
#include <xsd/cxx/tree/parsing/int.hxx>
#include <xsd/cxx/tree/parsing/unsigned-int.hxx>
#include <xsd/cxx/tree/parsing/long.hxx>
#include <xsd/cxx/tree/parsing/unsigned-long.hxx>
#include <xsd/cxx/tree/parsing/boolean.hxx>
#include <xsd/cxx/tree/parsing/float.hxx>
#include <xsd/cxx/tree/parsing/double.hxx>
#include <xsd/cxx/tree/parsing/decimal.hxx>

#include <xsd/cxx/xml/dom/serialization-header.hxx>
#include <xsd/cxx/tree/serialization.hxx>
#include <xsd/cxx/tree/serialization/byte.hxx>
#include <xsd/cxx/tree/serialization/unsigned-byte.hxx>
#include <xsd/cxx/tree/serialization/short.hxx>
#include <xsd/cxx/tree/serialization/unsigned-short.hxx>
#include <xsd/cxx/tree/serialization/int.hxx>
#include <xsd/cxx/tree/serialization/unsigned-int.hxx>
#include <xsd/cxx/tree/serialization/long.hxx>
#include <xsd/cxx/tree/serialization/unsigned-long.hxx>
#include <xsd/cxx/tree/serialization/boolean.hxx>
#include <xsd/cxx/tree/serialization/float.hxx>
#include <xsd/cxx/tree/serialization/double.hxx>
#include <xsd/cxx/tree/serialization/decimal.hxx>

#include <xsd/cxx/tree/std-ostream-operators.hxx>

namespace xml_schema
{
  // anyType and anySimpleType.
  //
  typedef ::xsd::cxx::tree::type Type;
  typedef ::xsd::cxx::tree::simple_type< char, Type > SimpleType;
  typedef ::xsd::cxx::tree::type Container;

  // 8-bit
  //
  typedef signed char Byte;
  typedef unsigned char UnsignedByte;

  // 16-bit
  //
  typedef short Short;
  typedef unsigned short UnsignedShort;

  // 32-bit
  //
  typedef int Int;
  typedef unsigned int UnsignedInt;

  // 64-bit
  //
  typedef long long Long;
  typedef unsigned long long UnsignedLong;

  // Supposed to be arbitrary-length integral types.
  //
  typedef long long Integer;
  typedef long long NonPositiveInteger;
  typedef unsigned long long NonNegativeInteger;
  typedef unsigned long long PositiveInteger;
  typedef long long NegativeInteger;

  // Boolean.
  //
  typedef bool Boolean;

  // Floating-point types.
  //
  typedef float Float;
  typedef double Double;
  typedef double Decimal;

  // String types.
  //
  typedef ::xsd::cxx::tree::string< char, SimpleType > String;
  typedef ::xsd::cxx::tree::normalized_string< char, String > NormalizedString;
  typedef ::xsd::cxx::tree::token< char, NormalizedString > Token;
  typedef ::xsd::cxx::tree::name< char, Token > Name;
  typedef ::xsd::cxx::tree::nmtoken< char, Token > Nmtoken;
  typedef ::xsd::cxx::tree::nmtokens< char, SimpleType, Nmtoken > Nmtokens;
  typedef ::xsd::cxx::tree::ncname< char, Name > Ncname;
  typedef ::xsd::cxx::tree::language< char, Token > Language;

  // ID/IDREF.
  //
  typedef ::xsd::cxx::tree::id< char, Ncname > Id;
  typedef ::xsd::cxx::tree::idref< char, Ncname, Type > Idref;
  typedef ::xsd::cxx::tree::idrefs< char, SimpleType, Idref > Idrefs;

  // URI.
  //
  typedef ::xsd::cxx::tree::uri< char, SimpleType > Uri;

  // Qualified name.
  //
  typedef ::xsd::cxx::tree::qname< char, SimpleType, Uri, Ncname > Qname;

  // Binary.
  //
  typedef ::xsd::cxx::tree::buffer< char > Buffer;
  typedef ::xsd::cxx::tree::base64_binary< char, SimpleType > Base64Binary;
  typedef ::xsd::cxx::tree::hex_binary< char, SimpleType > HexBinary;

  // Date/time.
  //
  typedef ::xsd::cxx::tree::time_zone TimeZone;
  typedef ::xsd::cxx::tree::date< char, SimpleType > Date;
  typedef ::xsd::cxx::tree::date_time< char, SimpleType > DateTime;
  typedef ::xsd::cxx::tree::duration< char, SimpleType > Duration;
  typedef ::xsd::cxx::tree::gday< char, SimpleType > Gday;
  typedef ::xsd::cxx::tree::gmonth< char, SimpleType > Gmonth;
  typedef ::xsd::cxx::tree::gmonth_day< char, SimpleType > GmonthDay;
  typedef ::xsd::cxx::tree::gyear< char, SimpleType > Gyear;
  typedef ::xsd::cxx::tree::gyear_month< char, SimpleType > GyearMonth;
  typedef ::xsd::cxx::tree::time< char, SimpleType > Time;

  // Entity.
  //
  typedef ::xsd::cxx::tree::entity< char, Ncname > Entity;
  typedef ::xsd::cxx::tree::entities< char, SimpleType, Entity > Entities;

  typedef ::xsd::cxx::tree::content_order ContentOrder;
  // Namespace information and list stream. Used in
  // serialization functions.
  //
  typedef ::xsd::cxx::xml::dom::namespace_info< char > NamespaceInfo;
  typedef ::xsd::cxx::xml::dom::namespace_infomap< char > NamespaceInfomap;
  typedef ::xsd::cxx::tree::list_stream< char > ListStream;
  typedef ::xsd::cxx::tree::as_double< Double > AsDouble;
  typedef ::xsd::cxx::tree::as_decimal< Decimal > AsDecimal;
  typedef ::xsd::cxx::tree::facet Facet;

  // Flags and properties.
  //
  typedef ::xsd::cxx::tree::flags Flags;
  typedef ::xsd::cxx::tree::properties< char > Properties;

  // Parsing/serialization diagnostics.
  //
  typedef ::xsd::cxx::tree::severity Severity;
  typedef ::xsd::cxx::tree::error< char > Error;
  typedef ::xsd::cxx::tree::diagnostics< char > Diagnostics;

  // Exceptions.
  //
  typedef ::xsd::cxx::tree::exception< char > Exception;
  typedef ::xsd::cxx::tree::bounds< char > Bounds;
  typedef ::xsd::cxx::tree::duplicate_id< char > DuplicateId;
  typedef ::xsd::cxx::tree::parsing< char > Parsing;
  typedef ::xsd::cxx::tree::expected_element< char > ExpectedElement;
  typedef ::xsd::cxx::tree::unexpected_element< char > UnexpectedElement;
  typedef ::xsd::cxx::tree::expected_attribute< char > ExpectedAttribute;
  typedef ::xsd::cxx::tree::unexpected_enumerator< char > UnexpectedEnumerator;
  typedef ::xsd::cxx::tree::expected_text_content< char > ExpectedTextContent;
  typedef ::xsd::cxx::tree::no_prefix_mapping< char > NoPrefixMapping;
  typedef ::xsd::cxx::tree::no_type_info< char > NoTypeInfo;
  typedef ::xsd::cxx::tree::not_derived< char > NotDerived;
  typedef ::xsd::cxx::tree::serialization< char > Serialization;

  // Error handler callback interface.
  //
  typedef ::xsd::cxx::xml::error_handler< char > ErrorHandler;

  // DOM interaction.
  //
  namespace dom
  {
    // Automatic pointer for DOMDocument.
    //
    using ::xsd::cxx::xml::dom::unique_ptr;

#ifndef XSD_CXX_TREE_TREE_NODE_KEY__XML_SCHEMA
#define XSD_CXX_TREE_TREE_NODE_KEY__XML_SCHEMA
    // DOM user data key for back pointers to tree nodes.
    //
    const XMLCh* const treeNodeKey = ::xsd::cxx::tree::user_data_keys::node;
#endif
  }
}

// Forward declarations.
//
namespace am
{
  class UUID;
  class UserText;
  class ChunkType;
  class AssetType;
  class AssetMapType;
  class VolumeIndexType;
  class AssetType_ChunkListType;
  class AssetMapType_AssetListType;
}


#include <memory>    // ::std::unique_ptr
#include <limits>    // std::numeric_limits
#include <algorithm> // std::binary_search
#include <utility>   // std::move

#include <xsd/cxx/xml/char-utf8.hxx>

#include <xsd/cxx/tree/exceptions.hxx>
#include <xsd/cxx/tree/elements.hxx>
#include <xsd/cxx/tree/containers.hxx>
#include <xsd/cxx/tree/list.hxx>

#include <xsd/cxx/xml/dom/parsing-header.hxx>

#include <xsd/cxx/tree/containers-wildcard.hxx>

namespace am
{
  class UUID: public ::xml_schema::Uri
  {
    public:
    // Constructors.
    //
    UUID (const ::xml_schema::Uri&);

    UUID (const ::xercesc::DOMElement& e,
          ::xml_schema::Flags f = 0,
          ::xml_schema::Container* c = 0);

    UUID (const ::xercesc::DOMAttr& a,
          ::xml_schema::Flags f = 0,
          ::xml_schema::Container* c = 0);

    UUID (const ::std::string& s,
          const ::xercesc::DOMElement* e,
          ::xml_schema::Flags f = 0,
          ::xml_schema::Container* c = 0);

    UUID (const UUID& x,
          ::xml_schema::Flags f = 0,
          ::xml_schema::Container* c = 0);

    virtual UUID*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    virtual 
    ~UUID ();
  };

  class UserText: public ::xml_schema::String
  {
    public:
    // language
    //
    typedef ::xml_schema::Language LanguageType;
    typedef ::xsd::cxx::tree::traits< LanguageType, char > LanguageTraits;

    const LanguageType&
    getLanguage () const;

    LanguageType&
    getLanguage ();

    void
    setLanguage (const LanguageType& x);

    void
    setLanguage (::std::unique_ptr< LanguageType > p);

    static const LanguageType&
    getLanguageDefaultValue ();

    // Constructors.
    //
    UserText ();

    UserText (const char*);

    UserText (const ::std::string&);

    UserText (const ::xml_schema::String&);

    UserText (const ::xercesc::DOMElement& e,
              ::xml_schema::Flags f = 0,
              ::xml_schema::Container* c = 0);

    UserText (const UserText& x,
              ::xml_schema::Flags f = 0,
              ::xml_schema::Container* c = 0);

    virtual UserText*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    UserText&
    operator= (const UserText& x);

    virtual 
    ~UserText ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ::xsd::cxx::tree::one< LanguageType > language_;
    static const LanguageType language_default_value_;
  };

  class ChunkType: public ::xml_schema::Type
  {
    public:
    // Path
    //
    typedef ::xml_schema::Uri PathType;
    typedef ::xsd::cxx::tree::traits< PathType, char > PathTraits;

    const PathType&
    getPath () const;

    PathType&
    getPath ();

    void
    setPath (const PathType& x);

    void
    setPath (::std::unique_ptr< PathType > p);

    // VolumeIndex
    //
    typedef ::xml_schema::PositiveInteger VolumeIndexType;
    typedef ::xsd::cxx::tree::optional< VolumeIndexType > VolumeIndexOptional;
    typedef ::xsd::cxx::tree::traits< VolumeIndexType, char > VolumeIndexTraits;

    const VolumeIndexOptional&
    getVolumeIndex () const;

    VolumeIndexOptional&
    getVolumeIndex ();

    void
    setVolumeIndex (const VolumeIndexType& x);

    void
    setVolumeIndex (const VolumeIndexOptional& x);

    // Offset
    //
    typedef ::xml_schema::NonNegativeInteger OffsetType;
    typedef ::xsd::cxx::tree::optional< OffsetType > OffsetOptional;
    typedef ::xsd::cxx::tree::traits< OffsetType, char > OffsetTraits;

    const OffsetOptional&
    getOffset () const;

    OffsetOptional&
    getOffset ();

    void
    setOffset (const OffsetType& x);

    void
    setOffset (const OffsetOptional& x);

    // Length
    //
    typedef ::xml_schema::PositiveInteger LengthType;
    typedef ::xsd::cxx::tree::optional< LengthType > LengthOptional;
    typedef ::xsd::cxx::tree::traits< LengthType, char > LengthTraits;

    const LengthOptional&
    getLength () const;

    LengthOptional&
    getLength ();

    void
    setLength (const LengthType& x);

    void
    setLength (const LengthOptional& x);

    // Constructors.
    //
    ChunkType (const PathType&);

    ChunkType (const ::xercesc::DOMElement& e,
               ::xml_schema::Flags f = 0,
               ::xml_schema::Container* c = 0);

    ChunkType (const ChunkType& x,
               ::xml_schema::Flags f = 0,
               ::xml_schema::Container* c = 0);

    virtual ChunkType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    ChunkType&
    operator= (const ChunkType& x);

    virtual 
    ~ChunkType ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ::xsd::cxx::tree::one< PathType > Path_;
    VolumeIndexOptional VolumeIndex_;
    OffsetOptional Offset_;
    LengthOptional Length_;
  };

  class AssetType: public ::xml_schema::Type
  {
    public:
    // Id
    //
    typedef ::am::UUID IdType;
    typedef ::xsd::cxx::tree::traits< IdType, char > IdTraits;

    const IdType&
    getId () const;

    IdType&
    getId ();

    void
    setId (const IdType& x);

    void
    setId (::std::unique_ptr< IdType > p);

    // AnnotationText
    //
    typedef ::am::UserText AnnotationTextType;
    typedef ::xsd::cxx::tree::optional< AnnotationTextType > AnnotationTextOptional;
    typedef ::xsd::cxx::tree::traits< AnnotationTextType, char > AnnotationTextTraits;

    const AnnotationTextOptional&
    getAnnotationText () const;

    AnnotationTextOptional&
    getAnnotationText ();

    void
    setAnnotationText (const AnnotationTextType& x);

    void
    setAnnotationText (const AnnotationTextOptional& x);

    void
    setAnnotationText (::std::unique_ptr< AnnotationTextType > p);

    // PackingList
    //
    typedef ::xml_schema::Boolean PackingListType;
    typedef ::xsd::cxx::tree::optional< PackingListType > PackingListOptional;
    typedef ::xsd::cxx::tree::traits< PackingListType, char > PackingListTraits;

    const PackingListOptional&
    getPackingList () const;

    PackingListOptional&
    getPackingList ();

    void
    setPackingList (const PackingListType& x);

    void
    setPackingList (const PackingListOptional& x);

    // ChunkList
    //
    typedef ::am::AssetType_ChunkListType ChunkListType;
    typedef ::xsd::cxx::tree::traits< ChunkListType, char > ChunkListTraits;

    const ChunkListType&
    getChunkList () const;

    ChunkListType&
    getChunkList ();

    void
    setChunkList (const ChunkListType& x);

    void
    setChunkList (::std::unique_ptr< ChunkListType > p);

    // Constructors.
    //
    AssetType (const IdType&,
               const ChunkListType&);

    AssetType (const IdType&,
               ::std::unique_ptr< ChunkListType >);

    AssetType (const ::xercesc::DOMElement& e,
               ::xml_schema::Flags f = 0,
               ::xml_schema::Container* c = 0);

    AssetType (const AssetType& x,
               ::xml_schema::Flags f = 0,
               ::xml_schema::Container* c = 0);

    virtual AssetType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    AssetType&
    operator= (const AssetType& x);

    virtual 
    ~AssetType ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ::xsd::cxx::tree::one< IdType > Id_;
    AnnotationTextOptional AnnotationText_;
    PackingListOptional PackingList_;
    ::xsd::cxx::tree::one< ChunkListType > ChunkList_;
  };

  class AssetMapType: public ::xml_schema::Type
  {
    public:
    // Id
    //
    typedef ::am::UUID IdType;
    typedef ::xsd::cxx::tree::traits< IdType, char > IdTraits;

    const IdType&
    getId () const;

    IdType&
    getId ();

    void
    setId (const IdType& x);

    void
    setId (::std::unique_ptr< IdType > p);

    // AnnotationText
    //
    typedef ::am::UserText AnnotationTextType;
    typedef ::xsd::cxx::tree::optional< AnnotationTextType > AnnotationTextOptional;
    typedef ::xsd::cxx::tree::traits< AnnotationTextType, char > AnnotationTextTraits;

    const AnnotationTextOptional&
    getAnnotationText () const;

    AnnotationTextOptional&
    getAnnotationText ();

    void
    setAnnotationText (const AnnotationTextType& x);

    void
    setAnnotationText (const AnnotationTextOptional& x);

    void
    setAnnotationText (::std::unique_ptr< AnnotationTextType > p);

    // Creator
    //
    typedef ::am::UserText CreatorType;
    typedef ::xsd::cxx::tree::traits< CreatorType, char > CreatorTraits;

    const CreatorType&
    getCreator () const;

    CreatorType&
    getCreator ();

    void
    setCreator (const CreatorType& x);

    void
    setCreator (::std::unique_ptr< CreatorType > p);

    // VolumeCount
    //
    typedef ::xml_schema::PositiveInteger VolumeCountType;
    typedef ::xsd::cxx::tree::traits< VolumeCountType, char > VolumeCountTraits;

    const VolumeCountType&
    getVolumeCount () const;

    VolumeCountType&
    getVolumeCount ();

    void
    setVolumeCount (const VolumeCountType& x);

    // IssueDate
    //
    typedef ::xml_schema::DateTime IssueDateType;
    typedef ::xsd::cxx::tree::traits< IssueDateType, char > IssueDateTraits;

    const IssueDateType&
    getIssueDate () const;

    IssueDateType&
    getIssueDate ();

    void
    setIssueDate (const IssueDateType& x);

    void
    setIssueDate (::std::unique_ptr< IssueDateType > p);

    // Issuer
    //
    typedef ::am::UserText IssuerType;
    typedef ::xsd::cxx::tree::traits< IssuerType, char > IssuerTraits;

    const IssuerType&
    getIssuer () const;

    IssuerType&
    getIssuer ();

    void
    setIssuer (const IssuerType& x);

    void
    setIssuer (::std::unique_ptr< IssuerType > p);

    // AssetList
    //
    typedef ::am::AssetMapType_AssetListType AssetListType;
    typedef ::xsd::cxx::tree::traits< AssetListType, char > AssetListTraits;

    const AssetListType&
    getAssetList () const;

    AssetListType&
    getAssetList ();

    void
    setAssetList (const AssetListType& x);

    void
    setAssetList (::std::unique_ptr< AssetListType > p);

    // Constructors.
    //
    AssetMapType (const IdType&,
                  const CreatorType&,
                  const VolumeCountType&,
                  const IssueDateType&,
                  const IssuerType&,
                  const AssetListType&);

    AssetMapType (const IdType&,
                  ::std::unique_ptr< CreatorType >,
                  const VolumeCountType&,
                  const IssueDateType&,
                  ::std::unique_ptr< IssuerType >,
                  ::std::unique_ptr< AssetListType >);

    AssetMapType (const ::xercesc::DOMElement& e,
                  ::xml_schema::Flags f = 0,
                  ::xml_schema::Container* c = 0);

    AssetMapType (const AssetMapType& x,
                  ::xml_schema::Flags f = 0,
                  ::xml_schema::Container* c = 0);

    virtual AssetMapType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    AssetMapType&
    operator= (const AssetMapType& x);

    virtual 
    ~AssetMapType ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ::xsd::cxx::tree::one< IdType > Id_;
    AnnotationTextOptional AnnotationText_;
    ::xsd::cxx::tree::one< CreatorType > Creator_;
    ::xsd::cxx::tree::one< VolumeCountType > VolumeCount_;
    ::xsd::cxx::tree::one< IssueDateType > IssueDate_;
    ::xsd::cxx::tree::one< IssuerType > Issuer_;
    ::xsd::cxx::tree::one< AssetListType > AssetList_;
  };

  class VolumeIndexType: public ::xml_schema::Type
  {
    public:
    // Index
    //
    typedef ::xml_schema::PositiveInteger IndexType;
    typedef ::xsd::cxx::tree::traits< IndexType, char > IndexTraits;

    const IndexType&
    getIndex () const;

    IndexType&
    getIndex ();

    void
    setIndex (const IndexType& x);

    // Constructors.
    //
    VolumeIndexType (const IndexType&);

    VolumeIndexType (const ::xercesc::DOMElement& e,
                     ::xml_schema::Flags f = 0,
                     ::xml_schema::Container* c = 0);

    VolumeIndexType (const VolumeIndexType& x,
                     ::xml_schema::Flags f = 0,
                     ::xml_schema::Container* c = 0);

    virtual VolumeIndexType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    VolumeIndexType&
    operator= (const VolumeIndexType& x);

    virtual 
    ~VolumeIndexType ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ::xsd::cxx::tree::one< IndexType > Index_;
  };

  class AssetType_ChunkListType: public ::xml_schema::Type
  {
    public:
    // Chunk
    //
    typedef ::am::ChunkType ChunkType;
    typedef ::xsd::cxx::tree::sequence< ChunkType > ChunkSequence;
    typedef ChunkSequence::iterator ChunkIterator;
    typedef ChunkSequence::const_iterator ChunkConstIterator;
    typedef ::xsd::cxx::tree::traits< ChunkType, char > ChunkTraits;

    const ChunkSequence&
    getChunk () const;

    ChunkSequence&
    getChunk ();

    void
    setChunk (const ChunkSequence& s);

    // Constructors.
    //
    AssetType_ChunkListType ();

    AssetType_ChunkListType (const ::xercesc::DOMElement& e,
                             ::xml_schema::Flags f = 0,
                             ::xml_schema::Container* c = 0);

    AssetType_ChunkListType (const AssetType_ChunkListType& x,
                             ::xml_schema::Flags f = 0,
                             ::xml_schema::Container* c = 0);

    virtual AssetType_ChunkListType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    AssetType_ChunkListType&
    operator= (const AssetType_ChunkListType& x);

    virtual 
    ~AssetType_ChunkListType ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ChunkSequence Chunk_;
  };

  class AssetMapType_AssetListType: public ::xml_schema::Type
  {
    public:
    // Asset
    //
    typedef ::am::AssetType AssetType;
    typedef ::xsd::cxx::tree::sequence< AssetType > AssetSequence;
    typedef AssetSequence::iterator AssetIterator;
    typedef AssetSequence::const_iterator AssetConstIterator;
    typedef ::xsd::cxx::tree::traits< AssetType, char > AssetTraits;

    const AssetSequence&
    getAsset () const;

    AssetSequence&
    getAsset ();

    void
    setAsset (const AssetSequence& s);

    // Constructors.
    //
    AssetMapType_AssetListType ();

    AssetMapType_AssetListType (const ::xercesc::DOMElement& e,
                                ::xml_schema::Flags f = 0,
                                ::xml_schema::Container* c = 0);

    AssetMapType_AssetListType (const AssetMapType_AssetListType& x,
                                ::xml_schema::Flags f = 0,
                                ::xml_schema::Container* c = 0);

    virtual AssetMapType_AssetListType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    AssetMapType_AssetListType&
    operator= (const AssetMapType_AssetListType& x);

    virtual 
    ~AssetMapType_AssetListType ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    AssetSequence Asset_;
  };
}

#include <iosfwd>

namespace am
{
  ::std::ostream&
  operator<< (::std::ostream&, const UUID&);

  ::std::ostream&
  operator<< (::std::ostream&, const UserText&);

  ::std::ostream&
  operator<< (::std::ostream&, const ChunkType&);

  ::std::ostream&
  operator<< (::std::ostream&, const AssetType&);

  ::std::ostream&
  operator<< (::std::ostream&, const AssetMapType&);

  ::std::ostream&
  operator<< (::std::ostream&, const VolumeIndexType&);

  ::std::ostream&
  operator<< (::std::ostream&, const AssetType_ChunkListType&);

  ::std::ostream&
  operator<< (::std::ostream&, const AssetMapType_AssetListType&);
}

#include <iosfwd>

#include <xercesc/sax/InputSource.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>

namespace am
{
  // Parse a URI or a local file.
  //

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (const ::std::string& uri,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (const ::std::string& uri,
                 ::xml_schema::ErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (const ::std::string& uri,
                 ::xercesc::DOMErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse std::istream.
  //

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::std::istream& is,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::std::istream& is,
                 ::xml_schema::ErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::std::istream& is,
                 ::xercesc::DOMErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::std::istream& is,
                 const ::std::string& id,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::std::istream& is,
                 const ::std::string& id,
                 ::xml_schema::ErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::std::istream& is,
                 const ::std::string& id,
                 ::xercesc::DOMErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse xercesc::InputSource.
  //

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::xercesc::InputSource& is,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::xercesc::InputSource& is,
                 ::xml_schema::ErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::xercesc::InputSource& is,
                 ::xercesc::DOMErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse xercesc::DOMDocument.
  //

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (const ::xercesc::DOMDocument& d,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::AssetMapType >
  parseAssetMap (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse a URI or a local file.
  //

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (const ::std::string& uri,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (const ::std::string& uri,
                    ::xml_schema::ErrorHandler& eh,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (const ::std::string& uri,
                    ::xercesc::DOMErrorHandler& eh,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse std::istream.
  //

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::std::istream& is,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::std::istream& is,
                    ::xml_schema::ErrorHandler& eh,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::std::istream& is,
                    ::xercesc::DOMErrorHandler& eh,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::std::istream& is,
                    const ::std::string& id,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::std::istream& is,
                    const ::std::string& id,
                    ::xml_schema::ErrorHandler& eh,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::std::istream& is,
                    const ::std::string& id,
                    ::xercesc::DOMErrorHandler& eh,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse xercesc::InputSource.
  //

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::xercesc::InputSource& is,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::xercesc::InputSource& is,
                    ::xml_schema::ErrorHandler& eh,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::xercesc::InputSource& is,
                    ::xercesc::DOMErrorHandler& eh,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse xercesc::DOMDocument.
  //

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (const ::xercesc::DOMDocument& d,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::am::VolumeIndexType >
  parseVolumeIndex (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
                    ::xml_schema::Flags f = 0,
                    const ::xml_schema::Properties& p = ::xml_schema::Properties ());
}

#include <iosfwd>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/framework/XMLFormatter.hpp>

#include <xsd/cxx/xml/dom/auto-ptr.hxx>

namespace am
{
  void
  operator<< (::xercesc::DOMElement&, const UUID&);

  void
  operator<< (::xercesc::DOMAttr&, const UUID&);

  void
  operator<< (::xml_schema::ListStream&,
              const UUID&);

  void
  operator<< (::xercesc::DOMElement&, const UserText&);

  void
  operator<< (::xercesc::DOMElement&, const ChunkType&);

  void
  operator<< (::xercesc::DOMElement&, const AssetType&);

  void
  operator<< (::xercesc::DOMElement&, const AssetMapType&);

  // Serialize to std::ostream.
  //

  void
  serializeAssetMap (::std::ostream& os,
                     const ::am::AssetMapType& x, 
                     const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                     const ::std::string& e = "UTF-8",
                     ::xml_schema::Flags f = 0);

  void
  serializeAssetMap (::std::ostream& os,
                     const ::am::AssetMapType& x, 
                     ::xml_schema::ErrorHandler& eh,
                     const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                     const ::std::string& e = "UTF-8",
                     ::xml_schema::Flags f = 0);

  void
  serializeAssetMap (::std::ostream& os,
                     const ::am::AssetMapType& x, 
                     ::xercesc::DOMErrorHandler& eh,
                     const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                     const ::std::string& e = "UTF-8",
                     ::xml_schema::Flags f = 0);

  // Serialize to xercesc::XMLFormatTarget.
  //

  void
  serializeAssetMap (::xercesc::XMLFormatTarget& ft,
                     const ::am::AssetMapType& x, 
                     const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                     const ::std::string& e = "UTF-8",
                     ::xml_schema::Flags f = 0);

  void
  serializeAssetMap (::xercesc::XMLFormatTarget& ft,
                     const ::am::AssetMapType& x, 
                     ::xml_schema::ErrorHandler& eh,
                     const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                     const ::std::string& e = "UTF-8",
                     ::xml_schema::Flags f = 0);

  void
  serializeAssetMap (::xercesc::XMLFormatTarget& ft,
                     const ::am::AssetMapType& x, 
                     ::xercesc::DOMErrorHandler& eh,
                     const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                     const ::std::string& e = "UTF-8",
                     ::xml_schema::Flags f = 0);

  // Serialize to an existing xercesc::DOMDocument.
  //

  void
  serializeAssetMap (::xercesc::DOMDocument& d,
                     const ::am::AssetMapType& x,
                     ::xml_schema::Flags f = 0);

  // Serialize to a new xercesc::DOMDocument.
  //

  ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
  serializeAssetMap (const ::am::AssetMapType& x, 
                     const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                     ::xml_schema::Flags f = 0);

  void
  operator<< (::xercesc::DOMElement&, const VolumeIndexType&);

  // Serialize to std::ostream.
  //

  void
  serializeVolumeIndex (::std::ostream& os,
                        const ::am::VolumeIndexType& x, 
                        const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                        const ::std::string& e = "UTF-8",
                        ::xml_schema::Flags f = 0);

  void
  serializeVolumeIndex (::std::ostream& os,
                        const ::am::VolumeIndexType& x, 
                        ::xml_schema::ErrorHandler& eh,
                        const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                        const ::std::string& e = "UTF-8",
                        ::xml_schema::Flags f = 0);

  void
  serializeVolumeIndex (::std::ostream& os,
                        const ::am::VolumeIndexType& x, 
                        ::xercesc::DOMErrorHandler& eh,
                        const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                        const ::std::string& e = "UTF-8",
                        ::xml_schema::Flags f = 0);

  // Serialize to xercesc::XMLFormatTarget.
  //

  void
  serializeVolumeIndex (::xercesc::XMLFormatTarget& ft,
                        const ::am::VolumeIndexType& x, 
                        const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                        const ::std::string& e = "UTF-8",
                        ::xml_schema::Flags f = 0);

  void
  serializeVolumeIndex (::xercesc::XMLFormatTarget& ft,
                        const ::am::VolumeIndexType& x, 
                        ::xml_schema::ErrorHandler& eh,
                        const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                        const ::std::string& e = "UTF-8",
                        ::xml_schema::Flags f = 0);

  void
  serializeVolumeIndex (::xercesc::XMLFormatTarget& ft,
                        const ::am::VolumeIndexType& x, 
                        ::xercesc::DOMErrorHandler& eh,
                        const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                        const ::std::string& e = "UTF-8",
                        ::xml_schema::Flags f = 0);

  // Serialize to an existing xercesc::DOMDocument.
  //

  void
  serializeVolumeIndex (::xercesc::DOMDocument& d,
                        const ::am::VolumeIndexType& x,
                        ::xml_schema::Flags f = 0);

  // Serialize to a new xercesc::DOMDocument.
  //

  ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
  serializeVolumeIndex (const ::am::VolumeIndexType& x, 
                        const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                        ::xml_schema::Flags f = 0);

  void
  operator<< (::xercesc::DOMElement&, const AssetType_ChunkListType&);

  void
  operator<< (::xercesc::DOMElement&, const AssetMapType_AssetListType&);
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // SMPTE_429_9_2007_AM_H
