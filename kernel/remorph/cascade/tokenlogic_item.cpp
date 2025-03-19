#include "tokenlogic_item.h"

#include <kernel/remorph/common/article_util.h>
#include <kernel/gazetteer/common/reflectioniter.h>
#include <kernel/gazetteer/gztarticle.h>

#include <util/generic/ptr.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace NCascade {

const static TString FIELD_NAMEDTOKEN = "namedToken";
const static TString FIELD_HEAD_TOKENS = "headTokens";

void TTokenLogicCascadeItem::InitFromArticle(const TArticlePtr& a) {
    // Get main word from the special remorph article. Use the first word by default.
    // Negative values specify the offset from the last token:
    // -1 refers to the last token, -2 refers to the previous of the last one, and so on.
    NGztSupport::GetMainGztWord(a, MainWord);

    a.GetField<TString, const TString&>(FIELD_NAMEDTOKEN, NamedToken);

    HeadTokens.clear();
    for (NGzt::TProtoFieldIterator<TString> i = a.IterField<TString, const TString&>(FIELD_HEAD_TOKENS); i.Ok(); ++i) {
        HeadTokens.push_back(*i);
    }
}

void TTokenLogicCascadeItem::Init(const TArticlePtr& a, const NGzt::TGazetteer& gzt) {
    NTokenLogic::TMatcher::Init(&gzt);
    InitSubCascades(gzt);
    TCascadeItem::Init(a, gzt);
    InitFromArticle(a);
}

void TTokenLogicCascadeItem::Save(IOutputStream& out) const {
    TCascadeItem::Save(out);
    NTokenLogic::TMatcher::SaveToStream(out);
}

void TTokenLogicCascadeItem::Load(IInputStream& in) {
    TCascadeItem::Load(in);
    NTokenLogic::TMatcher::LoadFromStream(in, true);
}

TCascadeItemPtr TTokenLogicCascadeItem::LoadItemFromFile(const NGzt::TGazetteer& gzt, const TString& baseDir,
    const TArticlePtr& a, const TString& file) {

    THolder<TTokenLogicCascadeItem> res(new TTokenLogicCascadeItem());

    res->LoadSubCascadeFromFile(file, gzt, baseDir);
    res->TCascadeItem::Init(a, gzt);
    res->InitFromArticle(a);

    return res.Release();
}

size_t TTokenLogicCascadeItem::GetMainWord(const NTokenLogic::TTokenLogicResult& res) const {
    typedef NTokenLogic::TNamedTokens::const_iterator nt_iter;

    NSorted::TSortedVector<size_t> heads;
    // Get used sub-match region if it is specified
    if (!NamedToken.empty()) {
        std::pair<nt_iter, nt_iter> range = res.NamedTokens.equal_range(NamedToken);
        for (nt_iter i = range.first; i != range.second; ++i) {
            heads.insert_unique(i->second);
        }
    }
    // Try to find one of the specified head tokens
    // It must be equal to one of the named tokens
    if (heads.size() > 1 && !HeadTokens.empty()) {
        for (TVector<TString>::const_iterator i = HeadTokens.begin(); i != HeadTokens.end() && heads.size() > 1; ++i) {
            std::pair<nt_iter, nt_iter> range = res.NamedTokens.equal_range(*i);
            NSorted::TSortedVector<size_t> newHeads;
            for (nt_iter j = range.first; j != range.second; ++j) {
                if (heads.has(j->second))
                    newHeads.insert_unique(j->second);
            }
            if (!newHeads.empty()) {
                DoSwap(newHeads, heads);
                break;
            }
        }
    }
    const size_t headRange = heads.empty() ? res.GetMatchedCount() : heads.size();
    size_t offset = 0;
    if (MainWord < 0) {
        offset = -MainWord <= i32(headRange) ? headRange + MainWord : 0;
    } else {
        offset = MainWord < i32(headRange) ? MainWord : headRange - 1;
    }
    return heads.empty() ? offset : heads[offset];
}

NRemorph::TSubmatch TTokenLogicCascadeItem::GetMatchRange(const NTokenLogic::TTokenLogicResult& res) const {
    NRemorph::TSubmatch matchRange(0, res.GetMatchedCount());
    if (!NamedToken.empty()) {
        std::pair<NTokenLogic::TNamedTokens::const_iterator, NTokenLogic::TNamedTokens::const_iterator> range = res.NamedTokens.equal_range(NamedToken);
        if (range.first != range.second) {
            matchRange.first = matchRange.second = -1;
            for (NTokenLogic::TNamedTokens::const_iterator i = range.first; i != range.second; ++i) {
                if (matchRange.IsEmpty()) {
                    matchRange.first = i->second;
                    matchRange.second = i->second + 1;
                } else {
                    matchRange.first = Min(matchRange.first, i->second);
                    matchRange.second = Max(matchRange.second, i->second + 1);
                }
            }
        }
    }
    return matchRange;
}

void TTokenLogicCascadeItem::GetNamedSubRanges(const NTokenLogic::TTokenLogicResult& res, const NRemorph::TSubmatch& matchRange,
    NRemorph::TNamedSubmatches& namedSubRanges) const {

    if (NamedToken.empty()) {
        res.GetNamedRanges(namedSubRanges);
    } else {
        for (NTokenLogic::TNamedTokens::const_iterator iSub = res.NamedTokens.begin(); iSub != res.NamedTokens.end(); ++iSub) {
            // Insert only sub-matches, which falls to the matched range
            if (iSub->second >= matchRange.first && iSub->second < matchRange.second) {
                // Use relative coordinates
                namedSubRanges.insert(std::make_pair(iSub->first, NRemorph::TSubmatch(iSub->second - matchRange.first, iSub->second + 1 - matchRange.first)));
            }
        }
    }
}

} // NCascade
