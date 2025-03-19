#pragma once

#include "cascade_item.h"
#include "transform_params.h"

#include <kernel/remorph/tokenlogic/tlmatcher.h>
#include <kernel/remorph/common/gztoccurrence.h>
#include <kernel/remorph/common/verbose.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

namespace NCascade {

class TTokenLogicCascadeItem: public TCascadeItemT<NMatcher::MT_TOKENLOGIC>, public NTokenLogic::TMatcher {
public:
    i32 MainWord;
    TString NamedToken;
    TVector<TString> HeadTokens;

private:
    void InitFromArticle(const TArticlePtr& a);

public:
    TTokenLogicCascadeItem()
        : MainWord(0)
    {
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

    size_t GetMainWord(const NTokenLogic::TTokenLogicResult& res) const;
    NRemorph::TSubmatch GetMatchRange(const NTokenLogic::TTokenLogicResult& res) const;
    void GetNamedSubRanges(const NTokenLogic::TTokenLogicResult& res, const NRemorph::TSubmatch& matchRange,
        NRemorph::TNamedSubmatches& namedSubRanges) const;

    static TCascadeItemPtr LoadItemFromFile(const NGzt::TGazetteer& gzt, const TString& baseDir,
        const TArticlePtr& a, const TString& file);
};

} // NCascade
