#pragma once

#include <kernel/doom/key/decoded_key.h>

#include <util/digest/multi.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/yexception.h>

namespace NDoom {

namespace NPrivate {
    struct TKeyFormIndexesEqual {
        TKeyFormIndexesEqual(const TDecodedKey& key)
            : Key(key)
        {
        }

        inline bool operator()(size_t l, size_t r) const {
            return Key.Form(l) == Key.Form(r);
        }

        const TDecodedKey& Key;
    };

    struct TKeyFormIndexHasher {
        TKeyFormIndexHasher(const TDecodedKey& key)
            : Key(key)
        {
        }

        inline size_t operator()(size_t ind) const {
            TDecodedFormRef form = Key.Form(ind);
            return MultiHash(ComputeHash(form.Text()), (static_cast<size_t>(form.Language()) + 1) * 42424243, (static_cast<size_t>(form.Flags()) + 1) * 4243);
        }

        const TDecodedKey& Key;
    };
}

template <class Base>
class TMultiKeyIndexWriter: public Base {
    static_assert(std::is_same<typename Base::TKeyRef, TDecodedKey>::value, "Base class is expected to use TDecodedKey as key.");

    using TFinishReturnType = decltype(std::declval<Base>().Finish());

public:
    using THit = typename Base::THit;
    using TKey = TDecodedKey;
    using TKeyRef = TDecodedKey;

    template <class... Args>
    TMultiKeyIndexWriter(Args&&... args)
        : Base(std::forward<Args>(args)...)
        , LastKeyFormsSet_(16, NPrivate::TKeyFormIndexHasher(LastKey_), NPrivate::TKeyFormIndexesEqual(LastKey_))
    {
    }

    void WriteHit(const THit& hit) {
        CurrentKeyHitsBuffer_.push_back(hit);
    }

    void WriteKey(const TKeyRef& key) {
        Y_ASSERT(!key.Lemma().empty());
        if (LastKey_.Lemma() != key.Lemma()) {
            FlushKeyWithHits();
            LastKey_.SetLemma(key.Lemma());
        }
        AddCurrentKeyForms(key);
        AddCurrentKeyHits();
    }

    template <class... Args>
    void Reset(Args&&... args) {
        CurrentKeyHitsBuffer_.clear();

        LastKey_.Clear();
        LastKeyHitsBuffer_.clear();
        LastKeyFormsSet_.clear();

        Base::Reset(std::forward<Args>(args)...);
    }

    TFinishReturnType Finish() {
        FlushKeyWithHits();
        return Base::Finish();
    }

private:
    void AddCurrentKeyForms(const TKeyRef& key) {
        CurrentKeyFormsMapping_.resize(key.FormCount());
        for (size_t i = 0; i < key.FormCount(); ++i) {
            LastKey_.AddForm(key.Form(i));
            size_t formIndex = LastKey_.FormCount() - 1;
            auto it = LastKeyFormsSet_.find(formIndex);
            if (it != LastKeyFormsSet_.end()) {
                CurrentKeyFormsMapping_[i] = *it;
                LastKey_.PopBackForm();
            } else {
                CurrentKeyFormsMapping_[i] = formIndex;
                LastKeyFormsSet_.insert(formIndex);
            }
        }
    }

    void AddCurrentKeyHits() {
        for (const THit& hit : CurrentKeyHitsBuffer_) {
            ui32 form = hit.Form();
            if (CurrentKeyFormsMapping_.empty()) {
                Y_ENSURE(form == 0);
            } else {
                Y_ENSURE(form < CurrentKeyFormsMapping_.size());
            }
            LastKeyHitsBuffer_.push_back(hit);
            LastKeyHitsBuffer_.back().SetForm(CurrentKeyFormsMapping_.empty() ? 0 : CurrentKeyFormsMapping_[form]);
        }
        CurrentKeyHitsBuffer_.clear();
    }

    void FlushKeyWithHits() {
        if (LastKeyHitsBuffer_.empty()) {
            LastKey_.Clear();
            LastKeyFormsSet_.clear();
            return;
        }
        // renumerate forms
        if (LastKey_.FormCount() > 0) {
            FormIndexes_.resize(LastKey_.FormCount());
            std::iota(FormIndexes_.begin(), FormIndexes_.end(), 0);
            FormHitsCount_.assign(LastKey_.FormCount(), 0);
            for (const THit& hit : LastKeyHitsBuffer_) {
                Y_VERIFY(hit.Form() < FormHitsCount_.size());
                ++FormHitsCount_[hit.Form()];
            }
            auto cmp = [&] (size_t l, size_t r) {
                return FormHitsCount_[l] != FormHitsCount_[r] ? FormHitsCount_[l] > FormHitsCount_[r] : LastKey_.Form(l) < LastKey_.Form(r);
            };
            Sort(FormIndexes_.begin(), FormIndexes_.end(), cmp);
            InvertedFormIndexes_.resize(FormIndexes_.size());
            for (size_t i = 0; i < FormIndexes_.size(); ++i) {
                InvertedFormIndexes_[FormIndexes_[i]] = i;
            }
            for (THit& hit : LastKeyHitsBuffer_) {
                Y_VERIFY(hit.Form() < InvertedFormIndexes_.size());
                hit.SetForm(InvertedFormIndexes_[hit.Form()]);
            }
        } else {
            FormIndexes_.clear();
        }

        // write sorted unique hits
        Sort(LastKeyHitsBuffer_.begin(), LastKeyHitsBuffer_.end());
        size_t uniqueHitsCount = Unique(LastKeyHitsBuffer_.begin(), LastKeyHitsBuffer_.end()) - LastKeyHitsBuffer_.begin();
        for (size_t i = 0; i < uniqueHitsCount; ++i) {
            Base::WriteHit(LastKeyHitsBuffer_[i]);
        }
        LastKeyHitsBuffer_.clear();

        // write current key with sorted forms
        KeyToWrite_.Clear();
        KeyToWrite_.SetLemma(LastKey_.Lemma());
        for (size_t i = 0; i < FormIndexes_.size(); ++i) {
            KeyToWrite_.AddForm(LastKey_.Form(FormIndexes_[i]));
        }
        Base::WriteKey(KeyToWrite_);
        LastKey_.Clear();
        LastKeyFormsSet_.clear();
    }

    TVector<THit> CurrentKeyHitsBuffer_;
    TVector<size_t> CurrentKeyFormsMapping_;

    TDecodedKey LastKey_;
    TVector<THit> LastKeyHitsBuffer_;
    THashSet<size_t, NPrivate::TKeyFormIndexHasher, NPrivate::TKeyFormIndexesEqual> LastKeyFormsSet_;

    // write stuff
    TVector<size_t> FormHitsCount_;
    TVector<size_t> FormIndexes_;
    TVector<size_t> InvertedFormIndexes_;
    TDecodedKey KeyToWrite_;
};

} // namespace NDoom
