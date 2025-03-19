#include "char_item.h"

#include <kernel/remorph/common/article_util.h>
#include <kernel/gazetteer/common/reflectioniter.h>
#include <kernel/gazetteer/gztarticle.h>

#include <util/generic/ptr.h>

namespace NCascade {

using namespace NReMorph;

const static TString FIELD_SUBMATCH = "submatch";

void TCharCascadeItem::InitFromArticle(const TArticlePtr& a) {
    a.GetField<TString, const TString&>(FIELD_SUBMATCH, SubMatch);
}

void TCharCascadeItem::Init(const TArticlePtr& a, const NGzt::TGazetteer& gzt) {
    TCharEngine::Init(&gzt);
    InitSubCascades(gzt);
    TCascadeItem::Init(a, gzt);
    InitFromArticle(a);
}

void TCharCascadeItem::Save(IOutputStream& out) const {
    TCascadeItem::Save(out);
    TCharEngine::SaveToStream(out);
}

void TCharCascadeItem::Load(IInputStream& in) {
    TCascadeItem::Load(in);
    TCharEngine::LoadFromStream(in, true);
}

TCascadeItemPtr TCharCascadeItem::LoadItemFromFile(const NGzt::TGazetteer& gzt, const TString& baseDir,
    const TArticlePtr& a, const TString& file) {

    THolder<TCharCascadeItem> res(new TCharCascadeItem());
    res->LoadSubCascadeFromFile(file, gzt, baseDir);
    res->TCascadeItem::Init(a, gzt);
    res->InitFromArticle(a);

    return res.Release();
}

NRemorph::TSubmatch TCharCascadeItem::GetMatchRange(const TCharResult& res) const {
    NRemorph::TSubmatch matchRange(0, res.GetMatchedCount());
    if (!SubMatch.empty()) {
        NRemorph::TNamedSubmatches::const_iterator iSub = res.NamedSubmatches.find(SubMatch);
        if (iSub != res.NamedSubmatches.end()) {
            matchRange = iSub->second;
        }
    }
    return matchRange;
}

void TCharCascadeItem::GetNamedSubRanges(const TCharResult& res, const NRemorph::TSubmatch& matchRange,
    NRemorph::TNamedSubmatches& namedSubRanges) const {

    if (SubMatch.empty()) {
        res.GetNamedRanges(namedSubRanges);
    } else {
        for (NRemorph::TNamedSubmatches::const_iterator iSub = res.NamedSubmatches.begin();
            iSub != res.NamedSubmatches.end(); ++iSub) {
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
