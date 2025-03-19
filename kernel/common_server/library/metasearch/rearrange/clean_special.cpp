#include <search/meta/context.h>
#include <search/meta/mergedres.h>
#include <search/meta/rearrange/rearrange.h>
#include <search/web/core/rule.h>

class TCleanSpecialRule : public IRearrangeRule {
    class TCleanSpecialContext: public IRearrangeRuleContext {
    public:
        virtual void DoAdjustClientParams(const TAdjustParams& /*params*/) {
            Disabled = LocalScheme()["Enabled"].IsExplicitFalse();
        }

        virtual void DoRearrangeAfterFetch(TRearrangeParams& rp) {
            if (Disabled)
                return;

            TMetaGrouping* g = rp.Current->second;

            Y_ASSERT(g);

            ui64 removedDocs = 0;
            ui64 removedGroups = 0;

            for (size_t ind = 0; ind < g->Size();) {
                TMetaGroup::TDocs& docs = g->GetMetaGroup(ind).MetaDocs;
                for (size_t docInd = 0; docInd < docs.size();) {
                    TFetchedDocData* docData = docs[docInd].MutableData();
                    if (docData && !docData->HeadLine && !docData->Passages.size()) {
                        docs.erase(docs.begin() + docInd);
                        ++removedDocs;
                        continue;
                    }
                    docInd++;
                }

                if (!docs.size()) {
                    g->Erase(ind);
                    ++removedGroups;
                } else {
                    ind++;
                }
            }

            rp.InsertWorkedRule("RemovedDocs", ToString(removedDocs));
            rp.InsertWorkedRule("RemovedGroups", ToString(removedGroups));
        }

    private:
        bool Disabled;
    };
public:
    inline TCleanSpecialRule(const TString& /*config*/, const TSearchConfig& /*searchConfig*/)
    {
    }

    virtual IRearrangeRuleContext* DoConstructContext() const {
        return MakeHolder<TCleanSpecialContext>().Release();
    }
};

IRearrangeRule* CreateCleanSpecialRule(const TString& config, const TSearchConfig& searchConfig) {
    return new TCleanSpecialRule(config, searchConfig);
}

REGISTER_REARRANGE_RULE(RTYCleanSpecial, CreateCleanSpecialRule);
