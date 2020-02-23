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

#ifndef ST2067_2A_2016_CORE_H
#define ST2067_2A_2016_CORE_H

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
namespace cc2016
{
  class StereoImageTrackFileResourceType;
  class ApplicationIdentification_base;
  class ApplicationIdentification;
  class CDPSequence;
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

#include "st2067-3a-2016-CPL.h"

#include "st0433-dcmlTypes.h"

namespace cc2016
{
  class StereoImageTrackFileResourceType: public ::cpl2016::BaseResourceType
  {
    public:
    // LeftEye
    //
    typedef ::cpl2016::TrackFileResourceType LeftEyeType;
    typedef ::xsd::cxx::tree::traits< LeftEyeType, char > LeftEyeTraits;

    const LeftEyeType&
    getLeftEye () const;

    LeftEyeType&
    getLeftEye ();

    void
    setLeftEye (const LeftEyeType& x);

    void
    setLeftEye (::std::unique_ptr< LeftEyeType > p);

    // RightEye
    //
    typedef ::cpl2016::TrackFileResourceType RightEyeType;
    typedef ::xsd::cxx::tree::traits< RightEyeType, char > RightEyeTraits;

    const RightEyeType&
    getRightEye () const;

    RightEyeType&
    getRightEye ();

    void
    setRightEye (const RightEyeType& x);

    void
    setRightEye (::std::unique_ptr< RightEyeType > p);

    // Constructors.
    //
    StereoImageTrackFileResourceType (const IdType&,
                                      const IntrinsicDurationType&,
                                      const LeftEyeType&,
                                      const RightEyeType&);

    StereoImageTrackFileResourceType (const IdType&,
                                      const IntrinsicDurationType&,
                                      ::std::unique_ptr< LeftEyeType >,
                                      ::std::unique_ptr< RightEyeType >);

    StereoImageTrackFileResourceType (const ::xercesc::DOMElement& e,
                                      ::xml_schema::Flags f = 0,
                                      ::xml_schema::Container* c = 0);

    StereoImageTrackFileResourceType (const StereoImageTrackFileResourceType& x,
                                      ::xml_schema::Flags f = 0,
                                      ::xml_schema::Container* c = 0);

    virtual StereoImageTrackFileResourceType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    StereoImageTrackFileResourceType&
    operator= (const StereoImageTrackFileResourceType& x);

    virtual 
    ~StereoImageTrackFileResourceType ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ::xsd::cxx::tree::one< LeftEyeType > LeftEye_;
    ::xsd::cxx::tree::one< RightEyeType > RightEye_;
  };

  class ApplicationIdentification_base: public ::xml_schema::SimpleType,
    public ::xsd::cxx::tree::list< ::xml_schema::Uri, char >
  {
    public:
    ApplicationIdentification_base ();

    ApplicationIdentification_base (size_type n, const ::xml_schema::Uri& x);

    template < typename I >
    ApplicationIdentification_base (const I& begin, const I& end)
    : ::xsd::cxx::tree::list< ::xml_schema::Uri, char > (begin, end, this)
    {
    }

    ApplicationIdentification_base (const ::xercesc::DOMElement& e,
                                    ::xml_schema::Flags f = 0,
                                    ::xml_schema::Container* c = 0);

    ApplicationIdentification_base (const ::xercesc::DOMAttr& a,
                                    ::xml_schema::Flags f = 0,
                                    ::xml_schema::Container* c = 0);

    ApplicationIdentification_base (const ::std::string& s,
                                    const ::xercesc::DOMElement* e,
                                    ::xml_schema::Flags f = 0,
                                    ::xml_schema::Container* c = 0);

    ApplicationIdentification_base (const ApplicationIdentification_base& x,
                                    ::xml_schema::Flags f = 0,
                                    ::xml_schema::Container* c = 0);

    virtual ApplicationIdentification_base*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    virtual 
    ~ApplicationIdentification_base ();
  };

