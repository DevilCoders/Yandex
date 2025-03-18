#include <algorithm>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/string.h>

#include "sanit_policy.h"
#include "sanit_config.h"
#include "str_tools.h"
#include "compile_regex.h"

namespace NCssSanit {
    using namespace NCssConfig;

    TCiString ClearSelector(const TString& str);

    TSanitPolicy::TSanitPolicy(const NCssConfig::TConfig& aConfig)
        : Config(aConfig)
    {
        Prepare();
    }

    TSanitPolicy::~TSanitPolicy() {
        Clear();
    }

    void TSanitPolicy::Clear() {
        Clear(PropRegexpDeny);
        Clear(PropRegexpPass);
        Clear(SelectorRegexpDeny);
        Clear(SelectorRegexpPass);
        Clear(SchemeRegexpDeny);
        Clear(SchemeRegexpPass);
    }

    bool TSanitPolicy::Contains(const TSanitPolicy::TSimpleMap& map, const TString& text) {
        return map.find(text) != map.end();
    }

    bool TSanitPolicy::PassProperty(const TString& prop_name, const TContext& context) const {
        TString esc_resolved = ResolveEscapes(prop_name);

        if (Config.DefaultPass) {
            TSimpleMap::const_iterator it1 = PropMapDeny.find(esc_resolved);

            /* first will find in the list of simple string */
            if (it1 != PropMapDeny.end()) {
                /* Found. If there is expression, eval it and return the result */
                return it1->second ? !it1->second->DoEval(context) : false;
            } else {
                /* Next, will find in regexp's */
                TRegexpList::const_iterator it2 = FindRegexp(PropRegexpDeny, esc_resolved);

                if (it2 != PropRegexpDeny.end())
                    return it2->Expr ? !it2->Expr->DoEval(context) : false;
                else
                    return true;
            }
        } else {
            TSimpleMap::const_iterator it3 = PropMapPass.find(esc_resolved);
            if (it3 != PropMapPass.end()) {
                return it3->second ? it3->second->DoEval(context) : true;
            } else {
                TRegexpList::const_iterator it4 = FindRegexp(PropRegexpPass, esc_resolved);

                if (it4 != PropRegexpPass.end())
                    return it4->Expr ? it4->Expr->DoEval(context) : true;
                else
                    return false;
            }
        }
    }

    bool TSanitPolicy::DenyProperty(const TString& prop_name, const TContext& context) const {
        return !PassProperty(prop_name, context);
    }

    bool TSanitPolicy::PassSelector(const TString& selector_name) const {
        TString esc_resolved = ResolveEscapes(selector_name);

        if (Config.DefaultPass) {
            TSimpleMap::const_iterator it5 = SelectorMapDeny.find(esc_resolved);

            if (it5 != SelectorMapDeny.end())
                return false;
            else {
                TRegexpList::const_iterator it6 = FindRegexp(SelectorRegexpDeny, esc_resolved);

                if (it6 != SelectorRegexpDeny.end())
                    return false;
                else
                    return true;
            }
        } else {
            TSimpleMap::const_iterator it7 = SelectorMapPass.find(esc_resolved);
            if (it7 != SelectorMapPass.end()) {
                return true;
            } else {
                TRegexpList::const_iterator it8 = FindRegexp(SelectorRegexpPass, esc_resolved);

                if (it8 != SelectorRegexpPass.end())
                    return true;
                else
                    return false;
            }
        }
    }

    bool TSanitPolicy::DenySelector(const TString& selector_name) const {
        return !PassSelector(selector_name);
    }

    bool TSanitPolicy::PassExpression() const {
        return Config.ExpressionPass.IsEmpty() ? Config.DefaultPass : Config.ExpressionPass;
    }

    TString TSanitPolicy::HandleImport(const TString& import_param) const {
        TString res;
        if (Config.ImportAction.IsEmpty()) {
            if (Config.DefaultPass)
                res = import_param;
        } else {
            switch (Config.ImportAction->Action) {
                case AT_DENY:
                    break;

                case AT_PASS:
                    res = import_param;
                    break;

                case AT_MERGE:
                    // TO DO
                    break;
                case AT_REWRITE:
                    res = RewriteUrl(import_param);
                    break;
                default:
                    break;
            }
        }
        return res;
    }

    bool TSanitPolicy::PassScheme(const TString& scheme_name) const {
        TString esc_resolved = ResolveEscapes(scheme_name);

        if (Config.DefaultPass) {
            TSimpleMap::const_iterator it9 = SchemeMapDeny.find(esc_resolved);

            if (it9 != SchemeMapDeny.end())
                return false;
            else {
                TRegexpList::const_iterator it10 = FindRegexp(SchemeRegexpDeny, esc_resolved);

                if (it10 != SchemeRegexpDeny.end())
                    return false;
                else
                    return true;
            }
        } else {
            TSimpleMap::const_iterator it11 = SchemeMapPass.find(esc_resolved);
            if (it11 != SchemeMapPass.end()) {
                return true;
            } else {
                TRegexpList::const_iterator it12 = FindRegexp(SchemeRegexpPass, esc_resolved);

                if (it12 != SchemeRegexpPass.end())
                    return true;
                else
                    return false;
            }
        }
    }

