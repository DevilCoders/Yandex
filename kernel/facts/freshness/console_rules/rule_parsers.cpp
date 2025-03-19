#include "rule_parsers.h"

#include <util/string/split.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NFacts {

    bool TFreshnessConsoleRuleParser::ParseFields(const TVector<TString>& fields, IConsoleRuleDestination& dst) {
        const TString op = fields[0];
        if (op == "add_fact") {
            Y_ENSURE(3 <= fields.size() && fields.size() <= 6, "add_fact operation requires 3, 4, 5, 6 fields");

            const TString answer = fields[2];

            TString canonicalForm;
            if (fields.size() >= 4 && fields[3].size()) {
                canonicalForm = fields[3];
            }

            TString headline;
            if (fields.size() >= 5 && fields[4].size()) {
                headline = fields[4];
            }

            TString url;
            if (fields.size() >= 6 && fields[5].size()) {
                url = fields[5];
            }

            Y_ENSURE(url.Empty() == headline.Empty());

            TString key;
            const TString& fact_full = fields[1];
            TVector<TStringBuf> factProperties;
            Split(fact_full, "|", factProperties);
            if (factProperties.size() == 1) {
                key += fact_full + "|0|0";
            } else if (factProperties.size() == 3) {
                key = fact_full;
            } else {
                ythrow yexception() << "invalid fact key: expected 1 or 3 segments, got " << factProperties.size() << "(" << fact_full << ")";
            }
            dst.AddNewFact(key, answer, canonicalForm, headline, url);
        } else if (op == "ban_fact") {
            Y_ENSURE(fields.size() == 2 || fields.size() == 3, "ban_fact operation requires 2 or 3 fields");
            if(fields.size() < 3) {
                dst.AddBanByQuery(fields[1]);
            } else {
                dst.AddBanByAnswerAndUrl(fields[1], fields[2]);
            }
        } else if (op == "ban_fact_image") {
            Y_ENSURE(fields.size() == 3, "ban_fact_image operation requires 3 fields");
            dst.AddImageBanByAnswerAndUrl(fields[1], fields[2]);
        } else if (op == "ban_fact_type") {
            Y_ENSURE(fields.size() == 3, "ban_fact_type operation requires 3 fields");
            dst.AddBanByQueryAndType(fields[1], fields[2]);
        } else if (op == "add_runtime_alias") {
            Y_ENSURE(fields.size() == 3, "add_runtime_alias requires 3 fields");
            dst.AddNewRuntimeAlias(fields[1], fields[2]);
        } else if (op == "ban_runtime_alias") {
            Y_ENSURE(fields.size() == 2, "ban_runtime_alias requires 2 fields");
            dst.AddRuntimeAliasBanByQuery(fields[1]);
        } else if (op == "ban_fact_by_url") {
            Y_ENSURE(fields.size() == 2, "ban_fact_by_url requires 2 fields");
            dst.AddBanByUrl(fields[1]);
        } else if (op == "ban_fact_by_word_set") {
            Y_ENSURE(fields.size() == 2, "ban_fact_by_word_set requires 2 fields");
            dst.AddBanByWordSet(fields[1]);
        } else if (op.Contains("fact") || op.Contains("runtime_alias")) {
            // TODO: FACTS-1609 - remove this hack as soon as all new fact FConsole rules are deployed to production
        } else {
            return false;
        }
        return true;
    }

    void TFreshnessConsoleRuleParser::ParserFinished(IConsoleRuleDestination& dst) {
        dst.Finalize();
    }

}
