#pragma once

#include <kernel/remorph/proc_base/matcher_base.h>
#include <kernel/remorph/common/gztfilter.h>

#include <kernel/gazetteer/gazetteer.h>

#include <library/cpp/solve_ambig/rank.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCascade {

class TCascadeItem;
typedef TIntrusivePtr<TCascadeItem> TCascadeItemPtr;
typedef TVector<TCascadeItemPtr> TCascadeItems;

struct TCascadeBase {
    TCascadeItems SubCascades;
    NGztSupport::TGazetteerFilter CascadeArticles;
    bool ResolveCascadeAmbiguity;
    NSolveAmbig::TRankMethod CascadeRankMethod;

    TCascadeBase()
        : ResolveCascadeAmbiguity(false)
        , CascadeRankMethod(NSolveAmbig::DefaultRankMethod())
    {
    }

    virtual ~TCascadeBase() {
    }

    virtual NMatcher::TMatcherBase& GetMatcher() = 0;
    virtual const NMatcher::TMatcherBase& GetMatcher() const = 0;

    void InitSubCascades(const NGzt::TGazetteer& gzt);

    template <class TCascadeType>
    bool InitSubCascadeType(const NGzt::TGazetteer& gzt, const TString& baseDir, const NGzt::TArticlePtr& article, const TString& prefix);
    void InitSubCascadesFromArticle(const NGzt::TGazetteer& gzt, const TString& baseDir, const NGzt::TArticlePtr& article);
    void ScanCascades(const NGzt::TGazetteer& gzt, const TString& baseDir);

    void LoadRootCascadeFromFile(const TString& filePath, const TGazetteer* gzt, const TString& baseDir);
    void LoadRootCascadeFromFile(const TString& filePath, const TGazetteer* gzt);
    void LoadRootCascadeFromStream(IInputStream& in, const TGazetteer* gzt);
    void ReadCascadeData(IInputStream& in, bool signature);
    void LoadSubCascadeFromFile(const TString& filePath, const TGazetteer& gzt, const TString& baseDir);
    void SavePrefix(IOutputStream& out) const;
    void SaveData(IOutputStream& out) const;

    inline void SetResolveCascadeAmbiguity(bool resolve) {
        ResolveCascadeAmbiguity = resolve;
    }

    inline bool GetResolveCascadeAmbiguity() const {
        return ResolveCascadeAmbiguity;
    }

    inline void SetCascadeRankMethod(const NSolveAmbig::TRankMethod& cascadeRankMethod) {
        CascadeRankMethod = cascadeRankMethod;
    }

    inline const NSolveAmbig::TRankMethod& GetCascadeRankMethod() const {
        return CascadeRankMethod;
    }
};

} // NCascade
