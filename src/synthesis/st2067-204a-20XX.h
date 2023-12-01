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

#ifndef ST2067_204A_20XX_H
#define ST2067_204A_20XX_H

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
namespace imfadm
{
  class ADMSoundfieldGroupSelectorType;
  class ADMAudioVirtualTrackParameterSet;
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

#include "st2067-3a-2020-CPL.h"

#include "st0433-dcmlTypes.h"

namespace imfadm
{
  class ADMSoundfieldGroupSelectorType: public ::xml_schema::Type
  {
    public:
    // ResourceId
    //
    typedef ::dcml::UUIDType ResourceIdType;
    typedef ::xsd::cxx::tree::traits< ResourceIdType, char > ResourceIdTraits;

    const ResourceIdType&
    getResourceId () const;

    ResourceIdType&
    getResourceId ();

    void
    setResourceId (const ResourceIdType& x);

    void
    setResourceId (::std::unique_ptr< ResourceIdType > p);

    // ADMSoundfieldGroupLinkID
    //
    typedef ::dcml::UUIDType ADMSoundfieldGroupLinkIDType;
    typedef ::xsd::cxx::tree::sequence< ADMSoundfieldGroupLinkIDType > ADMSoundfieldGroupLinkIDSequence;
    typedef ADMSoundfieldGroupLinkIDSequence::iterator ADMSoundfieldGroupLinkIDIterator;
    typedef ADMSoundfieldGroupLinkIDSequence::const_iterator ADMSoundfieldGroupLinkIDConstIterator;
    typedef ::xsd::cxx::tree::traits< ADMSoundfieldGroupLinkIDType, char > ADMSoundfieldGroupLinkIDTraits;

    const ADMSoundfieldGroupLinkIDSequence&
    getADMSoundfieldGroupLinkID () const;

    ADMSoundfieldGroupLinkIDSequence&
    getADMSoundfieldGroupLinkID ();

    void
    setADMSoundfieldGroupLinkID (const ADMSoundfieldGroupLinkIDSequence& s);

    // Constructors.
    //
    ADMSoundfieldGroupSelectorType (const ResourceIdType&);

    ADMSoundfieldGroupSelectorType (const ::xercesc::DOMElement& e,
                                    ::xml_schema::Flags f = 0,
                                    ::xml_schema::Container* c = 0);

    ADMSoundfieldGroupSelectorType (const ADMSoundfieldGroupSelectorType& x,
                                    ::xml_schema::Flags f = 0,
                                    ::xml_schema::Container* c = 0);

    virtual ADMSoundfieldGroupSelectorType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    ADMSoundfieldGroupSelectorType&
    operator= (const ADMSoundfieldGroupSelectorType& x);

    virtual 
    ~ADMSoundfieldGroupSelectorType ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ::xsd::cxx::tree::one< ResourceIdType > ResourceId_;
    ADMSoundfieldGroupLinkIDSequence ADMSoundfieldGroupLinkID_;
  };

  class ADMAudioVirtualTrackParameterSet: public ::xml_schema::Type
  {
    public:
    // Id
    //
    typedef ::dcml::UUIDType IdType;
    typedef ::xsd::cxx::tree::traits< IdType, char > IdTraits;

    const IdType&
    getId () const;

    IdType&
    getId ();

    void
    setId (const IdType& x);

    void
    setId (::std::unique_ptr< IdType > p);

    // TrackId
    //
    typedef ::dcml::UUIDType TrackIdType;
    typedef ::xsd::cxx::tree::traits< TrackIdType, char > TrackIdTraits;

    const TrackIdType&
    getTrackId () const;

    TrackIdType&
    getTrackId ();

    void
    setTrackId (const TrackIdType& x);

    void
    setTrackId (::std::unique_ptr< TrackIdType > p);

    // ADMOperationalMode
    //
    typedef ::xml_schema::Uri ADMOperationalModeType;
    typedef ::xsd::cxx::tree::traits< ADMOperationalModeType, char > ADMOperationalModeTraits;