  class ApplicationIdentification: public ::cc2016::ApplicationIdentification_base
  {
    public:
    // Constructors.
    //
    ApplicationIdentification ();

    ApplicationIdentification (const ::cc2016::ApplicationIdentification_base&);

    ApplicationIdentification (const ::xercesc::DOMElement& e,
                               ::xml_schema::Flags f = 0,
                               ::xml_schema::Container* c = 0);

    ApplicationIdentification (const ::xercesc::DOMAttr& a,
                               ::xml_schema::Flags f = 0,
                               ::xml_schema::Container* c = 0);

    ApplicationIdentification (const ::std::string& s,
                               const ::xercesc::DOMElement* e,
                               ::xml_schema::Flags f = 0,
                               ::xml_schema::Container* c = 0);

    ApplicationIdentification (const ApplicationIdentification& x,
                               ::xml_schema::Flags f = 0,
                               ::xml_schema::Container* c = 0);

    virtual ApplicationIdentification*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    virtual 
    ~ApplicationIdentification ();
  };

  class CDPSequence: public ::cpl2016::SequenceType
  {
    public:
    // ParentTrackID
    //
    typedef ::dcml::UUIDType ParentTrackIDType;
    typedef ::xsd::cxx::tree::traits< ParentTrackIDType, char > ParentTrackIDTraits;

    const ParentTrackIDType&
    getParentTrackID () const;

    ParentTrackIDType&
    getParentTrackID ();

    void
    setParentTrackID (const ParentTrackIDType& x);

    void
    setParentTrackID (::std::unique_ptr< ParentTrackIDType > p);

    // Constructors.
    //
    CDPSequence (const IdType&,
                 const TrackIdType&,
                 const ResourceListType&,
                 const ParentTrackIDType&);

    CDPSequence (const IdType&,
                 const TrackIdType&,
                 ::std::unique_ptr< ResourceListType >,
                 const ParentTrackIDType&);

    CDPSequence (const ::xercesc::DOMElement& e,
                 ::xml_schema::Flags f = 0,
                 ::xml_schema::Container* c = 0);

    CDPSequence (const CDPSequence& x,
                 ::xml_schema::Flags f = 0,
                 ::xml_schema::Container* c = 0);

    virtual CDPSequence*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    CDPSequence&
    operator= (const CDPSequence& x);

    virtual 
    ~CDPSequence ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    ::xsd::cxx::tree::one< ParentTrackIDType > ParentTrackID_;
  };
}

#include <iosfwd>

namespace cc2016
{
  ::std::ostream&
  operator<< (::std::ostream&, const StereoImageTrackFileResourceType&);

  ::std::ostream&
  operator<< (::std::ostream&, const ApplicationIdentification_base&);

  ::std::ostream&
  operator<< (::std::ostream&, const ApplicationIdentification&);

  ::std::ostream&
  operator<< (::std::ostream&, const CDPSequence&);
}

#include <iosfwd>

#include <xercesc/sax/InputSource.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>

namespace cc2016
{
}

#include <iosfwd>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/framework/XMLFormatter.hpp>

#include <xsd/cxx/xml/dom/auto-ptr.hxx>

namespace cc2016
{
  void
  operator<< (::xercesc::DOMElement&, const StereoImageTrackFileResourceType&);

  void
  operator<< (::xercesc::DOMElement&, const ApplicationIdentification_base&);

  void
  operator<< (::xercesc::DOMAttr&, const ApplicationIdentification_base&);

  void
  operator<< (::xml_schema::ListStream&,
              const ApplicationIdentification_base&);

  void
  operator<< (::xercesc::DOMElement&, const ApplicationIdentification&);

  void
  operator<< (::xercesc::DOMAttr&, const ApplicationIdentification&);

  void
  operator<< (::xml_schema::ListStream&,
              const ApplicationIdentification&);

  void
  operator<< (::xercesc::DOMElement&, const CDPSequence&);
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // ST2067_2A_2016_CORE_H
