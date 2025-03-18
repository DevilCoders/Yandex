#include "dumphelper.h"

namespace NSegutils {
const char UNKNOWN[] = "unknown";

const char * GetConstName(PARSED_TYPE t) {
    switch (t) {
    default:
        return UNKNOWN;
    case PARSED_ERROR:
        return "error";
    case PARSED_EOF:
        return "eof";
    case PARSED_TEXT:
        return "text";
    case PARSED_MARKUP:
        return "markup";
    }
}

const char * GetConstName(HTLEX_TYPE t) {
    switch (t) {
    default:
        return UNKNOWN;
    case HTLEX_EOF:
        return "eof";
    case HTLEX_EMPTY_TAG:
        return "emptyTag";
    case HTLEX_START_TAG:
        return "startTag";
    case HTLEX_END_TAG:
        return "endTag";
    case HTLEX_MD:
        return "md";
    case HTLEX_PI:
        return "pi";
    case HTLEX_TEXT:
        return "text";
    case HTLEX_COMMENT:
        return "comment";
    case HTLEX_ASP:
        return "asp";
    }
}

const char * GetConstName(BREAK_TYPE t) {
    switch (t) {
    default:
        return UNKNOWN;
    case BREAK_NONE:
        return "none";
    case BREAK_DOMPATH:
        return "dompath";
    case BREAK_WORD:
        return "word";
    case BREAK_SENTENCE:
        return "sentence";
    case BREAK_PARAGRAPH:
        return "paragraph";
    case BREAK_CHAPTER:
        return "chapter";
    case BREAK_BODY:
        return "body";
    case BREAK_DOCUMENT:
        return "document";
    }
}

const char * GetConstName(SPACE_MODE t) {
    switch (t) {
    default:
        return UNKNOWN;
    case SPACE_DEFAULT:
        return "default";
    case SPACE_PRESERVE:
        return "preserve";
    case SPACE_NOBREAK:
        return "nobreak";
    }
}

const char * GetConstName(TEXT_WEIGHT t) {
    switch (t) {
    default:
        return UNKNOWN;
    case WEIGHT_ZERO:
        return "zero";
    case WEIGHT_LOW:
        return "low";
    case WEIGHT_NORMAL:
        return "normal";
    case WEIGHT_HIGH:
        return "high";
    case WEIGHT_BEST:
        return "best";
    }
}

const char * GetConstName(ATTR_POS t) {
    switch (t) {
    default:
        return UNKNOWN;
    case APOS_DOCUMENT:
        return "document";
    case APOS_ZONE:
        return "zone";
    }
}

const char * GetConstName(ATTR_TYPE t) {
    switch (t) {
    default:
        return UNKNOWN;
    case ATTR_LITERAL:
        return "literal";
    case ATTR_URL:
        return "url";
    case ATTR_DATE:
        return "date";
    case ATTR_INTEGER:
        return "integer";
    case ATTR_BOOLEAN:
        return "boolean";
    }
}

const char * GetConstName(MARKUP_TYPE t) {
    switch (t) {
    default:
        return UNKNOWN;
    case MARKUP_IGNORED:
        return "ignored";
    case MARKUP_IMPLIED:
        return "implied";
    case MARKUP_NORMAL:
        return "normal";
    }
}
}
