#pragma once

#include "util.h"
#include <stddef.h>

namespace NHtml5 {
    /**
 * An enum for all the tags defined in the HTML5 standard.  These correspond to
 * the tag names themselves.  Enum constants exist only for tags which appear in
 * the spec itself (or for tags with special handling in the SVG and MathML
 * namespaces); any other tags appear as GUMBO_TAG_UNKNOWN and the actual tag
 * name can be obtained through original_tag.
 *
 * This is mostly for API convenience, so that clients of this library don't
 * need to perform a strcasecmp to find the normalized tag name.  It also has
 * efficiency benefits, by letting the parser work with enums instead of
 * strings.
 */
    enum ETag {
        // Used for all tags that don't have special handling in HTML.
        TAG_UNKNOWN = 0,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/semantics.html#the-root-element
        TAG_HTML,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/semantics.html#document-metadata
        TAG_HEAD,
        TAG_TITLE,
        TAG_BASE,
        TAG_LINK,
        TAG_META,
        TAG_STYLE,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/scripting-1.html#scripting-1
        TAG_SCRIPT,
        TAG_NOSCRIPT,
        TAG_TEMPLATE,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/sections.html#sections
        TAG_BODY,
        TAG_ARTICLE,
        TAG_SECTION,
        TAG_NAV,
        TAG_ASIDE,
        TAG_H1,
        TAG_H2,
        TAG_H3,
        TAG_H4,
        TAG_H5,
        TAG_H6,
        TAG_HGROUP,
        TAG_HEADER,
        TAG_FOOTER,
        TAG_ADDRESS,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/grouping-content.html#grouping-content
        TAG_P,
        TAG_HR,
        TAG_PRE,
        TAG_BLOCKQUOTE,
        TAG_OL,
        TAG_UL,
        TAG_LI,
        TAG_DL,
        TAG_DT,
        TAG_DD,
        TAG_FIGURE,
        TAG_FIGCAPTION,
        TAG_MAIN,
        TAG_DIV,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/text-level-semantics.html#text-level-semantics
        TAG_A,
        TAG_EM,
        TAG_STRONG,
        TAG_SMALL,
        TAG_S,
        TAG_CITE,
        TAG_Q,
        TAG_DFN,
        TAG_ABBR,
        TAG_DATA,
        TAG_TIME,
        TAG_CODE,
        TAG_VAR,
        TAG_SAMP,
        TAG_KBD,
        TAG_SUB,
        TAG_SUP,
        TAG_I,
        TAG_B,
        TAG_U,
        TAG_MARK,
        TAG_RUBY,
        TAG_RT,
        TAG_RP,
        TAG_BDI,
        TAG_BDO,
        TAG_SPAN,
        TAG_BR,
        TAG_WBR,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/edits.html#edits
        TAG_INS,
        TAG_DEL,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/embedded-content-1.html#embedded-content-1
        TAG_IMAGE,
        TAG_IMG,
        TAG_IFRAME,
        TAG_EMBED,
        TAG_OBJECT,
        TAG_PARAM,
        TAG_VIDEO,
        TAG_AUDIO,
        TAG_SOURCE,
        TAG_TRACK,
        TAG_CANVAS,
        TAG_MAP,
        TAG_AREA,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-map-element.html#mathml
        TAG_MATH,
        TAG_MI,
        TAG_MO,
        TAG_MN,
        TAG_MS,
        TAG_MTEXT,
        TAG_MGLYPH,
        TAG_MALIGNMARK,
        TAG_ANNOTATION_XML,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-map-element.html#svg-0
        TAG_SVG,
        TAG_FOREIGNOBJECT,
        TAG_DESC,
        // SVG title tags will have GUMBO_TAG_TITLE as with HTML.
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/tabular-data.html#tabular-data
        TAG_TABLE,
        TAG_CAPTION,
        TAG_COLGROUP,
        TAG_COL,
        TAG_TBODY,
        TAG_THEAD,
        TAG_TFOOT,
        TAG_TR,
        TAG_TD,
        TAG_TH,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/forms.html#forms
        TAG_DIALOG,
        TAG_FORM,
        TAG_FIELDSET,
        TAG_LEGEND,
        TAG_LABEL,
        TAG_INPUT,
        TAG_BUTTON,
        TAG_SELECT,
        TAG_DATALIST,
        TAG_OPTGROUP,
        TAG_OPTION,
        TAG_TEXTAREA,
        TAG_KEYGEN,
        TAG_OUTPUT,
        TAG_PROGRESS,
        TAG_METER,
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/interactive-elements.html#interactive-elements
        TAG_DETAILS,
        TAG_SUMMARY,
        TAG_MENU,
        TAG_MENUITEM,
        // Non-conforming elements that nonetheless appear in the HTML5 spec.
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/obsolete.html#non-conforming-features
        TAG_APPLET,
        TAG_ACRONYM,
        TAG_BGSOUND,
        TAG_DIR,
        TAG_FRAME,
        TAG_FRAMESET,
        TAG_NOFRAMES,
        TAG_NOINDEX,
        TAG_ISINDEX,
        TAG_LISTING,
        TAG_XMP,
        TAG_NEXTID,
        TAG_NOEMBED,
        TAG_PLAINTEXT,
        TAG_RB,
        TAG_STRIKE,
        TAG_BASEFONT,
        TAG_BIG,
        TAG_BLINK,
        TAG_CENTER,
        TAG_FONT,
        TAG_MARQUEE,
        TAG_MULTICOL,
        TAG_NOBR,
        TAG_SPACER,
        TAG_TT,
        // XML
        TAG_COMMENT,
        TAG_XML,
        // Google AMP tags
        TAG_AMP_VK,
        // A marker value to indicate the end of the enum, for iterating over it.
        // Also used as the terminator for varargs functions that take tags.
        TAG_LAST,
    };

    ETag GetTagEnum(const char* name, size_t len);

    TStringPiece GetTagFromOriginalText(const TStringPiece& text);

    const char* GetTagName(const ETag tag);

}
