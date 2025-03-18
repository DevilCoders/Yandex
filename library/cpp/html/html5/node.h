#pragma once

#include "attribute.h"
#include "tag.h"
#include "vector.h"

namespace NHtml5 {
    struct TNode;

    /**
 * Enum denoting the type of node.  This determines the type of the node.v
 * union.
 */
    enum ENodeType {
        /** Document node.  v will be a TDocument. */
        NODE_DOCUMENT,
        /** Element node.  v will be a TElement. */
        NODE_ELEMENT,
        /** Text node.  v will be a TText. */
        NODE_TEXT,
        /** Comment node.  v. will be a TText, excluding comment delimiters. */
        NODE_COMMENT,
        /** Text node, where all contents is whitespace.  v will be a TText. */
        NODE_WHITESPACE
    };

    /** http://www.whatwg.org/specs/web-apps/current-work/complete/dom.html#quirks-mode */
    enum EQuirksMode {
        DOCTYPE_NO_QUIRKS,
        DOCTYPE_QUIRKS,
        DOCTYPE_LIMITED_QUIRKS
    };

    /**
 * Namespaces.
 * Unlike in X(HT)ML, namespaces in HTML5 are not denoted by a prefix.  Rather,
 * anything inside an <svg> tag is in the SVG namespace, anything inside the
 * <math> tag is in the MathML namespace, and anything else is inside the HTML
 * namespace.  No other namespaces are supported, so this can be an enum only.
 */
    enum ENamespace {
        NAMESPACE_HTML,
        NAMESPACE_SVG,
        NAMESPACE_MATHML
    };

    /**
 * Parse flags.
 * We track the reasons for parser insertion of nodes and store them in a
 * bitvector in the node itself.  This lets client code optimize out nodes that
 * are implied by the HTML structure of the document, or flag constructs that
 * may not be allowed by a style guide, or track the prevalence of incorrect or
 * tricky HTML code.
 */
    enum EParseFlags {
        /**
     * A normal node - both start and end tags appear in the source, nothing has
     * been reparented.
     */
        INSERTION_NORMAL = 0,

        /**
     * A node inserted by the parser to fulfill some implicit insertion rule.
     * This is usually set in addition to some other flag giving a more specific
     * insertion reason; it's a generic catch-all term meaning "The start tag for
     * this node did not appear in the document source".
     */
        INSERTION_BY_PARSER = 1 << 0,

        /**
     * A flag indicating that the end tag for this node did not appear in the
     * document source.  Note that in some cases, you can still have
     * parser-inserted nodes with an explicit end tag: for example, "Text</html>"
     * has INSERTED_BY_PARSER set on the <html> node, but
     * INSERTED_END_TAG_IMPLICITLY is unset, as the </html> tag actually
     * exists.  This flag will be set only if the end tag is completely missing;
     * in some cases, the end tag may be misplaced (eg. a </body> tag with text
     * afterwards), which will leave this flag unset and require clients to
     * inspect the parse errors for that case.
     */
        INSERTION_IMPLICIT_END_TAG = 1 << 1,

        /**
     * A flag for <html> or <body> nodes that are first implicit inserted by parser
     * but when merged with existing tag.
     */
        INSERTION_MERGED_ATTRIBUTES = 1 << 2,

        /**
     * A flag for nodes that are inserted because their presence is implied by
     * other tags, eg. <html>, <head>, <body>, <tbody>, etc.
     */
        INSERTION_IMPLIED = 1 << 3,

        /**
     * A flag for nodes that are converted from their end tag equivalents.  For
     * example, </p> when no paragraph is open implies that the parser should
     * create a <p> tag and immediately close it, while </br> means the same thing
     * as <br>.
     */
        INSERTION_CONVERTED_FROM_END_TAG = 1 << 4,

        /** A flag for nodes that are converted from the parse of an <isindex> tag. */
        INSERTION_FROM_ISINDEX = 1 << 5,

        /** A flag for <image> tags that are rewritten as <img>. */
        INSERTION_FROM_IMAGE = 1 << 6,

