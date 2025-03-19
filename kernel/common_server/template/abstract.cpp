#include "abstract.h"

namespace NCS {
    TString IVariableTemplate::Apply(const TString& input) const {
        TStringBuilder sb;
        size_t pos = 0;
        while (pos < input.size() && pos != TString::npos) {
            const auto begin = input.find("$(", pos);
            if (begin == TString::npos) {
                sb << input.substr(pos);
                break;
            }
            sb << input.substr(pos, begin);
            const auto end = input.find(")", begin);
            if (end == TString::npos) {
                return input;
            }
            const TString field = input.substr(begin + 2, end - begin - 2);
            TString result;
            if (!GetVariable(field, result)) {
                return input;
            }
            sb << result;
            pos = end + 1;
        }

        return sb;
    }

    bool TTemplateSettings::GetVariable(const TString& field, TString& result) const {
        return Settings.GetValueStr(Prefix + "." + field, result);
    }
}