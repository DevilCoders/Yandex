#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <algorithm>

#include <util/stream/file.h>
#include <util/string/cast.h>

namespace {

const TVector<TString> SVN_KEYWORDS = {
    "$Rev",
    "$Revision"
};

}

bool IsSvnKeyword(const TStringBuf& s) {
    auto str = s.After('"').RBefore('"');
    auto it = std::find_if(SVN_KEYWORDS.begin(), SVN_KEYWORDS.end(), [&str](auto const& s){
        return str.StartsWith(s);
    });
    return it != SVN_KEYWORDS.end() && str.back() == '$';
}

TString ExtractSvnKeywordValue(const TStringBuf& s)
{
    auto val = s.After(':');
    if (val == s) {
        Cerr << "Warning: SVN keyword '" << s << "' was not expanded on svn checkout!\n";
        return s.front() == '"' ? "\"\"" : "0";
    }
    val = val.RBefore('$');
    if (val.front() == ' ') {
        val = val.After(' ');
    }
    if (val.back() == ' ') {
        val = val.RBefore(' ');
    }
    return s.front() == '"' ? "\"" + ToString(val) + "\"" : ToString(val);
}
