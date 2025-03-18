#pragma once

#include "version.h"

#include "ngrammers.h"
#include "vectorizers.h"
#include "algorithms.h"

#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/generic/vector.h>
#include <util/string/printf.h>
#include <utility>

#include <util/system/defaults.h>
#include <util/system/yassert.h>

template <class T>
T SimhashFromVector(const TVector<bool>& v) {
    const size_t size = v.size();
    Y_VERIFY((sizeof(T) * 8 >= size), "To small type");
    T res = 0;
    for (size_t i = 0; i < size; ++i) {
        res <<= 1;
        res |= (v[size - i - 1] ? 1 : 0);
    }
    return res;
}

template <class T>
void VectorFromSimhash(T s, TVector<bool>* v, const size_t bits = sizeof(T) * 8) {
    Y_VERIFY((v != nullptr), "v == NULL");
    v->clear();
    v->resize(bits, false);
    for (size_t i = 0; i < bits; ++i) {
        if (s & 0x1) {
            v->operator[](bits - i - 1) = true;
        }
        s >>= 1;
    }
}

inline ui32 MakeVersion(const TSimHashVersion& V) {
    ui32 res = V.MethodVersion;
    res <<= 5;
    res |= V.NGrammVersion;
    res <<= 12;
    res |= V.RandomVersion;
    res <<= 10;
    res |= V.VectorizerVersion;

    return res;
}

inline TSimHashVersion ParseVersion(ui32 V) {
    TSimHashVersion res;
    res.VectorizerVersion = V & (0x3FF);
    V >>= 10;
    res.RandomVersion = V & (0xFFF);
    V >>= 12;
    res.NGrammVersion = V & (0x1F);
    V >>= 5;
    res.MethodVersion = V & (0x1F);

    return res;
}

inline TString VersionToString(const TSimHashVersion& V) {
    return Sprintf("N%u.V%u.A%u.R%u",
                   V.NGrammVersion,
                   V.VectorizerVersion,
                   V.MethodVersion,
                   V.RandomVersion);
}

#define DEFAULT_SIMHASH_VERSION (0)

template <
    template <class> class TNGrammer,
    template <class> class TVectorizer,
    class TAlgorithm>
