#pragma once

#include <queue>
#include <utility>

#include <util/generic/vector.h>
#include <util/system/yassert.h>
#include <util/str_stl.h>

#include <kernel/doom/key/decoded_key.h>
#include <kernel/doom/hits/long_form_hit.h>


namespace NDoom {

namespace NPrivate {

template<class THit, size_t bits = THit::FormBits>
struct THitFormGetter {
    ui32 operator()(const THit& hit) const {
        return hit.Form();
    }
};

template<class THit>
struct THitFormGetter<THit, 0> {
    ui32 operator()(const THit&) const {
        return 0;
    }
};

template<class THit>
ui32 HitForm(const THit& hit) {
    return THitFormGetter<THit>()(hit);
}

} // namespace NPrivate


template<class Base, class HitCompare = TLess<typename Base::THit>>
class TMultiKeyIndexReader: public Base {
    using TBaseHit = typename Base::THit;
    static_assert(std::is_same<typename Base::TKeyRef, TDecodedKey>::value, "Base class is expected to use TDecodedKey as key.");
    static_assert(TBaseHit::FormBits <= 4, "Base form bitness is expected to be reasonably small.");
public:
    using TKey = TDecodedKey;
    using TKeyRef = TDecodedKey;
    using TKeyData = typename Base::TKeyData;
    using THit = TLongFormHit<TBaseHit>;
    using THitCompare = HitCompare;

    template<class... Args>
    TMultiKeyIndexReader(Args&&... args): Base(std::forward<Args>(args)...) {}

    ~TMultiKeyIndexReader() {
        ClearHeap();
        for (THeapItem* item : Pool_)
            delete item;
    }

    bool ReadKey(TKeyRef* key, TKeyData* = NULL) {
        if (NextKey_.Lemma().empty()) {
            if (Initialized_)
                return false;
            Initialized_ = true;

            if (!Base::ReadKey(&NextKey_))
                return false;
        }

        key->Clear();
        key->SetLemma(NextKey_.Lemma());

        ClearHeap();
        for (ui32 kishka = 0;; kishka++) {
            ui32 id = kishka << TBaseHit::FormBits;

            if(NextKey_.FormCount() > 0) {
                for (ui32 i = 0; i < NextKey_.FormCount(); i++) {
                    i32 srcIndex = id | i;
                    i32 dstIndex = key->FormIndex(NextKey_.Form(i));

                    if (dstIndex == -1) {
                        AddFormMapping(srcIndex, key->FormCount());
                        key->AddForm(NextKey_.Form(i));
                    } else {
                        AddFormMapping(srcIndex, dstIndex);
                    }
                }
            } else {
                Y_VERIFY(key->FormCount() == 0);

                if (TBaseHit::FormBits != 0) {
                    /* Special key w/o forms actually might have hits with forms, doh!
                     * We zero those out. */
                    size_t maxForm = (1 << TBaseHit::FormBits) - 1;
                    for (size_t i = 0; i <= maxForm; i++)
                        AddFormMapping(id | i, 0);
                } else {
                    AddFormMapping(id, 0);
                }
            }

            THeapItem* item = NewItem(id);
            if (item->Fill(this)) {
                Heap_.push(item);
            } else {
                ReleaseItem(item);
            }

            if (!Base::ReadKey(&NextKey_))
                NextKey_.Clear();

            if (NextKey_.Lemma() != key->Lemma())
                return true;
        }
    }

    bool ReadHit(THit* hit) {
        Y_VERIFY(!FormMapping_.empty());

        if (Heap_.empty())
            return false;

        THeapItem* item = Heap_.top();
        Heap_.pop();

        *hit = THit(FormMapping_[item->Id() | NPrivate::HitForm(item->Hit())], item->Hit());

        if (item->ReadHit()) {
            Heap_.push(item);
        } else {
            ReleaseItem(item);
        }

        return true;
    }

private:
    class THeapItem {
    public:
        THeapItem() {}
        THeapItem(ui32 id) : Id_(id) {}

        ui32 Id() const {
            return Id_;
        }

        void SetId(ui32 id) {
            Id_ = id;
        }

        TBaseHit Hit() const {
            Y_ASSERT(Pos_ >= 0);

            return Hits_[Pos_];
        }

        bool ReadHit() {
            return ++Pos_ < Hits_.size();
        }

        bool Fill(Base* base) {
            Hits_.clear();
            Pos_ = 0;

            TBaseHit hit;
            while (base->ReadHit(&hit))
                Hits_.push_back(hit);

            return !Hits_.empty();
        }

    private:
        ui32 Id_ = 0;
        size_t Pos_ = 0;
        TVector<TBaseHit> Hits_;
    };

    struct THeapItemGreater : THitCompare {
        bool operator()(const THeapItem* l, const THeapItem* r) const {
            return THitCompare::operator()(r->Hit(), l->Hit());
        }
    };

private:
    void AddFormMapping(ui32 src, ui32 dst) {
        if (FormMapping_.size() <= src)
            FormMapping_.resize(src + 1, 0);

        FormMapping_[src] = dst;
    }

    THeapItem* NewItem(ui32 id) {
        if (Pool_.empty()) {
            return new THeapItem(id);
        } else {
            THeapItem* result = Pool_.back();
            Pool_.pop_back();
            result->SetId(id);
            return result;
        }
    }

    void ReleaseItem(THeapItem* item) {
        Pool_.push_back(item);
    }

    void ClearHeap() {
        while (!Heap_.empty()) {
            ReleaseItem(Heap_.top());
            Heap_.pop();
        }
    }

private:
    /** Whether at least one key was read from the underlying reader (and thus
     * next key below is valid). */
    bool Initialized_ = false;

    /** Next key, waiting to be returned to the user. We have to store it because
     * we merge keys with the same prefix+lemma. */
    TKey NextKey_;

    /** Pool of heap items, so that we don't allocate them all the time. */
    TVector<THeapItem*> Pool_;

    /** Mapping from kishka id + form to actual form id that is to be returned. */
    TVector<ui32> FormMapping_;

    /** Heap that is used for merging kishkas. */
    std::priority_queue<THeapItem*, TVector<THeapItem*>, THeapItemGreater> Heap_;
};


} // namespace NDoom
