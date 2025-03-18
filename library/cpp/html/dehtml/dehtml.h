#pragma once

#include <library/cpp/charset/doccodes.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

enum HtmlStripMode {
    HSM_NONE = 1 << 0,            // this has no effect but is nonzero
    HSM_ENTITY = 1 << 1,          // "...&lt;..." -> "...<..."
    HSM_SPACE = 1 << 2,           // remove repeated spaces
    HSM_SEMICOLON = 1 << 3,       // ';' -> ',' after processing but before removing repeated spaces
    HSM_DELIMSPACE = 1 << 4,      // treat ',' as space symbol while removing repeated spaces
    HSM_TRASH = 1 << 5,           // remove html trash (eol before closing tag, incorrect tags and entities)
    HSM_NO_SPACE_INSERT = 1 << 6, // not insert spaces instead of html tags

    HSM_EXT_GLUE = 1 << 16,     // not implemented
    HSM_EXT_CLEANURL = 1 << 17, // not implemented
};

// deletes html tags from input string
class THtmlStripper {
public:
    THtmlStripper(int mode, ECharset encoding);

    // process methods
    // These methods don't copy the string data if output appears the same as input.

    TString operator()(const TString& html) const;

    bool operator()(const TStringBuf& html, TString& out) const; // true if out has been written

    void operator()(const TStringBuf& html, IOutputStream& out) const;

    static void Strip(int mode, char* out, size_t& out_len, const char* text, size_t len, ECharset enc);

private:
    int Mode;
    ECharset Encoding;
};
