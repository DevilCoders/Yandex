#pragma once

#include "cascade_base.h"
#include "transform.h"

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/remorph/core/core.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/string/vector.h>
#include <util/stream/output.h>

namespace NCascade {

template <class TRootMatcher>
class TCascade: protected TCascadeBase, public TRootMatcher {
public:
    typedef typename TRootMatcher::TResult TResult;
    typedef typename TRootMatcher::TResultPtr TResultPtr;
    typedef typename TRootMatcher::TResults TResults;

    typedef TIntrusivePtr<TCascade<TRootMatcher>> TPtr;

protected:
    TCascade() {
    }

    NMatcher::TMatcherBase& GetMatcher() override {
        return *static_cast<TRootMatcher*>(this);
    }
    const NMatcher::TMatcherBase& GetMatcher() const override {
        return *static_cast<const TRootMatcher*>(this);
    }

    template <class TSymbolPtr>
    inline void UpdateInput(NRemorph::TInput<TSymbolPtr>& symbols) const {
        if (!SubCascades.empty()) {
            TTransformParams<TSymbolPtr> params(symbols, GetMatcher().GetDominantArticles());
            Transfrom(*this, params);
        }
    }

    template <class TSymbolPtr>
    inline void UpdateInput(TVector<TSymbolPtr>&) const {
        if (!SubCascades.empty()) {
            throw yexception() << "Cascaded rules can only be used with the NRemorph::TInput input.";
        }
    }

public:
    using TCascadeBase::SetResolveCascadeAmbiguity;
    using TCascadeBase::GetResolveCascadeAmbiguity;
    using TCascadeBase::SetCascadeRankMethod;
    using TCascadeBase::GetCascadeRankMethod;

    void SaveToStream(IOutputStream& out) const override {
        if (SubCascades.empty()) {
            TRootMatcher::SaveToStream(out);
        } else {
            SavePrefix(out);
            TRootMatcher::SaveToStream(out);
            SaveData(out);
        }
    }

#define FORWARD(F) \
    template <class TInputSource>                                                                                     \
    inline TResultPtr F(TInputSource& symbols, NRemorph::OperationCounter* opcnt = nullptr) const {                   \
        UpdateInput(symbols);                                                                                         \
        return TRootMatcher::F(symbols, opcnt);                                                                       \
    }                                                                                                                 \
                                                                                                                      \
    template <class TInputSource>                                                                                     \
    inline void F##All(TInputSource& symbols, TResults& results, NRemorph::OperationCounter* opcnt = nullptr) const { \
        UpdateInput(symbols);                                                                                         \
        TRootMatcher::F##All(symbols, results, opcnt);                                                                \
    }

    FORWARD(Match)
    FORWARD(Search)
#undef FORWARD

    // Load rules from the specified file. Resolves paths to cascaded rules relatively to the given rule file.
    // The Gazetteer reference is required only for article names validation. The cascade doesn't keep the reference to the gazetteer
    static TPtr Load(const TString& filePath, const NGzt::TGazetteer* gzt) {
        TPtr res = new TCascade<TRootMatcher>();
        res->LoadRootCascadeFromFile(filePath, gzt);
        return res;

    }
    // Load rules from the specified file. Resolves paths to cascaded rules relatively to the specified baseDir
    // The Gazetteer reference is required only for article names validation. The cascade doesn't keep the reference to the gazetteer
    static TPtr Load(const TString& filePath, const NGzt::TGazetteer* gzt, const TString& baseDir) {
        TPtr res = new TCascade<TRootMatcher>();
        res->LoadRootCascadeFromFile(filePath, gzt, baseDir);
        return res;
    }
    // Load rules from stream.
    // The Gazetteer reference is required only for article names validation. The cascade doesn't keep the reference to the gazetteer
    static TPtr Load(IInputStream& data, const NGzt::TGazetteer* gzt) {
        TPtr res = new TCascade<TRootMatcher>();
        res->LoadRootCascadeFromStream(data, gzt);
        return res;

    }
};

} // NCascade
