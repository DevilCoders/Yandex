#pragma once

#include <util/generic/queue.h>
#include <util/generic/maybe.h>

template <class T, class TPriority, bool ReverseOrder>
struct TExtPriorityQueueContent {
    T Data;
    TPriority Pri;

    bool operator<(const TExtPriorityQueueContent& other) const {
        return ReverseOrder ? (Pri < other.Pri) : (other.Pri < Pri);
    }
};

/**
 *  Класс-wrapper для TPriorityQueue, позволяющий работать в ситуации, когда приоритет элемента
 *  не вычисляется по самому элементу, а просто указывается при добавлении оного в очередь
 */
template <class T, class TPriority = int, bool ReverseOrder = false>
class TExtPriorityQueue: public TPriorityQueue<TExtPriorityQueueContent<T, TPriority, ReverseOrder>> {
    using TContent = TExtPriorityQueueContent<T, TPriority, ReverseOrder>;
    using TBase = TPriorityQueue<TContent>;

public:
    void push(const T& data, const TPriority priority) {
        TBase::push({data, priority});
    }
    void push(T&& data, const TPriority priority) {
        TBase::push({std::move(data), priority});
    }
    const T& ytop() const {
        return TBase::top().Data;
    }
};

/**
*  Класс-wrapper для частого случая, когда нам нужно просто получить N самых приоритетных элементов
*  и N известно заранее
*/
template <class T, class TPriority = int>
class TPriorityTopN: private TExtPriorityQueue<T, TPriority> {
    size_t N = 0;
    using TBase = TExtPriorityQueue<T, TPriority>;
    TMaybe<TPriority> LastShifted;

public:
    TPriorityTopN(size_t n)
        : N(n)
    {
        TBase::Container().reserve(N + 1);
    }

    TPriorityTopN(TPriorityTopN&&) = default;
    TPriorityTopN& operator=(TPriorityTopN&& b) = default;

    TPriorityTopN(const TPriorityTopN& b) {
        *this = b;
    }

    static TPriorityTopN MakeNotReserved(size_t n) {
        TPriorityTopN res;
        res.N = n;
        return res;
    }

    TPriorityTopN& operator=(const TPriorityTopN& b) {
        N = b.N;
        LastShifted = b.LastShifted;
        TBase::Container().reserve(N + 1);
        TBase::Container().assign(b.Container().begin(), b.Container().end());
        return *this;
    }

    void push(T&& data, const TPriority priority) {
        if (N > 0) {
            if (TBase::size() < N || priority > TBase::top().Pri) {
                TBase::push(std::move(data), priority);
                if (TBase::size() > N) {
                    LastShifted = TBase::top().Pri;
                    TBase::pop();
                }
            } else if (TBase::size() == N && priority == TBase::top().Pri) {
                LastShifted = priority;
            }
        }
    }

    void push(const T& data, const TPriority priority) {
        if (N > 0) {
            if (TBase::size() < N || priority > TBase::top().Pri) {
                TBase::push(data, priority);
                if (TBase::size() > N) {
                    LastShifted = TBase::top().Pri;
                    TBase::pop();
                }
            } else if (TBase::size() == N && priority == TBase::top().Pri) {
                LastShifted = priority;
            }
        }
    }

    using TBase::Container;
    using TBase::empty;
    using TBase::pop;
    using TBase::size;
    using TBase::top;
    using TBase::ytop;
    using TBase::operator bool;

    TBase& AsExtPriorityQueue() {
        return *this;
    }

    size_t GetMaxSize() const {
        return N;
    }

    // reduce size to be sure that all values equal to top() are presented in queue
    // useful to prevent dependence on sort order of inputs
    // NOTE: will not work if pop performed before ShrinkToSharpBorder
    void ShrinkToSharpBorder() {
        if (!LastShifted.Defined()) {
            return;
        }

        while (!TBase::empty() && TBase::top().Pri == LastShifted.GetRef()) {
            TBase::pop();
        }
    }

    TVector<T> ToSortedVec() {
        TVector<T> res(TBase::size());
        size_t idx = res.size();
        while (!TBase::empty()) {
            Y_ASSERT(idx > 0);
            --idx;
            res[idx] = TBase::ytop();
            TBase::pop();
        }
        return res;
    }
};
