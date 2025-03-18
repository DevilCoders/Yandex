#pragma once

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <library/cpp/regex/pcre/regexp.h>

namespace NRemorphCompiler {

typedef THashMap<TString, TString> TVars;

struct TVarReplacingError: public yexception {
    TString Name;

    TVarReplacingError(const TStringBuf& name)
        : Name(name)
    {
        *this << "Undefined variable: " << Name;
    }
};

class TVarReplacer {
private:
    TVars Vars;
    THolder<TRegExBase> RegEx;

public:
    TVarReplacer(const TVars& vars);

    void Parse(TString& str, const TVars& runtimeVars) const;
};

} // NRemorphCompiler
