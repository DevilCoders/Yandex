#pragma once

#include "coded_blob.h"

#include <library/cpp/codecs/tls_cache.h>
#include <library/cpp/succinct_arrays/eliasfanomonotone.h>

namespace NCodedBlob {
    const ui64 CODED_BLOB_ARRAY_VERSION = 0;
    const char CODED_BLOB_ARRAY_MAGIC[] = "CODEDARR";

    using TOffsetsArray = NSuccinctArrays::TEliasFanoMonotoneArray<ui64>;

    class TCodedBlobArray {
    public:
        class TIndexIterator {
        public:
            TIndexIterator(const TCodedBlobArray* arr = nullptr)
                : Array(arr)
                , CurrentIndex(-1)
            {
            }

            TStringBuf GetCurrentValue() const {
                return ValueHelper.GetCurrent(this);
            }

            ui64 GetCurrentIndex() const {
                return CurrentIndex;
            }

            ui64 GetCurrentOffset() const {
                return Array->GetOffsetByIndex(CurrentIndex);
            }

            bool HasNext() const {
                return Array && CurrentIndex + 1 < Array->Count();
            }

            bool Next() {
                if (!HasNext())
                    return false;

                ValueHelper.Invalidate();
                ++CurrentIndex;
                return true;
            }

            TStringBuf FetchCurrent(NUtils::TValueTag, TBuffer& b) const {
                return Array->GetByIndex(CurrentIndex, b);
            }

        private:
            NUtils::TValueHelper ValueHelper;

            const TCodedBlobArray* Array;
            ui64 CurrentIndex;
        };

    public:
        using TOffsetIterator = TCodedBlob::TOffsetIterator;

        TOffsetIterator OffsetIterator() const {
            return TOffsetIterator(&Data);
        }

        TStringBuf GetByOffset(ui64 offset) const {
            return Data.GetByOffset(offset);
        }

        TStringBuf GetByOffset(ui64 offset, TBuffer& buffer) const {
            return Data.GetByOffset(offset, buffer);
        }

        ui64 Size() const {
            return TotalSize;
        }

        ui64 GetIndexSize() const {
            return IndexSize;
        }

        ui64 GetDataSize() const {
            return Data.Size();
        }

        ui64 Count() const {
            return Offsets.Size();
        }

        TString GetCodecName() const {
            return Data.GetCodecName();
        }

    public:
        TCodedBlobArray() {
        }

        explicit TCodedBlobArray(const TBlob& b) {
            Init(b);
        }

        void Init(TBlob b);

        ui64 GetOffsetByIndex(ui64 index) const {
            return Offsets[index];
        }

        TStringBuf GetByIndex(ui64 index) const {
            auto tmpBuffer = NCodecs::TBufferTlsCache::TlsInstance().Item();
            return GetByIndex(index, tmpBuffer.Get());
        }

        TStringBuf GetByIndex(ui64 index, TBuffer& buffer) const {
            return Data.GetByOffset(GetOffsetByIndex(index), buffer);
        }

        TIndexIterator IndexIterator() const {
            return TIndexIterator(this);
        }

    private:
        TCodedBlob Data;
        TOffsetsArray Offsets;
        ui64 TotalSize = 0;
        ui64 IndexSize = 0;
    };

}
