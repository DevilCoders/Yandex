#include "query.h"

#include <kernel/text_machine/proto/text_machine.pb.h>

namespace NTextMachine {
    void MakeDummyQuery(TQuery& query)
    {
        query = TQuery();

        static TQueryWordMatch form = {TIdfsByType{}, TMatch::OriginalWord,
            TMatchPrecision::Exact, 0, 0, 0};
        TQueryWord word;
        word.Forms.push_back(&form);
        query.ExpansionType = TExpansion::OriginalRequest;
        query.Words.push_back(word);
        query.MainLanguage = LANG_UNK;
        query.Index = 0;
        query.IndexInBundle = Max<size_t>();

        TQueryValue value;
        value.Type.RegionClass = TRegionClass::World;
        value.Value = 1.0f;
        query.Values.push_back(value);
    }

    void MakeDummyQuery(TMultiQuery& multiQuery)
    {
        multiQuery = TMultiQuery();
        TQuery query;
        MakeDummyQuery(query);
        multiQuery.Queries.push_back(query);
    }

    void TQuery::DoReconstructQuery() {
            for(const auto& w : Words) {
                Y_ENSURE(w.Text);
                ReconstructedQuery += w.Text;
                ReconstructedQuery += " ";
            }
            ReconstructedQuery.pop_back();
    }

    void TMultiQuery::ApplyWeightsRewrites(TArrayRef<const NTextMachineProtocol::TInExpansionWordWeigthsRewrite> rewrites)
    {
        struct TActionRef {
            TMaybe<ui32> ExpansionId;
            const NTextMachineProtocol::TWordWeightRewriteAction* Action;

            TActionRef(const TMaybe<ui32>& expansionId, const NTextMachineProtocol::TWordWeightRewriteAction* action)
                : ExpansionId{expansionId}
                , Action{action}
            {}
        };

        THashMap<TStringBuf, THashMap<TStringBuf, TVector<TActionRef>> > ActionsList;
        {//preparing actions list
            for(auto& r : rewrites) {
                Y_ENSURE(r.GetForExpansionText());

                TMaybe<ui32> expType;
                if (r.HasForExpansionType()) {
                    expType = r.GetForExpansionType();
                }

                auto& expDst = ActionsList[r.GetForExpansionText()];

                for(auto& wordAction : r.GetWordActions()) {
                    Y_ENSURE(wordAction.GetWord());
                    auto& dst = expDst[wordAction.GetWord()];

                    dst.emplace_back(expType, &wordAction);
                }
            }
        }

        //comparing queries with prepared list
        for(auto& q : Queries) {
            auto forTextPtr = std::as_const(ActionsList).FindPtr(q.GetReconstructedQuery());
            if (!forTextPtr) {
                continue;
            }
            for(auto& w : q.Words) {
                auto forWordPtr = forTextPtr->FindPtr(w.Text);
                if(!forWordPtr) {
                    continue;
                }
                for(const TActionRef& a : *forWordPtr) {
                    if (a.ExpansionId && a.ExpansionId != ui32(q.ExpansionType)) {
                        continue;
                    }
                    if(a.Action->HasRewriteMainWeight()) {
                        w.RewriteMainWeight = a.Action->GetRewriteMainWeight();
                    }
                    if(a.Action->HasRewriteExactWeight()) {
                        w.RewriteExactWeight = a.Action->GetRewriteExactWeight();
                    }
                }
            }
        }
    }
}
