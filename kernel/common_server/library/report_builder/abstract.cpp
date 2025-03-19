#include "abstract.h"
#include <kernel/common_server/util/types/string_normal.h>

TString IReportBuilderContext::GetUri(const TString& defaultUri /*= ""*/) const {
    TString result = NCS::TStringNormalizer::TruncRet(DoGetUri(), '/');
    if (!result) {
        return defaultUri;
    } else {
        return result;
    }
}
