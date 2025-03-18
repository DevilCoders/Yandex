#pragma once

#include "util.h"

namespace NHtml5 {
    /**
* Attribute namespaces.
* HTML includes special handling for XLink, XML, and XMLNS namespaces on
* attributes.  Everything else goes in the generic "NONE" namespace.
*/
    enum EAttributeNamespace {
        ATTR_NAMESPACE_NONE,
        ATTR_NAMESPACE_XLINK,
        ATTR_NAMESPACE_XML,
        ATTR_NAMESPACE_XMLNS,
    };

    /**
* A struct representing a single attribute on an HTML tag.  This is a
* name-value pair, but also includes information about source locations and
* original source text.
*/
    struct TAttribute {
        /**
    * The original text of the attribute name, as a pointer into the original
    * source buffer.
    */
        TStringPiece OriginalName;

        /**
    * The original text of the value of the attribute.  This points into the
    * original source buffer.  It includes any quotes that surround the
    * attribute, and you can look at original_value.data[0] and
    * original_value.data[original_value.length - 1] to determine what the quote
    * characters were.  If the attribute has no value, this will be equal to
    * OriginalName.
    */
        TStringPiece OriginalValue;

        /**
    * The namespace for the attribute.  This will usually be
    * ATTR_NAMESPACE_NONE, but some XLink/XMLNS/XML attributes take special
    * values, per:
    * http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#adjust-foreign-attributes
    */
        EAttributeNamespace AttrNamespace;
    };

}
