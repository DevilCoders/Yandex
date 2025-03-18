#pragma once

#include <util/system/compiler.h>

#include <library/cpp/offroad/offset/data_offset.h>
#include <library/cpp/offroad/flat/flat_searcher.h>

namespace NOffroad {
    template <class Data, class Vectorizer>
    class TSubSeeker {
        using TSearcher = TFlatSearcher<Data, ui64, Vectorizer, TUi64Vectorizer>;

    public:
        using TData = Data;

        TSubSeeker() {
        }

        TSubSeeker(const TArrayRef<const char>& source)
            : Searcher_(source)
        {
        }

        void Reset() {
            Searcher_.Reset();
        }

        void Reset(const TArrayRef<const char>& source) {
            Searcher_.Reset(source);
        }

        template <class Reader>
        bool LowerBound(const TData& prefix, TData* first, Reader* reader) const {
            if (reader->LowerBoundLocal(prefix, first))
                return true; /* In-block seek successful. */

            size_t index = Searcher_.LowerBound(prefix);

            TData data;
            ui64 offset;
            if (Y_UNLIKELY(index == 0)) {
                data = TData();
                offset = 0;
            } else {
                data = Searcher_.ReadKey(index - 1);
                offset = Searcher_.ReadData(index - 1);
            }

            if (!reader->Seek(TDataOffset(offset, 0), data))
                return false;

            return reader->LowerBoundLocal(prefix, first);
        }

    private:
        TSearcher Searcher_;
    };

}
