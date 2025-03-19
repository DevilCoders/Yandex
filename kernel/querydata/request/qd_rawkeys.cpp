#include "qd_rawkeys.h"
#include "qd_genrequests.h"
#include "qd_key_patcher.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>

namespace NQueryData {

    const ui32 SUBKEYS_CACHE_SIZE = KT_COUNT - FAKE_KT_COUNT;

    void TRawMemoryTuple::Append(TStringBuf subkey) {
        Y_ENSURE(Size() + subkey.length() < MaxLen, "exceeded memory");
        memcpy(End, subkey.data(), subkey.size());
        End += subkey.length();
    }

    static EKeyType GetKeyType(int kt) {
        if (kt == FAKE_KT_STRUCTKEY_ANY || kt == FAKE_KT_STRUCTKEY_ORDERED) {
            return KT_STRUCTKEY;
        }

        return (EKeyType) (kt >= 0 ? kt : KT_COUNT - kt);
    }

    void FillSourceFactorsKeys(TSourceFactors& facts, const TRawMemoryTuple& tuple, const TStringBufs& keys, const TKeyTypes& keyTypes, bool patchKey) {
        Y_ENSURE(keys.size() > 0, "empty keys");
        Y_ENSURE(keyTypes.size() > 0, "empty keyTypes");
        Y_ENSURE(keys.size() == keyTypes.size(), "keys size (" << keys.size() << ") != keyTypes size (" << keyTypes.size() << ")");
        Y_ENSURE(!facts.HasSourceKey() && !facts.HasSourceKeyType() && !facts.HasSourceKeyTraits() && !facts.SourceSubkeysSize(), "already filled message");

        {
            TString key = TString{keys[0]};
            int kt = GetKeyType(keyTypes[0]);
            if (patchKey) {
                PatchKey(key, kt);
            }
            facts.SetSourceKey(key);
            facts.SetSourceKeyType((EKeyType)kt);
            facts.MutableSourceKeyTraits()->SetIsPrioritized(IsPrioritizedNormalization(keyTypes[0]));
            facts.MutableSourceKeyTraits()->SetMustBeInScheme(NormalizationNeedsExplicitKeyInResult(kt));
        }

        for (ui32 i = 1, sz = keyTypes.size(); i < sz; ++i) {
            TString key = TString{keys[i]};
            int kt = GetKeyType(keyTypes[i]);
            if (patchKey) {
                PatchKey(key, kt);
            }
            TSourceSubkey* sk = facts.AddSourceSubkeys();
            sk->SetKey(key);
            sk->SetType((EKeyType)kt);
            sk->MutableTraits()->SetIsPrioritized(IsPrioritizedNormalization(keyTypes[i]));
            sk->MutableTraits()->SetMustBeInScheme(NormalizationNeedsExplicitKeyInResult(kt));
        }

        if (tuple.HasPriorities()) {
            facts.MutableMergeTraits()->SetPriority(tuple.GetPriority());
        }
    }


    class TTupleGenerator {
    private:
        const TVector<ui32>& Dimensions;
        TVector<ui32> Tuple;
        ui32 CurrentDimension = 0;

    public:
        TTupleGenerator(const TVector<ui32>& dimensions)
            : Dimensions(dimensions)
            , Tuple(dimensions.size(), 0)
        {
        }

        void Next() {
            const size_t dimensionsSz = Dimensions.size();
            while (CurrentDimension < dimensionsSz && Tuple[CurrentDimension] + 1 == Dimensions[CurrentDimension]) {
                Tuple[CurrentDimension++] = 0;
            }
            if (CurrentDimension == dimensionsSz) {
                return;
            }
            ++Tuple[CurrentDimension];
            CurrentDimension = 0;
        }

        const TVector<ui32>& GetTuple() const {
            return Tuple;
        }
    };

    TSubkeysCache::TSubkeysCache() {
        ResetCache();
    }

