#pragma once

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>

namespace NStringMatchTracker {

    class TTextWithDescription {
    public:
        TTextWithDescription(const TString text)
            : Text(text)
        {
        }

        TTextWithDescription& Set(ELanguage lang) {
            Lang = lang;
            return *this;
        }

        TString GetText() const {
            return Text;
        }

        ELanguage GetLang() const {
            return Lang;
        }

    private:
        TString Text;
        ELanguage Lang = LANG_UNK;
    };

    class IPreparer {
    public:
        virtual TString Prepare(const TTextWithDescription& descr) const = 0;

        virtual ~IPreparer() {}
    };
}
