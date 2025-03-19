#include "coded_exception.h"

NJson::TJsonValue TCodedException::GetDetailedReport() const {
    NJson::TJsonValue result;
    result["details"] = Details;
    result["debug_message"] = DebugMessage + "/" + what();
    result["meta_code"] = Code;
    result["http_code"] = Code;
    if (!SpecialErrorCodes.empty()) {
        NJson::TJsonValue specials;
        for (auto&& i : SpecialErrorCodes) {
            specials.AppendValue(i);
        }
        result.InsertValue("errors", specials);
    }
    return result;
}
