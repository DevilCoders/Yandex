#include "environment.h"

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/system/winint.h>

extern char **environ;

NUtil::TEnvVars NUtil::GetEnvironmentVariables() {
    NUtil::TEnvVars result;
#if defined(_unix_)
    for (char** env = environ; *env; ++env) {
        TStringBuf name, value;
        if (TStringBuf(*env).TrySplit('=', name, value)) {
            result.push_back(std::make_pair(TString{name}, TString{value}));
        }
    }
#elif defined(_win_)
    wchar_t* env = GetEnvironmentStringsW();
    for (size_t s = 0; env[s];) {
        TWtringBuf variable((wchar16*)env + s);
        TWtringBuf name;
        TWtringBuf value;
        if (variable.TrySplit('=', name, value)) {
            const TString& utfName = WideToUTF8(name);
            const TString& utfValue = WideToUTF8(value);
            result.push_back(std::make_pair(utfName, utfValue));
        } else {
            Cerr << "incorrect envvar string: " << variable;
        }

        s += variable.size() + 1;
    }
#else
#   error Unsupported platform
#endif
    return result;
}
