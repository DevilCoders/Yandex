#pragma once

#include "seek_mode.h"

#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/offroad/offset/tuple_sub_offset.h>

namespace NOffroad {
    template <class Data, class Vectorizer>
    class TLimitedTupleSubSeeker {
        using TSearcher = TFlatSearcher<Data, ui64, Vectorizer, TUi64Vectorizer>;

    public:
        using THit = Data;

        TLimitedTupleSubSeeker() = default;

        TLimitedTupleSubSeeker(const TArrayRef<const char>& source)
            : Searcher_(source)
        {
        }

        void Reset() {
            Searcher_.Reset();
        }

        void Reset(const TArrayRef<const char>& source) {
            Searcher_.Reset(source);
        }

        void Reset(const TBlob& blob) {
            Searcher_.Reset(blob);
        }

        template <class Reader>
        bool LowerBound(
            const THit& prefix,
            const TTupleSubOffset& start,
            const TTupleSubOffset& end,
            THit* first,
            Reader* reader) const {
            ui32 startSub = start.SubIndex();
            ui32 endSub = end.SubIndex();
            Y_ASSERT(startSub <= endSub);

            const size_t index = Searcher_.LowerBound(prefix, startSub, endSub);
            if (index == startSub) {
                if (!reader->Seek(start, THit(), TSeekPointSeek())) {
                    return false;
                }
            } else {
                THit data = Searcher_.ReadKey(index - 1);
                ui64 offset = Searcher_.ReadData(index - 1);

                if (!reader->Seek(TTupleSubOffset(TDataOffset(offset, 0), index), data)) {
                    return false;
                }
            }
            return reader->LowerBoundLocal(prefix, first);
        }

    private:
        TSearcher Searcher_;
    };

}
