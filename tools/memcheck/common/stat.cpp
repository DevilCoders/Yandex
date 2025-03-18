#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <util/stream/output.h>
#include <util/generic/yexception.h>

#include "stat.h"
#include "statparser.h"

using NMemstatsParser::TBackTrace;
using NMemstatsParser::TItem;
using NMemstatsParser::ReadItem;

static TString PrintItem(const TItem& item) {
    TString ret = "[";

    ret += ::ToString(item.Len);
    ret += ", ";
    ret += ::ToString(item.Hndl);

    for (NMemstatsParser::TBackTrace::const_iterator it = item.Bt.begin(); it != item.Bt.end(); ++it) {
        ret += ", ";
        ret += ::ToString(*it);
    }

    ret += "]";

    return ret;
}

class TAllocatorStats::TImpl {
    typedef TVector<ui64> TBackTrace;

    struct TItems {
        inline TItems() :
            cnt(0) {
        }

        inline ui64 HeapSize() const noexcept {
            ui64 ret = 0;

            for (TVector<TItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
                ret += it->Len;
            }

            return ret;
        }

        inline void Fix() {
            if (cnt > 0) {
                Sort(items.begin(), items.end(), SortByHndl);
                items.resize(cnt);
            }
        }

        static inline bool SortByHndl(const TItem& l, const TItem& r) noexcept {
            return l.Hndl > r.Hndl;
        }

        int cnt;
        TVector<TItem> items;
    };

public:
    inline TImpl() :
        mNew(0), mDel(0) {
    }

    inline ~TImpl() {
    }

    inline bool Read(IInputStream* in) {
        TItem item;

        if(!ReadItem(item, *in))
            return false;

        if(item.Delete) {
            Add(item.Ptr, -1);
            ++mDel;
        } else {
            TItems* items = Add(item.Ptr, 1);

            if (items)
                items->items.push_back(item);

            ++mNew;
        }

        return true;
    }

    inline void Print() {
        Fix();

        i64 heap = 0;

        for (TAllocs::const_iterator it = mAllocs.begin(); it != mAllocs.end(); ++it) {
            const TItems& items = it->second;
            heap += items.HeapSize();

            Cerr << "--> " << items.cnt << " (" << items.HeapSize();

            for (TVector<TItem>::const_iterator it = items.items.begin(); it != items.items.end(); ++it) {
                Cerr << ", " << PrintItem(*it);
            }

            Cerr << ")" << Endl;
        }

        Cerr << "Heap: " << heap << Endl;
        Cerr << "New: " << mNew << Endl;
        Cerr << "Del: " << mDel << Endl;
        Cerr << "Used: " << mAllocs.size() << Endl;
        Cerr << "Free(0): " << mAllocs[0].cnt << Endl;
    }

private:
    inline TItems* Add(ui64 ptr, int inc) {
        TItems& items = mAllocs[ptr];

        items.cnt += inc;

        if (items.cnt == 0) {
            mAllocs.erase(ptr);

            return nullptr;
        }

        return &items;
    }

    inline void Fix() {
        for (TAllocs::iterator it = mAllocs.begin(); it != mAllocs.end(); ++it) {
            it->second.Fix();
        }
    }

private:
    typedef THashMap<ui64, TItems> TAllocs;
    TAllocs mAllocs;
    ui64 mNew;
    ui64 mDel;
};

TAllocatorStats::TAllocatorStats() :
    mImpl(new TImpl()) {
}

TAllocatorStats::~TAllocatorStats() {
}

bool TAllocatorStats::Read(IInputStream* s) {
    return mImpl->Read(s);
}

void TAllocatorStats::Print() const {
    mImpl->Print();
}
