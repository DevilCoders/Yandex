#pragma once

#include <numeric>
#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

namespace NDoom {

template <class Key>
class TOffroadKeyBuffer {
    using TKey = Key;
    using TKeys = TDeque<TKey>;
    using TFrequences = TVector<std::pair<ui32, ui32>>;
public:
    TOffroadKeyBuffer() {}

    void Reset() {
        HitCount_ = 0;
        TermId_ = 0;
        Keys_.clear();
        FrequencesWithKeys_.clear();
        KeysMapping_.clear();
        SortedKeysMapping_.clear();
        NeedKeysSorting_ = false;
    }

    void AddKey(TKey&& key) {
        if (!Keys_.empty() && Keys_.back() > key) {
            NeedKeysSorting_ = true;
        }
        FrequencesWithKeys_.emplace_back(HitCount_, TermId_);
        Keys_.push_back(std::move(key));
        ++TermId_;
        HitCount_ = 0;
    }

    void AddHit() {
        ++HitCount_;
    }

    const TKeys& Keys() const {
        return Keys_;
    }

    const TVector<ui32>& KeysMapping() {
        return KeysMapping_;
    }

    const TVector<ui32>& SortedKeysMapping() {
        return SortedKeysMapping_;
    }

    void PrepareKeys(bool sortByFrequences) {
        KeysMapping_.resize(Keys_.size());
        if (NeedKeysSorting_) {
            TVector<ui32> keyIdxs(Keys_.size());
            std::iota(keyIdxs.begin(), keyIdxs.end(), 0);
            auto cmp = [&k = Keys_] (ui32 l, ui32 r) {
                return k[l] < k[r];
            };
            Sort(keyIdxs.begin(), keyIdxs.end(), cmp);
            for (ui32 i = 0; i < Keys_.size(); ++i) {
                KeysMapping_[keyIdxs[i]] = i;
            }
            for (ui32 i = 0; i < Keys_.size(); ++i) {
                if (keyIdxs[i] != Max<ui32>()) {
                    ui32 cur = keyIdxs[i];
                    ui32 last = i;
                    for (;;) {
                        std::swap(Keys_[last], Keys_[cur]);
                        ui32 next = keyIdxs[cur];
                        keyIdxs[cur] = Max<ui32>();
                        if (next == i) {
                            break;
                        }
                        last = cur;
                        cur = next;
                    }
                }
            }
        } else {
            std::iota(KeysMapping_.begin(), KeysMapping_.end(), 0);
        }

        SortedKeysMapping_.resize(Keys_.size());
        if (sortByFrequences) {
            Sort(FrequencesWithKeys_.begin(), FrequencesWithKeys_.end(), TGreater<>());
            for (ui32 i = 0; i < Keys_.size(); ++i) {
                SortedKeysMapping_[KeysMapping_[FrequencesWithKeys_[i].second]] = i;
            }
            for (ui32 i = 0; i < Keys_.size(); ++i) {
                KeysMapping_[FrequencesWithKeys_[i].second] = i;
            }
        } else {
            std::iota(SortedKeysMapping_.begin(), SortedKeysMapping_.end(), 0);
        }
    }

private:
    TFrequences FrequencesWithKeys_;
    TVector<ui32> KeysMapping_;
    TVector<ui32> SortedKeysMapping_;
    TKeys Keys_;
    bool NeedKeysSorting_ = false;
    ui32 HitCount_ = 0;
    ui32 TermId_ = 0;
};

} //namespace NDoom
