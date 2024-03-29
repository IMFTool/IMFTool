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

#ifndef LABELS_REGISTER_H
#define LABELS_REGISTER_H

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
namespace lr
{
  class LabelEntry;
  class LabelsRegister;
  class LabelEntry_KindType;
  class LabelsRegister_EntriesType;
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

namespace lr
{
  class LabelEntry: public ::xml_schema::Type
  {
    public:
    // Register
    //
    typedef ::xml_schema::String RegisterType;
    typedef ::xsd::cxx::tree::optional< RegisterType > RegisterOptional;
    typedef ::xsd::cxx::tree::traits< RegisterType, char > RegisterTraits;

    const RegisterOptional&
    getRegister () const;

    RegisterOptional&
    getRegister ();

    void
    setRegister (const RegisterType& x);

    void
    setRegister (const RegisterOptional& x);

    void
    setRegister (::std::unique_ptr< RegisterType > p);

    // NamespaceName
    //
    typedef ::xml_schema::String NamespaceNameType;
    typedef ::xsd::cxx::tree::traits< NamespaceNameType, char > NamespaceNameTraits;

    const NamespaceNameType&
    getNamespaceName () const;

    NamespaceNameType&
    getNamespaceName ();

    void
    setNamespaceName (const NamespaceNameType& x);

    void
    setNamespaceName (::std::unique_ptr< NamespaceNameType > p);

    // Symbol
    //
    typedef ::xml_schema::String SymbolType;
    typedef ::xsd::cxx::tree::traits< SymbolType, char > SymbolTraits;

    const SymbolType&
    getSymbol () const;

    SymbolType&
    getSymbol ();

    void
    setSymbol (const SymbolType& x);

    void
    setSymbol (::std::unique_ptr< SymbolType > p);

    // UL
    //
    typedef ::xml_schema::String ULType;
    typedef ::xsd::cxx::tree::traits< ULType, char > ULTraits;

    const ULType&
    getUL () const;

    ULType&
    getUL ();

    void
    setUL (const ULType& x);

    void
    setUL (::std::unique_ptr< ULType > p);

    // Kind
    //
    typedef ::lr::LabelEntry_KindType KindType;
    typedef ::xsd::cxx::tree::optional< KindType > KindOptional;
    typedef ::xsd::cxx::tree::traits< KindType, char > KindTraits;

    const KindOptional&
    getKind () const;

    KindOptional&
    getKind ();

    void
    setKind (const KindType& x);

    void
    setKind (const KindOptional& x);

    void
    setKind (::std::unique_ptr< KindType > p);

    // Name
    //
    typedef ::xml_schema::String NameType;
    typedef ::xsd::cxx::tree::optional< NameType > NameOptional;
    typedef ::xsd::cxx::tree::traits< NameType, char > NameTraits;

    const NameOptional&
    getName () const;

    NameOptional&
    getName ();

    void
    setName (const NameType& x);

    void
    setName (const NameOptional& x);

    void
    setName (::std::unique_ptr< NameType > p);

    // Definition
    //
    typedef ::xml_schema::String DefinitionType;
    typedef ::xsd::cxx::tree::optional< DefinitionType > DefinitionOptional;
    typedef ::xsd::cxx::tree::traits< DefinitionType, char > DefinitionTraits;

    const DefinitionOptional&
    getDefinition () const;

    DefinitionOptional&
    getDefinition ();

    void
    setDefinition (const DefinitionType& x);

    void
    setDefinition (const DefinitionOptional& x);

    void
    setDefinition (::std::unique_ptr< DefinitionType > p);

    // Applications
    //
    typedef ::xml_schema::String ApplicationsType;
    typedef ::xsd::cxx::tree::optional< ApplicationsType > ApplicationsOptional;
    typedef ::xsd::cxx::tree::traits< ApplicationsType, char > ApplicationsTraits;

    const ApplicationsOptional&
    getApplications () const;