        /**
     * A flag for nodes that are cloned as a result of the reconstruction of
     * active formatting elements.  This is set only on the clone; the initial
     * portion of the formatting run is a NORMAL node with an IMPLICIT_END_TAG.
     */
        INSERTION_RECONSTRUCTED_FORMATTING_ELEMENT = 1 << 7,

        /** A flag for nodes that are cloned by the adoption agency algorithm. */
        INSERTION_ADOPTION_AGENCY_CLONED = 1 << 8,

        /** A flag for nodes that are moved by the adoption agency algorithm. */
        INSERTION_ADOPTION_AGENCY_MOVED = 1 << 9,

        /**
     * A flag for nodes that have been foster-parented out of a table (or
     * should've been foster-parented, if verbatim mode is set).
     */
        INSERTION_FOSTER_PARENTED = 1 << 10,
    };

    /**
 * Information specific to document nodes.
 */
    struct TDocument {
        /**
     * An array of GumboNodes, containing the children of this element.  This will
     * normally consist of the <html> element and any comment nodes found.
     * Pointers are owned.
     */
        TVectorType<TNode*> Children;

        /**
     * The original text of this node, as a pointer into the original buffer.
     */
        TStringPiece OriginalText;

        // Fields from the doctype token, copied verbatim.
        TStringPiece Name;
        TStringPiece PublicIdentifier;
        TStringPiece SystemIdentifier;

        /**
     * Whether or not the document is in QuirksMode, as determined by the values
     * in the GumboTokenDocType template.
     */
        EQuirksMode DocTypeQuirksMode;

        /** True if there was an explicit doctype token as opposed to it being omitted. */
        bool HasDoctype;
    };

    /**
 * The struct used to represent TEXT, CDATA, COMMENT, and WHITESPACE elements.
 * This contains just a block of text and its position.
 */
    struct TText {
        /**
     * The text of this node, after entities have been parsed and decoded.  For
     * comment/cdata nodes, this does not include the comment delimiters.
     */
        TStringPiece Text;

        /**
     * The original text of this node, as a pointer into the original buffer.  For
     * comment/cdata nodes, this includes the comment delimiters.
     */
        TStringPiece OriginalText;
    };

    /**
 * The struct used to represent all HTML elements.  This contains information
 * about the tag, attributes, and child nodes.
 */
    struct TElement {
        /**
     * An array of GumboNodes, containing the children of this element.  Pointers
     * are owned.
     */
        TVectorType<TNode*> Children;

        /** The GumboTag enum for this element. */
        ETag Tag;

        /** The GumboNamespaceEnum for this element. */
        ENamespace TagNamespace;

        /**
     * A GumboStringPiece pointing to the original tag text for this element,
     * pointing directly into the source buffer.  If the tag was inserted
     * algorithmically (for example, <head> or <tbody> insertion), this will be a
     * zero-length string.
     */
        TStringPiece OriginalTag;

        /**
     * A GumboStringPiece pointing to the original end tag text for this element.
     * If the end tag was inserted algorithmically, (for example, closing a
     * self-closing tag), this will be a zero-length string.
     */
        TStringPiece OriginalEndTag;

        /**
     * An array of GumboAttributes, containing the attributes for this tag in the
     * order that they were parsed.  Pointers are owned.
     */
        TVectorType<TAttribute> Attributes;
    };

    /**
 * A supertype for GumboElement and GumboText, so that we can include one
 * generic type in lists of children and cast as necessary to subtypes.
 */
    struct TNode {
        /** The type of node that this is. */
        ENodeType Type;

        /**
    * A bitvector of flags containing information about why this element was
    * inserted into the parse tree, including a variety of special parse
    * situations.
    */
        EParseFlags ParseFlags;

        /** Pointer back to parent node.  Not owned. */
        TNode* Parent;

        /** The index within the parent's children vector of this node. */
        size_t IndexWithinParent;

        /** The actual node data. */
        union {
            TDocument Document; // For NODE_DOCUMENT.
            TElement Element;   // For NODE_ELEMENT.
            TText Text;         // For everything else.
        };
    };

}
