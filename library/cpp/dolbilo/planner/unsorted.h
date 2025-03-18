#pragma once

#include "loader.h"

#include <util/stream/output.h>
#include <util/generic/set.h>
#include <util/datetime/base.h>

class TMonotonicAdapter {
        struct TRec {
            TInstant Start;
            ui64 Id;
            TDevastateItem Item;
        };

        struct TSorter {
            inline bool operator() (const TRec& l, const TRec& r) const noexcept {
                return l.Start < r.Start || (l.Start == r.Start && l.Id < r.Id);
            }
        };

        typedef TSet<TRec, TSorter> TSortedItems;
    public:
        typedef IReqsLoader::TParams TParams;

        inline TMonotonicAdapter(TParams* params)
            : Params_(params)
            , Id_(0)
            , Last_(TInstant::Max())
            , Errors_(0)
            , Threshold_(5000)
        {
        }

        inline void Add(const TInstant& start, const TDevastateItem& item) {
            TRec rec = {
                  start
                , ++Id_
                , item
            };

            Items_.insert(rec);

            if (Items_.size() > Threshold_ * (1 + Errors_)) {
                ProcessOne();
            }
        }

        inline void Flush() {
            while (Items_.size()) {
                ProcessOne();
            }

            if (Errors_) {
                --Errors_;
            }

            if (Errors_) {
                Cerr << "non-monotonic elements: " << Errors_ << Endl;
            }
        }

    private:
        inline void ProcessOne() {
            TRec cur = *Items_.begin();
            Items_.erase(Items_.begin());

            if (Last_ > cur.Start) {
                Last_ = cur.Start;
                ++Errors_;
            }

            const TDuration delta = cur.Start - Last_;

            cur.Item.ToWait(delta);

            Params_->Add(cur.Item);
            Last_ = cur.Start;
        }

    private:
        TParams* Params_;
        ui64 Id_;
        TSortedItems Items_;
        TInstant Last_;
        size_t Errors_;
        size_t Threshold_;
};

template <class T>
inline void SimpleCallOnce(bool& isCalled, T callable) { // as Simple as TSimpleCounter :)
    const bool old = isCalled;
    isCalled = true;
    if (!old) {
        callable();
    }
}