    ApplicationsOptional&
    getApplications ();

    void
    setApplications (const ApplicationsType& x);

    void
    setApplications (const ApplicationsOptional& x);

    void
    setApplications (::std::unique_ptr< ApplicationsType > p);

    // Notes
    //
    typedef ::xml_schema::String NotesType;
    typedef ::xsd::cxx::tree::optional< NotesType > NotesOptional;
    typedef ::xsd::cxx::tree::traits< NotesType, char > NotesTraits;

    const NotesOptional&
    getNotes () const;

    NotesOptional&
    getNotes ();

    void
    setNotes (const NotesType& x);

    void
    setNotes (const NotesOptional& x);

    void
    setNotes (::std::unique_ptr< NotesType > p);

    // DefiningDocument
    //
    typedef ::xml_schema::String DefiningDocumentType;
    typedef ::xsd::cxx::tree::optional< DefiningDocumentType > DefiningDocumentOptional;
    typedef ::xsd::cxx::tree::traits< DefiningDocumentType, char > DefiningDocumentTraits;

    const DefiningDocumentOptional&
    getDefiningDocument () const;

    DefiningDocumentOptional&
    getDefiningDocument ();

    void
    setDefiningDocument (const DefiningDocumentType& x);

    void
    setDefiningDocument (const DefiningDocumentOptional& x);

    void
    setDefiningDocument (::std::unique_ptr< DefiningDocumentType > p);

    // IsDeprecated
    //
    typedef ::xml_schema::Boolean IsDeprecatedType;
    typedef ::xsd::cxx::tree::traits< IsDeprecatedType, char > IsDeprecatedTraits;

    const IsDeprecatedType&
    getIsDeprecated () const;

    IsDeprecatedType&
    getIsDeprecated ();

    void
    setIsDeprecated (const IsDeprecatedType& x);

    // Constructors.
    //
    LabelEntry (const NamespaceNameType&,
                const SymbolType&,
                const ULType&,
                const IsDeprecatedType&);

    LabelEntry (const ::xercesc::DOMElement& e,
                ::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0);

    LabelEntry (const LabelEntry& x,
                ::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0);

    virtual LabelEntry*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    LabelEntry&
    operator= (const LabelEntry& x);

    virtual 
    ~LabelEntry ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    RegisterOptional Register_;
    ::xsd::cxx::tree::one< NamespaceNameType > NamespaceName_;
    ::xsd::cxx::tree::one< SymbolType > Symbol_;
    ::xsd::cxx::tree::one< ULType > UL_;
    KindOptional Kind_;
    NameOptional Name_;
    DefinitionOptional Definition_;
    ApplicationsOptional Applications_;
    NotesOptional Notes_;
    DefiningDocumentOptional DefiningDocument_;
    ::xsd::cxx::tree::one< IsDeprecatedType > IsDeprecated_;
  };

  class LabelsRegister: public ::xml_schema::Type
  {
    public:
    // Entries
    //
    typedef ::lr::LabelsRegister_EntriesType EntriesType;
    typedef ::xsd::cxx::tree::optional< EntriesType > EntriesOptional;
    typedef ::xsd::cxx::tree::traits< EntriesType, char > EntriesTraits;

    const EntriesOptional&
    getEntries () const;

    EntriesOptional&
    getEntries ();

    void
    setEntries (const EntriesType& x);

    void
    setEntries (const EntriesOptional& x);

    void
    setEntries (::std::unique_ptr< EntriesType > p);

    // Constructors.
    //
    LabelsRegister ();

    LabelsRegister (const ::xercesc::DOMElement& e,
                    ::xml_schema::Flags f = 0,
                    ::xml_schema::Container* c = 0);

    LabelsRegister (const LabelsRegister& x,
                    ::xml_schema::Flags f = 0,
                    ::xml_schema::Container* c = 0);

