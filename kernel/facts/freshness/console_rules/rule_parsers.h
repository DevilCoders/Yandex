#pragma once

#include <util/generic/fwd.h>
#include <util/generic/yexception.h>

namespace NFacts {

    class IConsoleRuleDestination {
    public:
        virtual ~IConsoleRuleDestination() {}

        // key includes uil and region (may be defaults), i.e. "адрес деда мороза|ru|213" or "животное на букву у|0|0"
        virtual void AddNewFact(const TString& key, const TString& answer, const TString& canonicalForm, const TString& headline, const TString& url) = 0;
        virtual void AddBanByQuery(const TString& query) = 0;
        virtual void AddBanByQueryAndType(const TString& query, const TString& type) = 0;
        virtual void AddBanByAnswerAndUrl(const TString& answer, const TString& url) = 0;
        virtual void AddImageBanByAnswerAndUrl(const TString& answer, const TString& url) = 0;
        virtual void AddBanByUrl(const TString& url) = 0;
        // wordSet is a string of words separated by spaces
        virtual void AddBanByWordSet(const TString& wordSet) = 0;
        virtual void AddRuntimeAliasBanByQuery(const TString& query) = 0;
        virtual void AddNewRuntimeAlias(const TString& query, const TString& alias) = 0;
        virtual void Finalize() = 0;
    };

    class TFreshnessConsoleRuleParser final {
    public:
        static bool ParseFields(const TVector<TString>& fields, IConsoleRuleDestination& dst);
        static void ParserFinished(IConsoleRuleDestination& dst);
    };
}
