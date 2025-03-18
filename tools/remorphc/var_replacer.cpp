#include "var_replacer.h"

namespace NRemorphCompiler {

TVarReplacer::TVarReplacer(const TVars& vars)
    : Vars(vars)
    , RegEx(new TRegExBase("{([a-zA-Z0-9_]+)}", REG_EXTENDED))
{
}

void TVarReplacer::Parse(TString& str, const TVars& runtimeVars) const {
    size_t pos = 0;
    regmatch_t matches[NMATCHES];
    while (pos < str.size()) {
        if (RegEx->Exec(str.data() + pos, matches, 0) != 0) {
            break;
        }
        Y_ASSERT(matches[0].rm_so != -1);
        Y_ASSERT(matches[1].rm_so != -1);
        TStringBuf name(str.data() + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
        const TString* value = nullptr;
        TVars::const_iterator found = Vars.find(name);
        if (found != Vars.end()) {
            value = &(found->second);
        } else {
            TVars::const_iterator runtimeFound = runtimeVars.find(name);
            if (runtimeFound != runtimeVars.end()) {
                value = &(runtimeFound->second);
            }
        }
        if (!value) {
            throw TVarReplacingError(name);
        }
        str.replace(matches[0].rm_so, matches[0].rm_eo - matches[0].rm_so, *value);
        pos += matches[0].rm_eo;
    }
}

} // NRemorphCompiler
