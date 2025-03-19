#pragma once

#include "cascade_item.h"
#include "transform_params.h"

#include <kernel/remorph/engine/char/char.h>
#include <kernel/remorph/common/gztoccurrence.h>
#include <kernel/remorph/common/verbose.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

namespace NCascade {

template <class TSymbolPtr>
void Transfrom(const TCascadeBase& cascades, TTransformParams<TSymbolPtr>& params);

class TCharCascadeItem: public TCascadeItemT<NMatcher::MT_CHAR>, public NReMorph::TCharEngine {
public:
    TString SubMatch;

private:
    void InitFromArticle(const TArticlePtr& a);

public:
    TCharCascadeItem() {
    }

    void Init(const TArticlePtr& a, const NGzt::TGazetteer& gzt) override;
    void Save(IOutputStream& out) const override;
    void Load(IInputStream& in) override;
    NMatcher::TMatcherBase& GetMatcher() override {
        return *this;
    }
    const NMatcher::TMatcherBase& GetMatcher() const override {
        return *this;
    }

    size_t GetMainWord(const NReMorph::TCharResult&) const {
        return 0;
    }
    NRemorph::TSubmatch GetMatchRange(const NReMorph::TCharResult& res) const;
    void GetNamedSubRanges(const NReMorph::TCharResult& res, const NRemorph::TSubmatch& matchRange,
        NRemorph::TNamedSubmatches& namedSubRanges) const;

    static TCascadeItemPtr LoadItemFromFile(const NGzt::TGazetteer& gzt, const TString& baseDir,
        const TArticlePtr& a, const TString& file);
};

} // NCascade
