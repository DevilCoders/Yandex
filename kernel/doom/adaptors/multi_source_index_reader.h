#pragma once

#include <queue>
#include <utility>

#include <util/generic/fwd.h>
#include <util/generic/noncopyable.h>
#include <util/generic/yexception.h>
#include <util/system/yassert.h>
#include <util/str_stl.h>

#include <kernel/doom/progress/progress.h>

#include <library/cpp/containers/mh_heap/mh_heap.h>

#include "key_data_accumulator.h"

namespace NDoom {
    template <class Reader,
              class KeyDataAccumulator = TKeyDataAccumulator<typename Reader::TKeyData>,
              class KeyComparer = TLess<typename Reader::TKey>,
              class HitComparer = TLess<typename Reader::THit>>
    class TMultiSourceIndexReader: private TNonCopyable {
    public:
        using TKeyDataAccumulator = KeyDataAccumulator;
        using TKeyComparer = KeyComparer;
        using THitComparer = HitComparer;

        using TReader = Reader;
        using TKey = typename TReader::TKey;
        using TKeyRef = typename TReader::TKeyRef;
        using TKeyData = typename TKeyDataAccumulator::TAccumulatedData;
        using THit = typename TReader::THit;

        template <class SourceContainer, class... Args>
        TMultiSourceIndexReader(const SourceContainer& sources, Args&&... args) {
            TKeyHeapItemPtr keyItem;
            for (const auto& source : sources) {
                keyItem = MakeHolder<TKeyHeapItem>(std::forward<Args>(args)..., source);
                if (keyItem->ReadKey()) {
                    KeyHeap_.push(std::move(keyItem));
                }
            }
        }

        void Restart() {
            for (auto& keyItem: RemovedKeyItems_) {
                keyItem->Restart();
                if (keyItem->ReadKey()) {
                    KeyHeap_.push(std::move(keyItem));
                }
            }
            RemovedKeyItems_.clear();
        }

        bool ReadKey(TKeyRef* key) {
            NewKey = true;
            for (auto& keyItem : CurrentKeyItems_) {
                if (keyItem->ReadKey()) {
                    KeyHeap_.push(std::move(keyItem));
                } else {
                    RemovedKeyItems_.push_back(std::move(keyItem));
                }
            }
            CurrentKeyItems_.clear();

            if (KeyHeap_.empty()) {
                return false;
            }

            *key = Key_ = KeyHeap_.top()->Key();

            do {
                TKeyHeapItemPtr keyItem = KeyHeap_.Pop();
                CurrentKeyItems_.push_back(std::move(keyItem));
            } while (!KeyHeap_.empty() && KeyHeap_.top()->Key() == Key_);

            return true;
        }

        void ReadHits() {
            HitItems.clear();
            HitItemHolders.clear();
            for (auto& keyItem: CurrentKeyItems_) {
                THitHeapItemPtr hitItem = MakeHolder<THitHeapItem>(keyItem);
                HitItemHolders.push_back(std::move(hitItem));
                HitItems.push_back(HitItemHolders.back().Get());
            }
            HitHeap_.Restart(HitItems.begin(), HitItems.size());
        }

        bool ReadKey(TKeyRef* key, TKeyData* data) {
            if (ReadKey(key)) {
                TKeyData accumulated = TKeyDataAccumulator::Zero();
                for (const auto& keyItem : CurrentKeyItems_) {
                    TKeyDataAccumulator::Accumulate(&accumulated, keyItem->Data());
                }
                *data = accumulated;
                return true;
            }
            return false;
        }

        bool ReadHit(THit* hit) {
            if (NewKey) {
                ReadHits();
                NewKey = false;
            }

            if (!HitHeap_.Valid()) {
                return false;
            }

            *hit = HitHeap_.Current();
            ++HitHeap_;
            return true;
        }

        TProgress Progress() {
            ui64 current = 0;
            ui64 total = 0;
            AddProgress(CurrentKeyItems_, current, total);
            AddProgress(RemovedKeyItems_, current, total);
            KeyHeap_.AddProgress(current, total);
            return TProgress(current, total);
        }

