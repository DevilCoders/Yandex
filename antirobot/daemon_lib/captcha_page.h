#pragma once

#include "page_template.h"

#include <antirobot/captcha/localized_data.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAntiRobot {
    struct TCaptchaPageParams;

    class TCaptchaPage {
    public:
        TCaptchaPage(
            TString pageTemplate,
            TStringBuf resourceKey,
            TPageTemplate::EEscapeMode escapeMode = TPageTemplate::EEscapeMode::Html
        );

        TString Gen(const TCaptchaPageParams& params) const;
    private:
        TPageTemplate PageTemplate;
        const TStringMap& LocalizedVars;
    };
}