    bool TSanitPolicy::DenyScheme(const TString& scheme_name) const {
        return !PassScheme(scheme_name);
    }

    TString TSanitPolicy::RewriteUrl(const TString& url) const {
        TString res;
        const TString& templ = Config.ImportAction->StrParam;

        res.reserve(templ.size() + url.size());

        TString::const_iterator it = templ.begin();
        TString::const_iterator end = templ.end();

        while (it != end) {
            switch (*it) {
                case '\\':
                    it++;
                    if (it != url.end() && *it == '$')
                        res += '$';
                    else {
                        res += '\\';
                        res += *it;
                    }
                    break;
                case '$':
                    it++;
                    if (it != end && *it == '0') {
                        res += UrlEncode(url);
                    } else {
                        res += '$';
                        if (it != end)
                            res += *it;
                    }
                    break;
                default:
                    res += *it;
                    break;
            }
            it++;
        }

        return res;
    }

    void TSanitPolicy::Prepare() {
        PrepareEssenceSet(Config.PropertyDeny().GetEssences(), PropMapDeny, PropRegexpDeny);
        PrepareEssenceSet(Config.PropertyPass().GetEssences(), PropMapPass, PropRegexpPass);
        PrepareEssenceSet(Config.SelectorPass().GetEssences(), SelectorMapPass, SelectorRegexpPass);
        PrepareEssenceSet(Config.SelectorDeny().GetEssences(), SelectorMapDeny, SelectorRegexpDeny);
        PrepareEssenceSet(Config.SchemeDeny().GetEssences(), SchemeMapDeny, SchemeRegexpDeny);
        PrepareEssenceSet(Config.SchemePass().GetEssences(), SchemeMapPass, SchemeRegexpPass);
        PrepareSelectorAppend(Config.SelectorAppend());
    }

    void TSanitPolicy::PrepareSelectorAppend(const TStrokaList& list) {
        TStrokaList::const_iterator it = list.begin();
        TStrokaList::const_iterator it_end = list.end();

        SelectorAppend = "";

        if (it != it_end) {
            SelectorAppend = *it++;
            for (; it != it_end; it++)
                SelectorAppend += " " + *it;
        }
    }

    void TSanitPolicy::PrepareEssenceSet(const TEssenceSet::TEssenceSetType& essence_set,
                                         TSimpleMap& map, TRegexpList& list) {
        TEssenceSet::TEssenceSetType::const_iterator it = essence_set.begin();
        TEssenceSet::TEssenceSetType::const_iterator it_end = essence_set.end();

        for (; it != it_end; it++) {
            if (it->IsRegexp()) {
                TRegexpItem item;
                item.Text = it->GetText();
                item.Expr = it->GetExpr();
                item.Regexp = CompileRegexp(it->GetText());
                list.push_back(item);
            } else
                map[it->GetText()] = it->GetExpr();
        }
    }

    TSanitPolicy::TRegexpList::const_iterator TSanitPolicy::FindRegexp(const TSanitPolicy::TRegexpList& list, const TString& text) {
        TRegexpList::const_iterator it = list.begin();
        TRegexpList::const_iterator it_end = list.end();

        for (; it != it_end; it++) {
            if (it->Regexp->Match(text.c_str()))
                return it;
        }
        return it_end;
    }

    void TSanitPolicy::Clear(TRegexpList& list) {
        TRegexpList::iterator it = list.begin();
        TRegexpList::iterator it_end = list.end();

        for (; it != it_end; it++) {
            delete it->Regexp;
        }
    }

    TCiString ClearSelector(const TString& str) {
        size_t pos = str.find(':');

        if (pos != str.npos) {
            return TCiString(str.begin(), str.begin() + pos);
        } else
            return TCiString(str);
    }

    void SplitByHTMLorBODY(const TString& str, TString& str1, TString& str2) {
        TStrokaList selector_parts;
        GetStringList(str, selector_parts);

        str1 = "";
        str2 = "";

        TStrokaList::const_iterator it = selector_parts.begin();
        TStrokaList::const_iterator it_end = selector_parts.end();
        TStrokaList::const_iterator it_middle = it_end;

        for (; it != it_end; it++) {
            TCiString s = ClearSelector(*it);
            if (s == "html" || s == "body") { // 'magic' tags
                it_middle = it;
                break;
            }
        }

        it = selector_parts.begin();

        /* str1 will contain selector's parts from begining to 'magic' tag inclusive */
        if (it_middle != it_end) {
            str1 = *it;

            while (true) {
                if (it == it_middle) {
                    it++;
                    break;
                } else if (it != it_end) {
                    ++it;
                    str1 += " " + *it;
                } else
                    break;
            }
        }

        /* the rest is in str2 */
        if (it != it_end) {
            str2 = *it++;
            for (; it != it_end; it++)
                str2 += " " + *it;
        }
    }

    TString TSanitPolicy::CorrectSelector(const TString& selector) const {
        if (!SelectorAppend.empty()) {
            TString str1;
            TString str2;

            SplitByHTMLorBODY(selector, str1, str2);

            return str1 + (str1.empty() ? "" : " ") + SelectorAppend + " " + str2;
            //if ( TCiString(add_str) == TCiString("html") ||
        } else
            return selector;
    }

} //namespace NCssSanit
