#include <library/cpp/charset/wide.h>
#include <library/cpp/regex/pire/regexp.h>
#include <library/cpp/binsaver/bin_saver.h>
#include <util/system/yassert.h>
#include "regex_wrapper.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSerializibleScanner
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static NPire::TScanner CompileRegexp(const TUtf16String& pattern) {
    return Pire::Lexer(pattern.data(), pattern.data() + pattern.size())
        .SetEncoding(NPire::NEncodings::Utf8())
        .Parse()
        .Surround()
        .Compile<NPire::TScanner>();
}

static NPire::TScanner CompileRegexp(const TString& pattern, ECharset charset) {
    return CompileRegexp(CharToWide(pattern, charset));
}

static NPire::TScanner GetFalseScanner() {
    return NPire::TFsm::MakeFalse().Compile<NPire::TScanner>();
}

TSerializibleScanner::TSerializibleScanner()
    : NPire::TScanner(GetFalseScanner()) {
}

TSerializibleScanner::TSerializibleScanner(const NPire::TScanner& scanner)
    : NPire::TScanner(scanner) {
}

TSerializibleScanner::TSerializibleScanner(const TUtf16String& pattern)
    : NPire::TScanner(CompileRegexp(pattern)) {
}

TSerializibleScanner::TSerializibleScanner(const TString& pattern, ECharset charset)
    : NPire::TScanner(CompileRegexp(pattern, charset)) {
}

bool TSerializibleScanner::IsMatch(const TUtf16String& what) const {
    return IsMatch(WideToUTF8(what));
}

bool TSerializibleScanner::IsMatch(const TString& utf8what) const {
    return NPire::Runner(*this).Begin().Run(utf8what.data(), utf8what.size()).End();
}

int TSerializibleScanner::operator&(IBinSaver &f) {
    TStringStream ss;
    if (f.IsReading()) {
        TString s;
        f.Add(2, &s);
        ss << s;
        Load(&ss);

    } else {
        Save(&ss);
        f.Add(2, &ss.Str());
    }
    return 0;
}

bool TSerializibleScanner::operator == (const TSerializibleScanner& /*rhs*/) const {
    return 0;
}

TSerializibleScanner operator | (const TSerializibleScanner& lhs, const TSerializibleScanner& rhs) {
    NPire::TScanner res = NPire::TScanner::Glue(lhs, rhs);
    Y_VERIFY(!res.Empty());
    return res;
}

void operator |= (TSerializibleScanner& lhs, const TSerializibleScanner& rhs) {
    lhs = NPire::TScanner::Glue(lhs, rhs);
    Y_VERIFY(!lhs.Empty());
}
