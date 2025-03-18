#pragma once

#include <library/cpp/charset/ci_string.h>
#include <util/generic/list.h>
#include <util/generic/map.h>
#include <util/generic/yexception.h>
#include <library/cpp/regex/pcre/regexp.h>
#include "sanit_config.h"
#include "expressions.h"

namespace NCssSanit {
    using namespace NCssConfig;

    class TSanitPolicy {
    public:
        struct Error: public yexception {
            Error(const TString& msg) {
                *this << msg;
            }
        };

    private:
        struct TRegexpItem {
            TString Text;
            TRegExMatch* Regexp;
            const TBoolExpr* Expr;
        };

        //typedef std::unordered_map<TString, const TBoolExpr*> TSimpleMap;
        typedef TMap<TCiString, const TBoolExpr*> TSimpleMap;
        typedef TList<TRegexpItem> TRegexpList;

    private:
        NCssConfig::TConfig Config;

        TSimpleMap PropMapDeny;
        TSimpleMap PropMapPass;
        TSimpleMap SelectorMapDeny;
        TSimpleMap SelectorMapPass;
        TSimpleMap SchemeMapDeny;
        TSimpleMap SchemeMapPass;

        TRegexpList PropRegexpDeny;
        TRegexpList PropRegexpPass;
        TRegexpList SelectorRegexpDeny;
        TRegexpList SelectorRegexpPass;
        TRegexpList SchemeRegexpDeny;
        TRegexpList SchemeRegexpPass;

        TString SelectorAppend;

    protected:
        TString RewriteUrl(const TString& url) const;
        //    std::string ResolveEscapes(const std::string & str) const;
        //    std::string TranslateEscape(std::string::const_iterator & it, const std::string::const_iterator & end ) const;
        void Prepare();
        void Clear();
        static void PrepareEssenceSet(const TEssenceSet::TEssenceSetType& essence_set,
                                      TSimpleMap& map, TRegexpList& list);
        static void Clear(TRegexpList& list);
        static bool Contains(const TSimpleMap& map, const TString& text);
        static TRegexpList::const_iterator FindRegexp(const TRegexpList& list, const TString& text);

        void PrepareSelectorAppend(const TStrokaList& str_list);

    private:
        /* NO COPY*/
        TSanitPolicy(const TSanitPolicy&) {
        }
        void operator=(const TSanitPolicy&) {
        }

    public:
        TSanitPolicy(const NCssConfig::TConfig& config);
        ~TSanitPolicy();

        bool PassProperty(const TString& prop_name, const TContext& context) const;
        bool DenyProperty(const TString& prop_name, const TContext& context) const;
        bool PassExpression() const;
        bool DefaultPass() const {
            return Config.DefaultPass;
        }
        bool DefaultDeny() const {
            return !Config.DefaultPass;
        }
        bool PassSelector(const TString& selector_name) const;
        bool DenySelector(const TString& selector_name) const;
        bool PassScheme(const TString& scheme_name) const;
        bool DenyScheme(const TString& scheme_name) const;

        //css_config::Config::ActionType ImportAction() const;
        //css_config::Config::ActionTypeParam ImportActionParam() const;
        TString HandleImport(const TString& import_param) const;
        TString CorrectSelector(const TString& selector) const;
    };

} //namespace NCssSanit
