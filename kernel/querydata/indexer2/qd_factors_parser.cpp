#include "qd_factors_parser.h"

#include <kernel/querydata/common/querydata_traits.h>
#include <kernel/querydata/idl/querydata_structs_client.pb.h>

#include <util/generic/yexception.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/cast.h>
#include <util/string/escape.h>
#include <util/string/strip.h>

namespace NQueryData {

    void FillSourceFactors(TSourceFactors& sf, TStringBuf recordValue) {
        while (recordValue) {
            TStringBuf value = StripString(recordValue.NextTok('\t'));

            if (!value) {
                continue;
            }

            TStringBuf key, valueType;
            value.Split('=', key, value);
            key.Split(':', key, valueType);

            Y_ENSURE(key, "empty factor key");

            if (!valueType) {
                valueType = FACTOR_VALUE_TYPE_STRING;
            }

            auto& factor = *sf.AddFactors();
            factor.SetName(TString{key});

            TString buffer;
            if (FACTOR_VALUE_TYPE_INT == valueType) {
                i64 v;
                Y_ENSURE(value && TryFromString(value, v), "bad value");
                factor.SetIntValue(v);
            } else if (FACTOR_VALUE_TYPE_FLOAT == valueType) {
                float v;
                Y_ENSURE(value && TryFromString(value, v), "bad value");
                factor.SetFloatValue(v);
            } else if (FACTOR_VALUE_TYPE_STRING == valueType) {
                factor.SetStringValue(TString{value});
            } else if (FACTOR_VALUE_TYPE_STRING_ESCAPED == valueType) {
                factor.SetStringValue(UnescapeC(TString{value}));
            } else if (FACTOR_VALUE_TYPE_BINARY == valueType) {
                factor.SetBinaryValue(Base64Decode(value));
            } else {
                Y_ENSURE(false, "unsupported value type suffix " << valueType);
            }
        }
    }

}
