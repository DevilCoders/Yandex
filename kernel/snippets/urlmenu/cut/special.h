#pragma once

#include <kernel/snippets/urlmenu/common/common.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

namespace NUrlMenu {
    class TSpecialBuilder {
    public:
        static void Create(const TString& url, TUrlMenuVector& res, ELanguage lang);
    };
}
