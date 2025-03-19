#include "remorph_item.h"

#include <kernel/remorph/common/article_util.h>
#include <kernel/gazetteer/common/reflectioniter.h>
#include <kernel/gazetteer/gztarticle.h>

#include <util/generic/ptr.h>

namespace NCascade {

const static TString FIELD_SUBMATCH = "submatch";
const static TString FIELD_HEAD_SUBMATCHES = "headSubMatches";

void TRemorphCascadeItem::InitFromArticle(const TArticlePtr& a) {
    // Get main word from the special remorph article. Use the first word by default.
    // Negative values specify the offset from the last token:
    // -1 refers to the last token, -2 refers to the previous of the last one, and so on.
    NGztSupport::GetMainGztWord(a, MainWord);

    a.GetField<TString, const TString&>(FIELD_SUBMATCH, SubMatch);

    HeadSubMatches.clear();
    for (NGzt::TProtoFieldIterator<TString> i = a.IterField<TString, const TString&>(FIELD_HEAD_SUBMATCHES); i.Ok(); ++i) {
        HeadSubMatches.push_back(*i);
    }
}

void TRemorphCascadeItem::Init(const TArticlePtr& a, const NGzt::TGazetteer& gzt) {
    NReMorph::TMatcher::Init(&gzt);
    InitSubCascades(gzt);
    TCascadeItem::Init(a, gzt);
    InitFromArticle(a);
}

void TRemorphCascadeItem::Save(IOutputStream& out) const {
    TCascadeItem::Save(out);
    NReMorph::TMatcher::SaveToStream(out);
}

void TRemorphCascadeItem::Load(IInputStream& in) {
    TCascadeItem::Load(in);
    NReMorph::TMatcher::LoadFromStream(in, true);
}

TCascadeItemPtr TRemorphCascadeItem::LoadItemFromFile(const NGzt::TGazetteer& gzt, const TString& baseDir,
    const TArticlePtr& a, const TString& file) {

    THolder<TRemorphCascadeItem> res(new TRemorphCascadeItem());
    res->LoadSubCascadeFromFile(file, gzt, baseDir);
    res->TCascadeItem::Init(a, gzt);
    res->InitFromArticle(a);

    return res.Release();
}

size_t TRemorphCascadeItem::GetMainWord(const NReMorph::TMatchResult& res) const {
    NRemorph::TSubmatch headRange(0, res.GetMatchedCount());
    // Get used sub-match region if it is specified
    if (!SubMatch.empty()) {
        NRemorph::TNamedSubmatches::const_iterator iSub = res.Result->NamedSubmatches.find(SubMatch);
        if (iSub == res.Result->NamedSubmatches.end())
            return 0;
        headRange = iSub->second;
    }
    // Try to find one of the specified head sub-matches
    // It must belong to the matched region
    size_t headOffset = 0;
    if (!HeadSubMatches.empty()) {
        for (TVector<TString>::const_iterator i = HeadSubMatches.begin(); i != HeadSubMatches.end(); ++i) {
            NRemorph::TNamedSubmatches::const_iterator iSub = res.Result->NamedSubmatches.find(*i);
            if (iSub != res.Result->NamedSubmatches.end()
                && iSub->second.first >= headRange.first
                && iSub->second.second <= headRange.second) {

                headOffset = iSub->second.first - headRange.first;
                headRange = iSub->second;
                break;
            }
        }
    }
    size_t offsetInHeadSubmatch = 0;
    if (MainWord < 0) {
        offsetInHeadSubmatch = -MainWord <= i32(headRange.Size()) ? headRange.Size() + MainWord : 0;
    } else {
        offsetInHeadSubmatch = MainWord < i32(headRange.Size()) ? MainWord : headRange.Size() - 1;
    }
    return headOffset + offsetInHeadSubmatch;
}

NRemorph::TSubmatch TRemorphCascadeItem::GetMatchRange(const NReMorph::TMatchResult& res) const {
    NRemorph::TSubmatch matchRange(0, res.GetMatchedCount());
    if (!SubMatch.empty()) {
        NRemorph::TNamedSubmatches::const_iterator iSub = res.Result->NamedSubmatches.find(SubMatch);
        if (iSub != res.Result->NamedSubmatches.end()) {
            matchRange = iSub->second;
        }
    }
    return matchRange;
}

void TRemorphCascadeItem::GetNamedSubRanges(const NReMorph::TMatchResult& res, const NRemorph::TSubmatch& matchRange,
    NRemorph::TNamedSubmatches& namedSubRanges) const {

    if (SubMatch.empty()) {
        res.GetNamedRanges(namedSubRanges);
    } else {
        for (NRemorph::TNamedSubmatches::const_iterator iSub = res.Result->NamedSubmatches.begin();
            iSub != res.Result->NamedSubmatches.end(); ++iSub) {
            // Insert only sub-matches, which falls to the matched range
            if (iSub->second.first >= matchRange.first && iSub->second.second <= matchRange.second) {
                // Use relative coordinates
                namedSubRanges.insert(
                    std::make_pair(
                        iSub->first,
                        NRemorph::TSubmatch(iSub->second.first - matchRange.first, iSub->second.second - matchRange.first)
                    )
                );
            }
        }
    }
}

} // NCascade
