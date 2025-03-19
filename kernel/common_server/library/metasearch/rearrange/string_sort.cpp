#include <search/meta/context.h>
#include <search/meta/mergedres.h>
#include <search/meta/rearrange/rearrange.h>
#include <search/web/core/rule.h>

class TStringSortRule : public IRearrangeRule {
    class TStringSortContext : public IRearrangeRuleContext {
    public:
        virtual void DoRearrangeAfterMerge(TRearrangeParams& rearrangeParams) {
            TMetaGrouping* g = rearrangeParams.Current->second;
            if (g && TStringBuf(g->GetSortMethod(), PropPrefix.length()) == PropPrefix) {
                TString attr = g->GetSortMethod() + PropPrefix.length();
                TMultiMap<TString, ui32> groups;
                for (size_t ind = 0; ind < g->Size(); ind++) {
                    TMsString attrVal;
                    const auto& group = g->GetMetaGroup(ind);
                    if (!group.MetaDocs.empty()) {
                        TSearchedDocConstRef doc = g->GetMetaGroup(ind).MetaDocs[0];
                        if (doc->FirstStageAttrValues()) {
                            for (const auto& val : doc->FirstStageAttrValues()->GetValues(attr))
                                if (!attrVal || val < attrVal)
                                    attrVal = val;
                        }
                    }
                    groups.emplace(TString(attrVal), ind);
                }
                TVector<size_t> groupsOrdered;
                groupsOrdered.resize(g->Size(), (size_t)-1);
                if (g->SP.QAscendOrder) {
                    ui32 id = 0;
                    for (TMultiMap<TString, ui32>::const_iterator i = groups.begin(), e = groups.end(); i != e; ++i, ++id) {
                        groupsOrdered[id] = i->second;
                    }
                }
                else {
                    ui32 id = 0;
                    for (TMultiMap<TString, ui32>::const_reverse_iterator i = groups.rbegin(), e = groups.rend(); i != e; ++i, ++id) {
                        groupsOrdered[id] = i->second;
                    }
                }
                g->RearrangeGroups(groupsOrdered);
            }
        }
        virtual void DoAdjustClientParams(const TAdjustParams& ap) override {
            for (const auto& grouping : ap.RP.GroupingParams) {
                if (grouping.gMode == GM_FLAT
                    && grouping.gAttrToSort.StartsWith(PropPrefix.data(), PropPrefix.size())
                    && !ap.ClientRequestAdjuster->ClientFormFieldHas("fsgta", grouping.gAttrToSort.data() + PropPrefix.length()))
                {
                        ap.ClientRequestAdjuster->ClientFormFieldInsert("fsgta", grouping.gAttrToSort.data() + PropPrefix.length());
                }
            }
        }
        const static TStringBuf PropPrefix;
    };
public:
    inline TStringSortRule(const TString& /*config*/, const TSearchConfig& /*searchConfig*/)
    {
    }

    virtual ~TStringSortRule() {
    }

    virtual IRearrangeRuleContext* DoConstructContext() const {
        return new TStringSortContext;
    }
};

const TStringBuf TStringSortRule::TStringSortContext::PropPrefix("gta_");

IRearrangeRule* CreateStringSortRule(const TString& config, const TSearchConfig& searchConfig) {
    return new TStringSortRule(config, searchConfig);
}

REGISTER_REARRANGE_RULE(StringSort, CreateStringSortRule);