    const ADMOperationalModeType&
    getADMOperationalMode () const;

    ADMOperationalModeType&
    getADMOperationalMode ();

    void
    setADMOperationalMode (const ADMOperationalModeType& x);

    void
    setADMOperationalMode (::std::unique_ptr< ADMOperationalModeType > p);

    // ADMSoundfieldGroupSelector
    //
    typedef ::imfadm::ADMSoundfieldGroupSelectorType ADMSoundfieldGroupSelectorType;
    typedef ::xsd::cxx::tree::sequence< ADMSoundfieldGroupSelectorType > ADMSoundfieldGroupSelectorSequence;
    typedef ADMSoundfieldGroupSelectorSequence::iterator ADMSoundfieldGroupSelectorIterator;
    typedef ADMSoundfieldGroupSelectorSequence::const_iterator ADMSoundfieldGroupSelectorConstIterator;
    typedef ::xsd::cxx::tree::traits< ADMSoundfieldGroupSelectorType, char > ADMSoundfieldGroupSelectorTraits;

    const ADMSoundfieldGroupSelectorSequence&
    getADMSoundfieldGroupSelector () const;

    ADMSoundfieldGroupSelectorSequence&
    getADMSoundfieldGroupSelector ();

    void
    setADMSoundfieldGroupSelector (const ADMSoundfieldGroupSelectorSequence& s);

    // any
    //
    typedef ::xsd::cxx::tree::element_sequence AnySequence;
    typedef AnySequence::iterator AnyIterator;
    typedef AnySequence::const_iterator AnyConstIterator;

    const AnySequence&
    getAny () const;

    AnySequence&
    getAny ();

    void
    setAny (const AnySequence& s);

    // DOMDocument for wildcard content.
    //
    const ::xercesc::DOMDocument&
    getDomDocument () const;

    ::xercesc::DOMDocument&
    getDomDocument ();

    // Constructors.
    //
    ADMAudioVirtualTrackParameterSet (const IdType&,
                                      const TrackIdType&,
                                      const ADMOperationalModeType&);

    ADMAudioVirtualTrackParameterSet (const ::xercesc::DOMElement& e,
                                      ::xml_schema::Flags f = 0,
                                      ::xml_schema::Container* c = 0);

    ADMAudioVirtualTrackParameterSet (const ADMAudioVirtualTrackParameterSet& x,
                                      ::xml_schema::Flags f = 0,
                                      ::xml_schema::Container* c = 0);

    virtual ADMAudioVirtualTrackParameterSet*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    ADMAudioVirtualTrackParameterSet&
    operator= (const ADMAudioVirtualTrackParameterSet& x);

    virtual 
    ~ADMAudioVirtualTrackParameterSet ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > dom_document_;

    ::xsd::cxx::tree::one< IdType > Id_;
    ::xsd::cxx::tree::one< TrackIdType > TrackId_;
    ::xsd::cxx::tree::one< ADMOperationalModeType > ADMOperationalMode_;
    ADMSoundfieldGroupSelectorSequence ADMSoundfieldGroupSelector_;
    AnySequence any_;
  };
}

#include <iosfwd>

namespace imfadm
{
  ::std::ostream&
  operator<< (::std::ostream&, const ADMSoundfieldGroupSelectorType&);

  ::std::ostream&
  operator<< (::std::ostream&, const ADMAudioVirtualTrackParameterSet&);
}

#include <iosfwd>

#include <xercesc/sax/InputSource.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>

namespace imfadm
{
}

#include <iosfwd>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/framework/XMLFormatter.hpp>

#include <xsd/cxx/xml/dom/auto-ptr.hxx>

namespace imfadm
{
  void
  operator<< (::xercesc::DOMElement&, const ADMSoundfieldGroupSelectorType&);

  void
  operator<< (::xercesc::DOMElement&, const ADMAudioVirtualTrackParameterSet&);
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // ST2067_204A_20XX_H