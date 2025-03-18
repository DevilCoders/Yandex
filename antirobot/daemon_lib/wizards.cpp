#include "wizards.h"

#include "eventlog_err.h"

#include <antirobot/lib/kmp_skip_search.h>
#include <antirobot/lib/yandexuid.h>

#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <search/fields/fields.h>
#include <search/idl/events.ev.pb.h>
#include <search/wizard/config/config.h>
#include <search/wizard/face/wizinfo.h>
#include <search/wizard/remote/remote_wizard.h>

#include <ysite/yandex/reqanalysis/normalize.h>

#include <library/cpp/charset/recyr.hh>
#include <library/cpp/http/cookies/cookies.h>

#include <util/ysaveload.h>
#include <util/charset/utf8.h>

#define A_INT "AntirobotInternal"

Y_DECLARE_PODTYPE(NAntiRobot::TWizardFactorsCalculator::TSyntaxFactors);

namespace NAntiRobot {
    static const TKmpSkipSearch searchCityIds("\"CityIDs\":[");

    template <class T, class I = ui32>
    struct TArrayItemIndexGetter: public THashMap<T, I> {
        template <class S>
        TArrayItemIndexGetter(const S* items, I n) {
            for (I i = 0; i < n; ++i) {
                (*this)[items[i]] = i;
            }
        }

        I operator()(const T& item) const noexcept {
            typename TArrayItemIndexGetter::const_iterator it = this->find(item);

            return it == this->end() ? I(-1) : it->second;
        }
    };

    static TArrayItemIndexGetter<TStringBuf>  GetQueryLangID(QUERY_LANGS, NUM_QUERY_LANGS);
    static TArrayItemIndexGetter<ELanguage> GetPersonLangID(PERSON_LANGS, NUM_PERSON_LANGS);

    class TSyntaxFactorsCalc {
        typedef TWizardFactorsCalculator::TSyntaxFactors TSyntaxFactors;

    public:
        static void Apply(TSyntaxFactors& sf, const TRichRequestNode* t) {
            Syntax(sf, t);
            Restr(sf, t);
        }

    private:
        static void Syntax(TSyntaxFactors& sf, const TRichRequestNode* t) {
            sf.HasMiscOps |= !t->MiscOps.empty(); // << site:url
            sf.HasQuotes |= IsQuote(*t);

            if (!IsQuote(*t)) {
                sf.HasUserOp |= t->GetPhraseType() == PHRASE_USEROP; // &/(1 1)
                sf.HasNecessity |= t->Necessity != nDEFAULT; //+, -, %
                sf.HasFormType |= t->GetFormType() != fGeneral;

                if (!IsMultitoken(*t)) {
                    for (size_t i = 0; i < t->Children.size(); ++i) {
                        Syntax(sf, t->Children[i].Get());
                    }
                }
            }
        }

        static void Restr(TSyntaxFactors& sf, const TRichRequestNode* t) {
            if (IsAttributeOrZone(*t)) {
                TString key = WideToASCII(t->GetTextName());

                if (key == "url"sv) {
                    ++sf.UrlRestr;
                } else if (key == "host"sv || key == "rhost"sv) {
                    ++sf.HostRestr;
                } else if (key == "domain"sv) {
                    ++sf.DomainRestr;
                } else if (key == "inurl"sv) {
                    ++sf.InUrlRestr;
                } else if (key == "site"sv) {
                    ++sf.SiteRestr;
                } else {
                    ++sf.OtherRestr;
                }
            }

            for (size_t i = 0; i < t->MiscOps.size(); ++i) {
                Restr(sf, t->MiscOps[i].Get());
            }

            for (size_t i = 0; i < t->Children.size(); ++i) {
                Restr(sf, t->Children[i].Get());
            }
        }
    };

    template<typename T>
    inline static T ParseProperty(const IRulesResults& wizardResults, const char* ruleName, const char* propName) {
        TStringBuf value = wizardResults.GetProperty(ruleName, propName, 0);
        return value.empty() ? T() : FromString<T>(value);
    }

    template<typename T>
    inline static T ParseFactor(const TSearchFields* searchFields, const char* section, const char* factorName, const T& defaultValue = T()) {
        TStringBuf value;
        if (searchFields->GetFactor(factorName, section, value)) {
            return FromString<T>(value);
        } else {
            return defaultValue;
        }
    }

