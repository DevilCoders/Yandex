#pragma once

#include <library/cpp/offroad/fat/fat_searcher.h>
#include <library/cpp/offroad/offset/offset_data.h>
#include <library/cpp/offroad/offset/data_offset.h>

namespace NOffroad {
    template <class Data, class Serializer>
    class TFatKeySeeker {
        using TSearcher = TFatSearcher<TOffsetData<Data>, TOffsetDataSerializer<Serializer>>;

    public:
        using TKey = typename TSearcher::TKey;
        using TKeyRef = typename TSearcher::TKeyRef;
        using TData = Data;

        TFatKeySeeker() {
        }

        TFatKeySeeker(const TArrayRef<const char>& fat, const TArrayRef<const char>& fatsub)
            : Searcher_(fat, fatsub)
        {
        }

        TFatKeySeeker(const TBlob& fat, const TBlob& fatsub)
            : Searcher_(fat, fatsub)
        {
        }

        void Reset(const TArrayRef<const char>& fat, const TArrayRef<const char>& fatsub) {
            Searcher_.Reset(fat, fatsub);
        }

        void Reset(const TBlob& fat, const TBlob& fatsub) {
            Searcher_.Reset(fat, fatsub);
        }

        template <class Reader>
        bool Seek(size_t keyIndex, Reader* reader) const {
            const size_t index = keyIndex / Reader::BlockSize;
            if (index + 1 >= Searcher_.Size()) {
                return false;
            }
            TKeyRef fatKey = Searcher_.ReadKey(index);
            TOffsetData<TData> fatData = Searcher_.ReadData(index);
            if (!reader->Seek(TDataOffset(fatData.Offset, 0), fatKey, fatData.Base)) {
                return false;
            }

            TKeyRef key;
            TData data;
            for (size_t position = keyIndex % Reader::BlockSize; position > 0; --position) {
                if (!reader->ReadKey(&key, &data)) {
                    return false;
                }
            }

            return true;
        }

        template <class Reader>
        bool LowerBound(const TKeyRef& prefix, TKeyRef* firstKey, TData* firstData, Reader* reader, size_t* keyIndex = nullptr) const {
            size_t index = Searcher_.LowerBound(prefix);
            if (index >= Searcher_.Size())
                return false; /* Assume there is a terminator record. */

            Y_ASSERT(index > 0); /* And a starting record too! */

            TKeyRef fatKey = Searcher_.ReadKey(index - 1);
            TOffsetData<TData> fatData = Searcher_.ReadData(index - 1);
            if (!reader->Seek(TDataOffset(fatData.Offset, 0), fatKey, fatData.Base))
                return false;

            if (!reader->LowerBoundLocal(prefix, firstKey, firstData, fatKey, fatData.Base))
                return false;

            if (keyIndex) {
                *keyIndex = (index - 1) * Reader::BlockSize + reader->Position().Index();
            }

            return true;
        }

    private:
        TSearcher Searcher_;
    };

}
