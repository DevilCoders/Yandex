#include "indexstat_calc.h"

#include <library/cpp/json/json_writer.h>
#include <search/meta/context.h>
#include <search/meta/metasearch.h>
#include <search/meta/scatter/request.h>

namespace NRTYServer {

    void TMetaInfoIndexStatCalculator::Execute(const TMetaSearch& owner, TMetaSearchContext& ctx, const TCgiParameters&, const TRequestParams &, IOutputStream& infoBuf) {
        ui64 sentCount = 0;
        ui64 wordCount = 0;
        TMetaInfoTask::TKeyPrefixStat kps;

        NScatter::TTaskList tasks;
        for (size_t i = 0; i < owner.ClientCount(); ++i) {
            TClientInfo& ci = ctx.ClientsInfo[i];
            tasks.push_back(new TMetaInfoTask(&ci, sentCount, wordCount, kps));
        }

        ctx.RunTasks("indexstat", tasks);

        NJson::TJsonValue json;
        json["SentCount"] = sentCount;
        json["WordCount"] = wordCount;
        for (auto&& e: kps) {
            json["KPS"].InsertValue(ToString(e.first), e.second);
        }
        WriteJson(&infoBuf, &json);
    }

    TMetaInfoIndexStatCalculator::TMetaInfoTask::TMetaInfoTask(TClientInfo* ci, ui64& sentCount, ui64& wordCount, TKeyPrefixStat& kps)
        : IMetaInfoTask(ci)
        , SentCount(sentCount)
        , WordCount(wordCount)
        , Kps(kps)
    {
    }

    void TMetaInfoIndexStatCalculator::TMetaInfoTask::FillRequestData(NScatter::TRequestData& rd) const {
        rd.Url = TString("info=indexstat");
    }

    NScatter::TParseResult TMetaInfoIndexStatCalculator::TMetaInfoTask::DoParse(const NScatter::TTaskReply& reply) {
        NJson::TJsonValue json;
        TStringInput in(reply.Data);
        if (ReadJsonTree(&in, &json)) {
            NJson::TJsonValue value;
            if (json.GetValueByPath("SentCount", value)) {
                SentCount += value.GetUInteger();
            }
            if (json.GetValueByPath("WordCount", value)) {
                WordCount += value.GetUInteger();
            }

            if (json.GetValueByPath("KPS", value)) {
                if (value.IsArray()) {
                    for (auto&& e : value.GetArray())
                        Kps[e.GetIntegerRobust()] += 0;
                } else if (value.IsMap()) {
                    for (auto&& e : value.GetMap()) {
                        const i64 kps = FromString<i64>(e.first);
                        const i64 count = e.second.GetIntegerRobust();
                        Kps[kps] += count;
                    }
                }
            }
        }
        return { Finished };
    }

    IMetaInfoCalculator::TFactory::TRegistrator<TMetaInfoIndexStatCalculator> TMetaInfoIndexStatCalculator::Registrator("indexstat");

}