    bool TSubkeysCache::FillSubkeys(TKeyTypes& keyTypes, const TRequestRec& req, TStringBuf nSpace) {
        return FillSubkeysCache(*this, keyTypes, req, nSpace);
    }

    bool TSubkeysCache::KeysNeedPrefixPruning(const TKeyTypes& keyTypes) const {
        if (keyTypes.size() < 2) {
            return false;
        }

        for (ui32 i = 1, sz = keyTypes.size(); i < sz; ++i) {
            if (GetSubkeys(keyTypes[i]).size() > 1) {
                return true;
            }
        }

        return false;
    }

    const TStringBufs& TSubkeysCache::GetAllPrefixes(const TKeyTypes& keyTypes) const {
        Y_ENSURE(!keyTypes.empty() && !SubkeysCache.empty(), "OOPS");
        return GetSubkeys(keyTypes[0]);
    }

    void TSubkeysCache::ResetCache() {
        SubkeysCache.clear();
        SubkeysCache.resize(SUBKEYS_CACHE_SIZE);
        CacheStringPool.clear();
    }

    void TSubkeysCache::ClearFakeSubkeys() {
        for (int i = -1; i > FAKE_KT_COUNT; --i) {
            GetSubkeysMutable(i).clear();
        }
    }

    int TSubkeysCache::FixType(int type) {
        Y_ENSURE(type > FAKE_KT_COUNT && type < KT_COUNT, " invalid type: " << type);

        if (type >= 0) {
            return type;
        }

        if (FAKE_KT_STRUCTKEY_ANY == type || FAKE_KT_STRUCTKEY_ORDERED == type) {
            return KT_STRUCTKEY;
        }

        return KT_COUNT - type;
    }

    void TSubkeysCache::MakeFakeTuple(TRawTuples& tuples, TStringBuf rawquery) const {
        tuples.push_back(TRawMemoryTuple(rawquery));
    }

    void TSubkeysCache::FillTuplesCache(TRawTuplesCache& cache, const TKeyTypes& keyTypes, bool goodPrefixes) const {
        ui32 maxTupleSz = 0;
        ui32 tuplesNumber = 1;
        TVector<ui32> dimensions(keyTypes.size());

        for (ui32 i = 0, sz = keyTypes.size(); i < sz; ++i) {
            const TStringBufs& c = (goodPrefixes && !i) ? cache.GoodPrefixes : GetSubkeys(keyTypes[i]);

            ui32 max = 0;
            for (TStringBufs::const_iterator it = c.begin(); it != c.end(); ++it) {
                max = Max<ui32>(max, it->size());
            }

            maxTupleSz += 1 + max;
            tuplesNumber *= c.size();
            dimensions[i] = c.size();
        }

        bool needsSort = false;

        TTupleGenerator gen(dimensions);
        cache.Tuples.reserve(tuplesNumber);
        for (ui32 t = 0; t < tuplesNumber; ++t, gen.Next()) {
            char* buffer = (char*) cache.TuplePool.Allocate(maxTupleSz);
            auto& rawTuple = cache.Tuples.emplace_back(buffer, maxTupleSz);
            const TVector<ui32>& tuple = gen.GetTuple();

            for (ui32 dimension = 0; dimension < tuple.size(); ++dimension) {
                ui32 sliceIndex = tuple[dimension];
                int type = keyTypes[dimension];
                const TStringBufs& slice = (goodPrefixes && !dimension) ? cache.GoodPrefixes : GetSubkeys(type);
                if (IsPrioritizedNormalization(type)) {
                    rawTuple.AddPrioritizedSubkey(slice[sliceIndex], sliceIndex);
                    needsSort = true;
                } else {
                    rawTuple.AddSubkey(slice[sliceIndex]);
                }
            }
        }

        if (needsSort) {
            Sort(cache.Tuples.begin(), cache.Tuples.end());
        }
    }

}
