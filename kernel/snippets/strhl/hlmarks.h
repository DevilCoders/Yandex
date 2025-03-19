#pragma once

#include "hilite_mark.h"

#include <util/charset/wide.h>

namespace NSnippets {
    static const THiliteMark EXT_MARK = THiliteMark(u"\x07((", u"\x07))");
    static const THiliteMark LINK_MARK = THiliteMark(u"\x07+", u"\x07-");
    static const THiliteMark TABLECELL_MARK = THiliteMark(u"\x07|", u"");
}
