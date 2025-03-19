#include "normalizer.h"
#include <kernel/common_server/util/datetime/datetime.h>

namespace NCS {
    namespace NNormalizer {
        TStringNormalizerInstant::TFactory::TRegistrator<TStringNormalizerInstant> TStringNormalizerInstant::Registrator(TStringNormalizerInstant::GetTypeName());

        bool TStringNormalizerInstant::DoNormalize(const TStringBuf sbValue, TString& result) const {
            auto gLogging = TFLRecords::StartContext()("str_value", sbValue)("format", InstantView);
            TInstant value;
            if (InstantView == EInstantView::ISO8601) {
                if (!TInstant::TryParseIso8601(sbValue, value)) {
                    TFLEventLog::Error("cannot parse string as instant");
                    return false;
                }
            } else if (InstantView == EInstantView::RFC822) {
                if (!TInstant::TryParseRfc822(sbValue, value)) {
                    TFLEventLog::Error("cannot parse string as instant");
                    return false;
                }
            } else if (InstantView == EInstantView::Seconds) {
                TString resultLocal;
                for (auto&& i : sbValue) {
                    if (i >= '0' && i <= '9') {
                        resultLocal += i;
                    }
                }
                ui32 seconds;
                if (!TryFromString(resultLocal, seconds)) {
                    TFLEventLog::Error("cannot parse seconds as instant");
                    return false;
                }
                value = TInstant::Seconds(seconds);
            } else if (InstantView == EInstantView::Custom) {
                if (!TDateTimeParser::Parse(sbValue, Template, value)) {
                    TFLEventLog::Error("cannot parse string as instant");
                    return false;
                }
            } else {
                TFLEventLog::Error("incorrect instant_view");
                return false;
            }
            result = ::ToString(value.Seconds());
            return true;
        }

    }
}
