#pragma once

#include <library/cpp/html/face/event.h>

#include <util/generic/intrlist.h>
#include <util/memory/pool.h>

namespace NHtml {
    class TIntrusiveChunkQueue: public TNonCopyable {
    public:
        struct TChunk
           : public TIntrusiveSListItem<TChunk>,
              public TPoolable,
              public THtmlChunk {
        public:
            inline TChunk(const THtmlChunk& chunk, size_t index)
                : THtmlChunk(chunk)
                , Index_(index)
            {
            }

            inline size_t Index() const {
                return Index_;
            }

        private:
            const size_t Index_;
        };

        template <class TListItem, class TNode>
        class TIteratorBase {
        public:
            inline TIteratorBase(TListItem* item) noexcept
                : Item_(item)
            {
            }

            inline void Next() noexcept {
                Item_ = Item_->Next();
            }

            inline bool operator==(const TIteratorBase& right) const noexcept {
                return Item_ == right.Item_;
            }

            inline bool operator!=(const TIteratorBase& right) const noexcept {
                return Item_ != right.Item_;
            }

            inline TIteratorBase& operator++() noexcept {
                Next();

                return *this;
            }

            inline TNode& operator*() noexcept {
                return *Item_->Node();
            }

            inline const TNode& operator*() const noexcept {
                return *Item_->Node();
            }

            inline TNode* operator->() noexcept {
                return Item_->Node();
            }

        private:
            TListItem* Item_;
        };

        using TConstIterator = TIteratorBase<const TIntrusiveSListItem<TChunk>, const TChunk>;

    public:
        inline TIntrusiveChunkQueue() noexcept
            : Head_(nullptr)
            , Tail_(nullptr)
        {
        }

        inline TConstIterator Begin() const noexcept {
            return TConstIterator(Head_);
        }

        inline TConstIterator End() const noexcept {
            return TConstIterator(nullptr);
        }

        inline void Clear() noexcept {
            Head_ = Tail_ = nullptr;
        }

        inline bool Empty() const noexcept {
            return Head_ == nullptr;
        }

        inline void PushBack(TChunk* chunk) {
            if (Empty()) {
                Head_ = Tail_ = chunk;
            } else {
                Tail_->SetNext(chunk);
                Tail_ = chunk;
            }
        }

    private:
        TChunk* Head_;
        TChunk* Tail_;
    };

    using TSegmentedQueueIterator = TIntrusiveChunkQueue::TConstIterator;

    inline const THtmlChunk* GetHtmlChunk(const TSegmentedQueueIterator& it) {
        return static_cast<const THtmlChunk*>(&(*it));
    }

    inline ui32 GetHtmlChunkIndex(const THtmlChunk* chunk) {
        return static_cast<const TIntrusiveChunkQueue::TChunk*>(chunk)->Index();
    }

    inline const THtmlChunk* GetNextChunk(const THtmlChunk* chunk) {
        const auto tmp = static_cast<const TIntrusiveChunkQueue::TChunk*>(chunk);

        if (tmp->IsEnd()) {
            return nullptr;
        } else {
            return static_cast<const THtmlChunk*>(tmp->Next()->Node());
        }
    }

}