    template<typename T>
    inline static T ParseRearrFactor(const TSearchFields* searchFields, const char* factorName, const T& defaultValue = T()) {
        return ParseFactor<T>(searchFields, "rearr", factorName, defaultValue);
    }

    template<typename T>
    inline static T ParseRelevFactor(const TSearchFields* searchFields, const char* factorName, const T& defaultValue = T()) {
        return ParseFactor<T>(searchFields, "relev", factorName, defaultValue);
    }

    static void FillWizardFactors(TWizardFactorsCalculator::TWizardFactors& wf, const IRulesResults* wizardResults, const TSearchFields* searchFields) {
        wf.Commercial = ParseProperty<float>(*wizardResults, "Commercial", "commercial");
        wf.PopularityLevel = ParseProperty<ui32>(*wizardResults, "PopularityRequest", "PopularityLevel");
        ui32 queryLangId = GetQueryLangID(wizardResults->GetProperty("QueryLang", "queryLang", 0));
        TStringBuf geoCityStr = searchCityIds.SearchInText(wizardResults->GetProperty("GeoAddr", "Body", 0)).Skip(searchCityIds.Length()).Before(']').Before(',');
        wf.GeoCity = geoCityStr.empty() ? 0 : FromString<int>(geoCityStr);
        wf.PornoLevel = ParseProperty<float>(*wizardResults, "PornoQuery", "porno");
        wf.WizardCacheHit = IsTrue(wizardResults->GetProperty("Cache", "FromCache"));

        ui32 personLangId = GetPersonLangID(static_cast<ELanguage>(ParseRearrFactor<size_t>(searchFields, "fav_lang"))); // PersonData.FavourLanguage
        wf.IsNav = ParseRelevFactor<bool>(searchFields, "is_nav"); // IsNav.IsNav
        wf.GeoLocality = ParseRelevFactor<float>(searchFields, "gaddr");
        const i32 queryClass = ParseRelevFactor<i32>(searchFields, "qc"); // Classification.queryclass

        for (size_t i = 0; i < NUM_WIZARD_QUERY_CLASSES; ++i) {
            wf.QueryClass[i] = (queryClass & (1 << i)) != 0;
        }

        if (queryLangId < NUM_QUERY_LANGS) {
            wf.QueryLang[queryLangId] = true;
        }

        if (personLangId < NUM_PERSON_LANGS) {
            wf.PersonLang[personLangId] = true;
        }
    }

    static TString ConvertToUtf8(const TString& s) {
        return Recode(CODES_WIN, CODES_UTF8, s);
    }

    void TWizardFactorsCalculator::TValues::Print(IOutputStream& out) const {
#ifdef OUT
#   undef OUT
#endif

#ifdef OUTA
#   undef OUTA
#endif

#define OUT(X)\
                out << #X << " = " << X << Endl
#define OUTA(X, i)\
                out << #X << '[' << i << "] = " << X[i] << Endl

        OUT(HaveSyntax());
        OUT(HasMiscOps);
        OUT(HasUserOp);
        OUT(HasNecessity);
        OUT(HasFormType);
        OUT(HasQuotes);
        OUT(UrlRestr);
        OUT(SiteRestr);
        OUT(HostRestr);
        OUT(DomainRestr);
        OUT(InUrlRestr);
        OUT(OtherRestr);
        OUT(RequestLength);
        OUT(NumWords);
        OUT(SyntaxError);

        OUT(Commercial);
        OUT(PopularityLevel);
        OUT(IsNav);
        for (size_t i = 0; i < NUM_QUERY_LANGS; ++i) {
            OUTA(QueryLang, i);
        }
        for (size_t i = 0; i < NUM_PERSON_LANGS; ++i) {
            OUTA(PersonLang, i);
        }
        OUT(GeoCity);
        OUT(GeoLocality);
        OUT(PornoLevel);
        OUT(WizardCacheHit);
        for (size_t i = 0; i < NUM_WIZARD_QUERY_CLASSES; ++i) {
            OUTA(QueryClass, i);
        }
#undef OUT
#undef OUTA
    }

    class TWizardFactorsCalculator::TImpl {
        class ICalcPolicy {
        public:
            virtual ~ICalcPolicy() {
            }