    private:
        template <class TContainer>
        static inline void AddProgress(const TContainer& container, ui64& current, ui64& total) {
            for (const auto& item : container) {
                TProgress progress = item->Progress();
                current += progress.Current();
                total += progress.Total();
            }
        }

        class THitHeapItem;

        class TKeyHeapItem {
        public:
            template <class... Args>
            TKeyHeapItem(Args&&... args)
                : Reader_(std::forward<Args>(args)...)
            {
            }

            bool ReadKey() {
                TKeyRef key;
                if (Reader_.ReadKey(&key, &Data_)) {
                    Y_ENSURE(Key_ != key);
                    Key_ = key;
                    return true;
                }
                return false;
            }

            void Restart() {
                Reader_.Restart();
            }

            const TKey& Key() const {
                return Key_;
            }

            const TKeyData& Data() const {
                return Data_;
            }

            TProgress Progress() const {
                return Reader_.Progress();
            }

        private:
            friend class THitHeapItem;

            TReader Reader_;
            TKey Key_;
            TKeyData Data_;
        };

        using TKeyHeapItemPtr = THolder<TKeyHeapItem>;
        class TKeyHeapItemComparer: private TKeyComparer {
        public:
            bool operator()(const TKeyHeapItemPtr& lhs, const TKeyHeapItemPtr& rhs) const {
                return TKeyComparer::operator()(rhs->Key(), lhs->Key());
            }
        };

        template <class Base>
        class THeap: public Base {
        public:
            using TBase = Base;
            using TValue = typename TBase::value_type;

            TValue Pop() {
                TValue value = std::move(const_cast<TValue&>(TBase::top()));
                TBase::pop();
                return value;
            }

            void Clear() {
                *this = THeap<Base>();
            }
        };

        using TKeyHeapBase =
            THeap<std::priority_queue<TKeyHeapItemPtr, TVector<TKeyHeapItemPtr>, TKeyHeapItemComparer>>;
        class TKeyHeap: public TKeyHeapBase {
        public:
            void AddProgress(ui64& current, ui64& total) const {
                TMultiSourceIndexReader::AddProgress(TKeyHeapBase::c, current, total);
            }
        };

        class THitHeapItem {
        public:
            typedef THit value_type;

            THitHeapItem(const TKeyHeapItemPtr& heapItem)
                : Reader_(heapItem->Reader_)
            {
            }

            bool ReadHit() {
                return Reader_.ReadHit(&Hit_);
            }

            const THit& Hit() {
                return Hit_;
            }

            void Restart() {
                Valid_ = ReadHit();
            }

            void Next() {
                Valid_ = ReadHit();
            }

            void operator++() {
                Valid_ = ReadHit();
            }

            THit Current() const {
                return Hit_;
            }

            THit operator*() {
                return Hit_;
            }

            bool Valid() const {
                return Valid_;
            }

        private:
            TReader& Reader_;
            THit Hit_;
            bool Valid_ = true;
        };

        using THitHeapItemPtr = THolder<THitHeapItem>;
        class THitHeapItemComparer: private THitComparer {
        public:
            bool operator()(const THitHeapItemPtr& lhs, const THitHeapItemPtr& rhs) {
                return THitComparer::operator()(rhs->Hit(), lhs->Hit());
            }
        };

        using THitHeap =
            MultHitHeap<THitHeapItem>;

        TKeyHeap KeyHeap_;
        TKey Key_;
        TVector<TKeyHeapItemPtr> CurrentKeyItems_;
        TVector<TKeyHeapItemPtr> RemovedKeyItems_;
        TVector<THitHeapItemPtr> HitItemHolders;
        TVector<THitHeapItem*> HitItems;

        THitHeap HitHeap_;

        ui64 ProgressCurrent_ = 0;
        ui64 ProgressTotal_ = 0;

        bool NewKey = true;
    };

} // namespace NDoom