    virtual LabelsRegister*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    LabelsRegister&
    operator= (const LabelsRegister& x);

    virtual 
    ~LabelsRegister ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    EntriesOptional Entries_;
  };

  class LabelEntry_KindType: public ::xml_schema::String
  {
    public:
    enum Value
    {
      NODE,
      LEAF
    };

    LabelEntry_KindType (Value v);

    LabelEntry_KindType (const char* v);

    LabelEntry_KindType (const ::std::string& v);

    LabelEntry_KindType (const ::xml_schema::String& v);

    LabelEntry_KindType (const ::xercesc::DOMElement& e,
                         ::xml_schema::Flags f = 0,
                         ::xml_schema::Container* c = 0);

    LabelEntry_KindType (const ::xercesc::DOMAttr& a,
                         ::xml_schema::Flags f = 0,
                         ::xml_schema::Container* c = 0);

    LabelEntry_KindType (const ::std::string& s,
                         const ::xercesc::DOMElement* e,
                         ::xml_schema::Flags f = 0,
                         ::xml_schema::Container* c = 0);

    LabelEntry_KindType (const LabelEntry_KindType& x,
                         ::xml_schema::Flags f = 0,
                         ::xml_schema::Container* c = 0);

    virtual LabelEntry_KindType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    LabelEntry_KindType&
    operator= (Value v);

    virtual
    operator Value () const
    {
      return _xsd_LabelEntry_KindType_convert ();
    }

    protected:
    Value
    _xsd_LabelEntry_KindType_convert () const;

    public:
    static const char* const _xsd_LabelEntry_KindType_literals_[2];
    static const Value _xsd_LabelEntry_KindType_indexes_[2];
  };

  class LabelsRegister_EntriesType: public ::xml_schema::Type
  {
    public:
    // Entry
    //
    typedef ::lr::LabelEntry EntryType;
    typedef ::xsd::cxx::tree::sequence< EntryType > EntrySequence;
    typedef EntrySequence::iterator EntryIterator;
    typedef EntrySequence::const_iterator EntryConstIterator;
    typedef ::xsd::cxx::tree::traits< EntryType, char > EntryTraits;

    const EntrySequence&
    getEntry () const;

    EntrySequence&
    getEntry ();

    void
    setEntry (const EntrySequence& s);

    // Constructors.
    //
    LabelsRegister_EntriesType ();

    LabelsRegister_EntriesType (const ::xercesc::DOMElement& e,
                                ::xml_schema::Flags f = 0,
                                ::xml_schema::Container* c = 0);

    LabelsRegister_EntriesType (const LabelsRegister_EntriesType& x,
                                ::xml_schema::Flags f = 0,
                                ::xml_schema::Container* c = 0);

    virtual LabelsRegister_EntriesType*
    _clone (::xml_schema::Flags f = 0,
            ::xml_schema::Container* c = 0) const;

    LabelsRegister_EntriesType&
    operator= (const LabelsRegister_EntriesType& x);

    virtual 
    ~LabelsRegister_EntriesType ();

    // Implementation.
    //
    protected:
    void
    parse (::xsd::cxx::xml::dom::parser< char >&,
           ::xml_schema::Flags);

    protected:
    EntrySequence Entry_;
  };
}

#include <iosfwd>

namespace lr
{
  ::std::ostream&
  operator<< (::std::ostream&, const LabelEntry&);

  ::std::ostream&
  operator<< (::std::ostream&, const LabelsRegister&);

  ::std::ostream&
  operator<< (::std::ostream&, LabelEntry_KindType::Value);

  ::std::ostream&
  operator<< (::std::ostream&, const LabelEntry_KindType&);

