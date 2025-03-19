#pragma once

#include <util/generic/noncopyable.h>
#include <library/cpp/wordpos/wordpos.h>

#include "indexfile.h"

namespace NIndexerCore {

    //! universal implementation of hit writer supporting all formats of hits and compression
    //! @note represents generic interface that hides real implementation of hit writer
    //!       also it writes information about hits into key file
    template <typename THitCoder, typename TStream>
    class THitWriterImpl : private TNonCopyable {
        i64 Count; // optional for writers w/o subindex
        ui32 Length;
        THitCoder HitCoder;
        static const size_t MinBufSize = 8 + 8 * sizeof(SUPERLONG); // 8 bytes for DumpHeader() (actually it requires 3)
    public:
        explicit THitWriterImpl(EHitFormat hitFormat)
            : Count(0)
            , Length(0)
            , HitCoder(hitFormat)
        {
        }
        //! @return true - if subindex writer should add a new YxPerst()
        //! @note boolean return value looks bad - 'false' looks like hit was not written... try to rework...
        bool WriteHit(TOutputIndexFileImpl<TStream>& file, SUPERLONG hit, i64 subIndexStep) {
            if (HitCoder.GetCurrent() != START_POINT)
                Y_ENSURE(HitCoder.GetCurrent() <= hit, "Got hit less than current\n");
            Y_ENSURE(subIndexStep > 0, "subIndexStep should be positive\n");
            typename NIndexerDetail::TOutputIndexStream<TStream>::TDirectBuffer buf = file.GetDirectInvBuffer(MinBufSize);
            char* p = buf.GetPointer();
            ui32 len = 0;
            ++Count;
            const bool writeSubindex = ((Count % subIndexStep) == 0);
            const ui32 n = HitCoder.Output(hit, p);
            len += n;
            if (Y_UNLIKELY(writeSubindex))
                len += HitCoder.Flush(p + n);
            buf.ApplyChanges(len);
            Length += len;
            return writeSubindex;
        }
        //! @return true - if there were written hits (length > 0)
        //! @todo rename to FlushHits()
        bool WriteHits(TOutputIndexFileImpl<TStream>& file) {
            if (!Count)
                return false;
            typename NIndexerDetail::TOutputIndexStream<TStream>::TDirectBuffer buf = file.GetDirectInvBuffer(MinBufSize + 8); // + 8 bytes for DumpJunk() (actually it requires 7)
            const ui32 n = HitCoder.Finish(buf.GetPointer());
            buf.ApplyChanges(n);
            Length += n;
            return true;
        }
        void Reset() {
            Count = 0;
            Length = 0;
            HitCoder.Reset();
        }
        int GetKeyDataLength(TOutputIndexFileImpl<TStream>& file) const {
            int len = 0;
            if (file.GetFormat() == IYndexStorage::FINAL_FORMAT) {
                len += file.GetKeyDataLen(Count);
            }
            len += file.GetKeyDataLen(Length);
            return len;
        }
        void WriteKeyData(TOutputIndexFileImpl<TStream>& file) const {
            if (file.GetFormat() == IYndexStorage::FINAL_FORMAT) {
                file.WriteKeyData(Count);
            }
            file.WriteKeyData(Length);
        }
        i64 GetCount() const {
            return Count;
        }
        ui32 GetLength() const {
            return Length;
        }
        SUPERLONG GetLastWrittenHit() const {
            return HitCoder.GetCurrent();
        }
    };

}
