#pragma once

#include "util.h"

namespace NHtml5 {
    /**
 * Fixes the case of SVG elements that are not all lowercase.
 * http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#parsing-main-inforeign
 * This is not done at parse time because there's no place to store a mutated
 * tag name.  tag_name is an enum (which will be TAG_UNKNOWN for most SVG tags
 * without special handling), while original_tag_name is a pointer into the
 * original buffer.  Instead, we provide this helper function that clients can
 * use to rename SVG tags as appropriate.
 * Returns the case-normalized SVG tagname if a replacement is found, or NULL if
 * no normalization is called for.  The return value is static data and owned by
 * the library.
 */
    const char* NormalizeSvgTagname(const TStringPiece& tag);

}
