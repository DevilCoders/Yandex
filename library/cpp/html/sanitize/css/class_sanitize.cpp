#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/system/yassert.h>
#include <util/generic/yexception.h>
#include <util/generic/vector.h>

#include "compile_regex.h"
#include "config-drv.h"
#include "class_sanitize.h"
#include "sanit_config.h"
#include "str_tools.h"

namespace Yandex {
    namespace NCssSanitize {
        using namespace NCssSanit;

        void PrepareEssenceSet(const NCssConfig::TEssenceSet& set, TClassSanitizer::TStringSet& string_set,
                               TClassSanitizer::TRegexpList& regexp_list);

        TClassSanitizer::TRegexpList::const_iterator FindRegexp(const TClassSanitizer::TRegexpList& list, const TString& text);

        TClassSanitizer::TClassSanitizer()
            : RegExCheck(CompileRegexp("[\\w-]+"))
        {
        }

        void TClassSanitizer::OpenConfig(const TString& the_config_file) {
            Y_ASSERT(the_config_file != nullptr);

            NCssConfig::TConfigDriver drv;

            if (!drv.ParseFile(the_config_file))
                throw yexception() << drv.GetParseError();

            PrepareConfig(drv.GetConfig());
        }

        void TClassSanitizer::OpenConfigString(const TString& config_text) {
            NCssConfig::TConfigDriver drv;

            if (!drv.ParseString(config_text))
                throw yexception() << drv.GetParseError();

            PrepareConfig(drv.GetConfig());
        }

        void TClassSanitizer::PrepareConfig(const NCssConfig::TConfig& conf) {
            DefaultPass = conf.DefaultPass;
            if (DefaultPass.IsEmpty())
                DefaultPass = false;

            PrepareEssenceSet(conf.ClassDeny(), ClassSetDeny, ClassRegexpDeny);
            PrepareEssenceSet(conf.ClassPass(), ClassSetPass, ClassRegexpPass);
        }

        TString TClassSanitizer::Sanitize(const TString& class_list) {
            TStrokaList str_list;
            GetStringList(class_list, str_list);

            TString res;

            unsigned i = 0;
            for (; i < str_list.size(); i++) {
                if (Pass(str_list[i])) {
                    res = str_list[i];
                    break;
                }
            }

            for (++i; i < str_list.size(); i++) {
                if (Pass(str_list[i])) {
                    res += TString(" ") + str_list[i];
                }
            }

            return res;
        }

        bool TClassSanitizer::Pass(const TString& class_name) {
            TString esc_resolved = NCssSanit::ResolveEscapes(class_name);

            TStringSet::const_iterator it;
            //TStringSet::const_iterator it_end = ClassSetDeny.end();

            TRegexpList::const_iterator it_regex;
            TRegexpList::const_iterator it_regex_end;

            if (DefaultPass) {
                /* first will find in the list of simple string */
                if (ClassSetDeny.find(esc_resolved) != ClassSetDeny.end()) {
                    /* Found. If there is expression, eval it and return the result */
                    return false;
                } else {
                    /* Next, will find in regexp's */
                    return FindRegexp(ClassRegexpDeny, esc_resolved) == ClassRegexpDeny.end();
                }
            } else {
                if (ClassSetPass.find(esc_resolved) != ClassSetPass.end()) {
                    return true;
                } else {
                    return FindRegexp(ClassRegexpPass, esc_resolved) != ClassRegexpPass.end();
                }
            }
        }

        void PrepareEssenceSet(const NCssConfig::TEssenceSet& set, TClassSanitizer::TStringSet& string_set,
                               TClassSanitizer::TRegexpList& regexp_list) {
            TEssenceSet::TEssenceSetType::const_iterator it = set.GetEssences().begin();
            TEssenceSet::TEssenceSetType::const_iterator it_end = set.GetEssences().end();

            for (; it != it_end; it++) {
                if (it->IsRegexp()) {
                    TClassSanitizer::TRegexpItem item;
                    item.Text = it->GetText();
                    item.Regexp = CompileRegexp(it->GetText());
                    regexp_list.push_back(item);
                } else
                    string_set.insert(it->GetText());
            }
        }

        TClassSanitizer::TRegexpList::const_iterator FindRegexp(const TClassSanitizer::TRegexpList& list, const TString& text) {
            TClassSanitizer::TRegexpList::const_iterator it = list.begin();
            TClassSanitizer::TRegexpList::const_iterator it_end = list.end();

            for (; it != it_end; it++) {
                if (it->Regexp->Match(text.c_str()))
                    return it;
            }
            return it_end;
        }

    }

}

#ifdef CLASS_SANITIZE_TEST

#include <util/stream/output.h>

using namespace Yandex;
using namespace NCssSanitize;

void PrintStringList(const TStringList& list) {
    TStringList::const_iterator it = list.begin();
    TStringList::const_iterator it_end = list.end();

    if (it != it_end) {
        Cout << "<" << *it << ">";
        it++;
        for (; it != it_end; it++)
            Cout << "<" << *it << ">";
    }
}

int main() {
    TString s1 = "a1 a2";

    Cout << "Result:" << Endl;

    TStringList list;
    GetStringList(s1, list);
    PrintStringList(list);

    Cout << Endl;

    list.clear();

    s1 = "  aaa\\a1 bbb\t   ccc \t \n ddd   \t";
    GetStringList(s1, list);
    PrintStringList(list);
    Cout << Endl;

    return 0;
}

#endif