  ::std::ostream&
  operator<< (::std::ostream&, const LabelsRegister_EntriesType&);
}

#include <iosfwd>

#include <xercesc/sax/InputSource.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>

namespace lr
{
  // Parse a URI or a local file.
  //

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (const ::std::string& uri,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (const ::std::string& uri,
                       ::xml_schema::ErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (const ::std::string& uri,
                       ::xercesc::DOMErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse std::istream.
  //

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::std::istream& is,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::std::istream& is,
                       ::xml_schema::ErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::std::istream& is,
                       ::xercesc::DOMErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::std::istream& is,
                       const ::std::string& id,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::std::istream& is,
                       const ::std::string& id,
                       ::xml_schema::ErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::std::istream& is,
                       const ::std::string& id,
                       ::xercesc::DOMErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse xercesc::InputSource.
  //

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::xercesc::InputSource& is,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::xercesc::InputSource& is,
                       ::xml_schema::ErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::xercesc::InputSource& is,
                       ::xercesc::DOMErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  // Parse xercesc::DOMDocument.
  //

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (const ::xercesc::DOMDocument& d,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

  ::std::unique_ptr< ::lr::LabelsRegister >
  parseLabelsRegister (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());
}

#include <iosfwd>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/framework/XMLFormatter.hpp>

#include <xsd/cxx/xml/dom/auto-ptr.hxx>

namespace lr
{
  // Serialize to std::ostream.
  //

  void
  serializeLabelsRegister (::std::ostream& os,
                           const ::lr::LabelsRegister& x, 
                           const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                           const ::std::string& e = "UTF-8",
                           ::xml_schema::Flags f = 0);

  void
  serializeLabelsRegister (::std::ostream& os,
                           const ::lr::LabelsRegister& x, 
                           ::xml_schema::ErrorHandler& eh,
                           const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                           const ::std::string& e = "UTF-8",
                           ::xml_schema::Flags f = 0);

  void
  serializeLabelsRegister (::std::ostream& os,
                           const ::lr::LabelsRegister& x, 
                           ::xercesc::DOMErrorHandler& eh,
                           const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                           const ::std::string& e = "UTF-8",
                           ::xml_schema::Flags f = 0);

  // Serialize to xercesc::XMLFormatTarget.
  //

  void
  serializeLabelsRegister (::xercesc::XMLFormatTarget& ft,
                           const ::lr::LabelsRegister& x, 
                           const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                           const ::std::string& e = "UTF-8",
                           ::xml_schema::Flags f = 0);

  void
  serializeLabelsRegister (::xercesc::XMLFormatTarget& ft,
                           const ::lr::LabelsRegister& x, 
                           ::xml_schema::ErrorHandler& eh,
                           const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                           const ::std::string& e = "UTF-8",
                           ::xml_schema::Flags f = 0);

  void
  serializeLabelsRegister (::xercesc::XMLFormatTarget& ft,
                           const ::lr::LabelsRegister& x, 
                           ::xercesc::DOMErrorHandler& eh,
                           const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                           const ::std::string& e = "UTF-8",
                           ::xml_schema::Flags f = 0);

  // Serialize to an existing xercesc::DOMDocument.
  //

  void
  serializeLabelsRegister (::xercesc::DOMDocument& d,
                           const ::lr::LabelsRegister& x,
                           ::xml_schema::Flags f = 0);

  // Serialize to a new xercesc::DOMDocument.
  //

  ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
  serializeLabelsRegister (const ::lr::LabelsRegister& x, 
                           const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                           ::xml_schema::Flags f = 0);

  void
  operator<< (::xercesc::DOMElement&, const LabelEntry&);

  void
  operator<< (::xercesc::DOMElement&, const LabelsRegister&);

  void
  operator<< (::xercesc::DOMElement&, const LabelEntry_KindType&);

  void
  operator<< (::xercesc::DOMAttr&, const LabelEntry_KindType&);

  void
  operator<< (::xml_schema::ListStream&,
              const LabelEntry_KindType&);

  void
  operator<< (::xercesc::DOMElement&, const LabelsRegister_EntriesType&);
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // LABELS_REGISTER_H