            virtual void CalcFactors(TSearchFields* searchFields, TSelfFlushLogFramePtr log, TValues& values) const = 0;
        };

        class TRemoteCalc : public ICalcPolicy {
        public:
            TRemoteCalc(const TWizardConfig* cfg)
                : Config(cfg)
            {
                RemoteWizard.Init(*cfg);
            }

            void CalcFactors(TSearchFields* searchFields, TSelfFlushLogFramePtr /*log*/, TValues& values) const override
            {
                Replace(searchFields->CgiParam, TStringBuf("rwr"), Config->GetDefaultRulesList());

                TAutoPtr<TWizardResults> remoteRes;
                try {
                    RemoteWizard.Process(searchFields);
                    remoteRes.Reset(TWizardResultsCgiPacker::Deserialize(&searchFields->CgiParam).Release());

                } catch (const TRemoteWizard::TConnectionError&) {
                    values.WizardError = true;
                    throw;
                }

                FillWizardFactors(values, remoteRes->RulesResults.Get(), searchFields);

                const TString& treeStr = searchFields->CgiParam.Get(TStringBuf("qtree"));
                TRichTreePtr richTree = DeserializeRichTree(DecodeRichTreeBase64(treeStr));

                TSyntaxFactorsCalc::Apply(values, richTree->Root.Get());
                values.RequestLength = NormalizeRequestAggressively(richTree).size();
                values.NumWords = CalcNumWords(richTree->Root);
            }
        private:
            static size_t CalcNumWords(const TRichNodePtr& tree) {
                if (!tree.Get())
                    return 0;

                size_t res = 0;
                for (TRichNodeIterator it(tree); !it.IsDone(); ++it) {
                    if (IsWord(*it))
                        ++res;
                }
                return res;
            }

            const TWizardConfig* Config;
            TRemoteWizard RemoteWizard;
        };


            void DoCalcFactors(TSearchFields& fields, TValues& values) const {
                CalcPolicy->CalcFactors(&fields, TSelfFlushLogFramePtr(), values);
            }

            static void Replace(TCgiParameters& p, const TStringBuf& key, const TString& value) {
                p.EraseAll(key);
                p.InsertUnescaped(key, value);
            }

        public:
            TImpl(const TAutoPtr<TWizardConfig>& cfg)
                : WizardConfig(cfg)
            {
                CalcPolicy.Reset(new TRemoteCalc(WizardConfig.Get()));
            }

            void CalcFactors(const TString& reqText, const THttpCookies& cookies,
                             TValues& values, const TStringBuf reqid) const {
                TSearchFields fields;
                TCgiParameters& cgi = fields.CgiParam;

                cgi.InsertUnescaped(TStringBuf("yandexuid"), GetYandexUid(cookies));
                cgi.InsertUnescaped(TStringBuf("login"), cookies.Get(TStringBuf("yandex_login")));
                cgi.InsertUnescaped(TStringBuf("reqid"), reqid);

                try {
                    values.IsUtf8 = IsUtf(reqText);
                    TString textUtf = values.IsUtf8 ? reqText : ConvertToUtf8(reqText);
                    if (textUtf.empty())
                        return;

                    cgi.InsertUnescaped(TStringBuf("text"), textUtf);
                    cgi.InsertUnescaped(TStringBuf("user_request"), textUtf);

                    DoCalcFactors(fields, values);
                } catch(...) {
                    EVLOG_MSG << "Exception in remote wizard: "sv << CurrentExceptionMessage() << Endl;
                    if (!values.WizardError)
                        values.SyntaxError = true;
                }
            }

        private:
            TAutoPtr<TWizardConfig> WizardConfig;
            THolder<const ICalcPolicy> CalcPolicy;
    };

    TWizardFactorsCalculator::TWizardFactorsCalculator(bool useOnlyIps)
        : Wizard(new TWizardFactorsCalculator::TImpl(ANTIROBOT_DAEMON_CONFIG.GetWizardConfig(Cerr, useOnlyIps)))
    {
    }

    TWizardFactorsCalculator::~TWizardFactorsCalculator() {
    }

    void TWizardFactorsCalculator::CalcFactors(const TString& reqText, const THttpCookies& cookies,
                                               TValues& values, const TStringBuf reqid) const {
        Wizard->CalcFactors(reqText, cookies, values, reqid);
    }
}