TSimHashVersion GetVersion(ui32 randomVersion = 0) {
    TSimHashVersion res;
    res.NGrammVersion = TNGrammer<ui32>::GetVersion();
    res.VectorizerVersion = TVectorizer<ui32>::GetVersion();
    res.MethodVersion = TAlgorithm::GetVersion();
    res.RandomVersion = randomVersion;

    return res;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
/// !!!WARNING!!! Error prone concept. Updated by hands. Be carefull.
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

template <class T>
std::pair<ui32, TString> MakeDescription() {
    return std::make_pair(T::GetVersion(), T::GetDescription());
}

inline void ListNGrammers(TVector<std::pair<ui32, TString>>* ngrammers) {
    Y_VERIFY((ngrammers != nullptr), "ngrammers == NULL");
    ngrammers->clear();
    ngrammers->push_back(MakeDescription<TBigrammer<ui32>>());
    ngrammers->push_back(MakeDescription<TUnigrammer<ui32>>());
}

inline void ListVectorizers(TVector<std::pair<ui32, TString>>* vectorizers) {
    Y_VERIFY((vectorizers != nullptr), "vectorizers == NULL");
    vectorizers->clear();
    //vectorizers->push_back(MakeDescription< TDummyVectorizer<ui32> >());
    //vectorizers->push_back(MakeDescription< TDummyVectorizer2<ui32> >());
    //vectorizers->push_back(MakeDescription< TDummyVectorizer3<ui32> >());
    vectorizers->push_back(MakeDescription<TDummyVectorizer4<ui32>>());
}

inline void ListAlgorithms(TVector<std::pair<ui32, TString>>* algorithms) {
    Y_VERIFY((algorithms != nullptr), "algorithms == NULL");
    algorithms->clear();
    algorithms->push_back(MakeDescription<TInnerProductSimhash>());
}

namespace NSelectorsPrivate {
    inline bool GetDescription(
        const TVector<std::pair<ui32, TString>>& descriptions,
        const ui32 version,
        TString* description) {
        Y_VERIFY((description != nullptr), "description == NULL");
        for (size_t i = 0; i < descriptions.size(); ++i) {
            if (descriptions[i].first == version) {
                *description = descriptions[i].second;
                return true;
            }
        }
        return false;
    }

    template <
        template <class> class TNGrammer,
        template <class> class TVectorizer,
        class TAlgorithm,
        template <
            template <class> class,
            template <class> class,
            class>
        class TWorker,
        class TConfig>
    void Work(const TSimHashVersion& Version, const TConfig& Config) {
        TWorker<TNGrammer, TVectorizer, TAlgorithm> worker(Version, Config);
        worker.Work();
    }

    template <
        template <class> class TNGrammer,
        template <class> class TVectorizer,
        template <
            template <class> class,
            template <class> class,
            class>
        class TWorker,
        class TConfig>
    void SelectAlgorithm(const TSimHashVersion& Version, const TConfig& Config) {
        if (Version.MethodVersion == TInnerProductSimhash::GetVersion()) {
            Work<TNGrammer, TVectorizer, TInnerProductSimhash, TWorker, TConfig>(Version, Config);
        } else {
            ythrow yexception() << "No algorithm with version " << Version.MethodVersion;
        }
    }

    template <
        template <class> class TNGrammer,
        template <
            template <class> class,
            template <class> class,
            class>
        class TWorker,
        class TConfig>
    void SelectVectorizer(const TSimHashVersion& Version, const TConfig& Config) {
        /*if (Version.VectorizerVersion == TDummyVectorizer<ui32>::GetVersion()) {
            SelectAlgorithm<TNGrammer, TDummyVectorizer, TWorker, TConfig>(Version, Config);
        } else if (Version.VectorizerVersion == TDummyVectorizer2<ui32>::GetVersion()) {
            SelectAlgorithm<TNGrammer, TDummyVectorizer2, TWorker, TConfig>(Version, Config);
        } else if (Version.VectorizerVersion == TDummyVectorizer3<ui32>::GetVersion()) {
            SelectAlgorithm<TNGrammer, TDummyVectorizer3, TWorker, TConfig>(Version, Config);
        } else*/ if (Version.VectorizerVersion == TDummyVectorizer4<ui32>::GetVersion()) {
            SelectAlgorithm<TNGrammer, TDummyVectorizer4, TWorker, TConfig>(Version, Config);
        } else {
            ythrow yexception() << "No vectorizer with version " << Version.VectorizerVersion;
        }
    }

    template <
        template <
            template <class> class,
            template <class> class,
            class>
        class TWorker,
        class TConfig>
    void SelectNGrammer(const TSimHashVersion& Version, const TConfig& Config) {
        if (Version.NGrammVersion == TBigrammer<ui32>::GetVersion()) {
            SelectVectorizer<TBigrammer, TWorker, TConfig>(Version, Config);
        } else if (Version.NGrammVersion == TUnigrammer<ui32>::GetVersion()) {
            SelectVectorizer<TUnigrammer, TWorker, TConfig>(Version, Config);
        } else {
            ythrow yexception() << "No ngrammer with version " << Version.NGrammVersion;
        }
    }
}

template <
    template <
        template <class> class,
        template <class> class,
        class>
    class TWorker,
    class TConfig>
void SelectCalculator(const TSimHashVersion& Version, const TConfig& Config) {
    NSelectorsPrivate::SelectNGrammer<TWorker, TConfig>(Version, Config);
}

inline bool GetNGrammerDescription(ui32 version, TString* description) {
    Y_VERIFY((description != nullptr), "description == NULL");

    TVector<std::pair<ui32, TString>> ngrammers;
    ListNGrammers(&ngrammers);
    return NSelectorsPrivate::GetDescription(ngrammers, version, description);
}

inline bool GetVectorizerDescription(ui32 version, TString* description) {
    Y_VERIFY((description != nullptr), "description == NULL");

    TVector<std::pair<ui32, TString>> vectorizers;
    ListVectorizers(&vectorizers);
    return NSelectorsPrivate::GetDescription(vectorizers, version, description);
}

inline bool GetAlgorithmDescription(ui32 version, TString* description) {
    Y_VERIFY((description != nullptr), "description == NULL");

    TVector<std::pair<ui32, TString>> algorithms;
    ListAlgorithms(&algorithms);
    return NSelectorsPrivate::GetDescription(algorithms, version, description);
}
