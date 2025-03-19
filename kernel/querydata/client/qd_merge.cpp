#include "qd_merge.h"
#include "qd_key_impl.h"

#include <kernel/querydata/common/querydata_traits.h>
#include <kernel/querydata/idl/querydata_structs_client.pb.h>

#include <util/digest/city.h>
#include <util/digest/numeric.h>
#include <util/generic/hash.h>
#include <util/memory/pool.h>

namespace NQueryData {

    static ui64 GetPriority(const TSourceFactors& sf) {
        return sf.GetMergeTraits().GetPriority();
    }

    static ui64 GetTimestamp(const TSourceFactors& sf) {
        return GetTimestampMicrosecondsFromVersion(sf.GetVersion());
    }

    ui64 DoAddSubkey(int subkeyType, TStringBuf subkey, bool isPrioritized) {
        return (IsPrioritizedNormalization(subkeyType) || isPrioritized)
               ? subkeyType : CombineHashes<ui64>(subkeyType, CityHash64(subkey));
    }

    static ui64 GenerateKey(const TSourceFactors& sf) {
        ui64 result = CombineHashes<ui64>(CityHash64(sf.GetSourceName()), sf.GetCommon());

        TTempArray<ui64> arr(sf.SourceSubkeysSize() + 1);
        *arr.Proceed(1) = DoAddSubkey(sf.GetSourceKeyType(), sf.GetSourceKey(), sf.GetSourceKeyTraits().GetIsPrioritized());
        for (const auto& sk : sf.GetSourceSubkeys()) {
            *arr.Proceed(1) = DoAddSubkey(sk.GetType(), sk.GetKey(), sk.GetTraits().GetIsPrioritized());
        }

        Sort(arr.Data(), arr.Current());
        for (size_t i = 0; i < arr.Filled(); ++i) {
            result = CombineHashes(result, *(arr.Data() + i));
        }

        return result;
    }

    using TMergeDict = THashMap<ui64, TSourceFactors*, THash<ui64>, TEqualTo<ui64>, TPoolAllocator>;

    void DoMoveSourceFactors(TSourceFactors& to, const TSourceFactors& from) {
        to.CopyFrom(from);
    }

    void DoMoveSourceFactors(TSourceFactors& to, TSourceFactors& from) {
        to.Swap(&from);
    }

    void MergeMethodSimple(TSourceFactors& to, const TSourceFactors& from) {
        if (GetPriority(from) > GetPriority(to)
                || (GetPriority(from) == GetPriority(to) && GetTimestamp(from) > GetTimestamp(to)))
        {
            to.CopyFrom(from);
        }
    }

    template <class T, typename TMergeFunc>
    void DoMergeQueryData(TDeque<TSourceFactors>& result, TMergeDict& dict, T& sf, TMergeFunc&& doMerge) {
        ui64 key = GenerateKey(sf);
        if (dict.contains(key)) {
            auto* rival = dict.at(key);
            doMerge(*rival, sf);
        } else {
            auto& winner = result.emplace_back();
            DoMoveSourceFactors(winner, sf);
            dict[key] = &winner;
        }
    }

    template <typename TMergeFunc>
    void DoMergeQueryData(TQueryData& result, TMergeDict& dict, const NQueryData::TSourceFactors& sf, TMergeFunc&& doMerge) {
        ui64 key = GenerateKey(sf);
        if (dict.contains(key)) {
            auto* rival = dict.at(key);
            doMerge(*rival, sf);
        } else {
            auto& winner = *result.AddSourceFactors();
            DoMoveSourceFactors(winner, sf);
            dict[key] = &winner;
        }
    }

    void MergeQueryDataSimple(TDeque<NQueryData::TSourceFactors>& result, TDeque<NQueryData::TSourceFactors>& from) {
        TMemoryPool pool{1024};
        TMergeDict dict{&pool};
        for (auto& sf : from) {
            DoMergeQueryData(result, dict, sf, MergeMethodSimple);
        }
    }

    template <class TCollection, typename TMergeFunc>
    void DoMergeQueryDataTempl(TQueryData& result, TCollection&& input, TMergeFunc&& doMerge) {
        TDeque<TSourceFactors> tmp;
        {
            TMemoryPool pool{1024};
            TMergeDict dict(&pool);
            for (auto& sf : *result.MutableSourceFactors()) {
                DoMergeQueryData(tmp, dict, sf, doMerge);
            }

            for (auto& qd : input) {
                for (auto& sf : *qd.MutableSourceFactors()) {
                    DoMergeQueryData(tmp, dict, sf, doMerge);
                }
            }
        }
        result.Clear();
        for (auto& sf : tmp) {
            result.AddSourceFactors()->Swap(&sf);
        }
    }

    void DoMergeQueryDataConstInput(TQueryData& result, const TVector<const TQueryData*>& input, const TMergeMethod& doMerge) {
        TQueryData tmp;
        {
            TMemoryPool pool{1024};
            TMergeDict dict(&pool);
            for (auto& sf : result.GetSourceFactors()) {
                DoMergeQueryData(tmp, dict, sf, doMerge);
            }

            for (auto qd : input) {
                for (auto& sf : qd->GetSourceFactors()) {
                    DoMergeQueryData(tmp, dict, sf, doMerge);
                }
            }
        }
        result.Swap(&tmp);
    }

    void MergeQueryDataSimple(TQueryData& res, const TVector<TQueryData>& qd) {
        DoMergeQueryDataTempl(res, TVector<TQueryData>(qd), MergeMethodSimple);
    }

    void MergeQueryDataSimple(TQueryData& result, TVector<TQueryData>&& input) {
        DoMergeQueryDataTempl(result, input, MergeMethodSimple);
    }

    void MergeQueryDataSimple(TQueryData& result, TDeque<TQueryData>&& input) {
        DoMergeQueryDataTempl(result, input, MergeMethodSimple);
    }

    void MergeQueryDataCustom(TQueryData& result, TDeque<TQueryData>&& input, std::function<void (TSourceFactors& , const TSourceFactors& )>&& doMerge) {
        DoMergeQueryDataTempl(result, input, doMerge);
    }

    void MergeQueryDataCustom(TQueryData& result, const TVector<const TQueryData*>& input, const TMergeMethod& doMerge) {
        DoMergeQueryDataConstInput(result, input, doMerge);
    }

    TQueryData MergeQueryDataSimple(const TVector<TQueryData>& qd) {
        TQueryData res;
        MergeQueryDataSimple(res, qd);
        return res;
    }
}
