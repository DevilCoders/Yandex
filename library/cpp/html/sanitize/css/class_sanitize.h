#pragma once

#include <library/cpp/charset/ci_string.h>
#include <util/generic/ptr.h>
#include <util/generic/list.h>
#include <util/generic/map.h>

#include "sanit_config.h"

namespace Yandex {
    namespace NCssSanitize {
        using namespace NCssConfig;

        class TClassSanitizer {
        public:
            typedef TSet<TCiString> TStringSet;
            struct TRegexpItem {
                TString Text;
                TSimpleSharedPtr<TRegExMatch> Regexp;
                TRegexpItem()
                    : Regexp(nullptr)
                {
                }

                ~TRegexpItem() {
                }
            };
            typedef TList<TRegexpItem> TRegexpList;

        private:
            TString config_file;
            THolder<TRegExMatch> RegExCheck;

            TEmptible<bool> DefaultPass;

            TStringSet ClassSetDeny;
            TStringSet ClassSetPass;

            TRegexpList ClassRegexpDeny;
            TRegexpList ClassRegexpPass;

        private:
            void PrepareConfig(const NCssConfig::TConfig& conf);

        public:
            TClassSanitizer();

            void OpenConfig(const TString& config_file);

            void OpenConfigString(const TString& config_text);

            TString Sanitize(const TString& class_list);

            bool Pass(const TString& class_name);
        };

        using TFormSanitizer = TClassSanitizer;
        using TIframeSanitizer = TClassSanitizer;
    }

}
