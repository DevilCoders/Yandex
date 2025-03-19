#pragma once

#include <kernel/search_types/search_types.h>
#include <util/memory/blob.h>
#include <util/system/file.h>

#include <kernel/keyinv/invkeypos/keycode.h>

#include <kernel/keyinv/hitlist/hitformat.h>
#include <kernel/keyinv/hitlist/subindex.h>
#include <kernel/keyinv/hitlist/invsearch.h>
#include <kernel/keyinv/hitlist/positerator.h>
#include "indexstorageface.h"
#include "indexutil.h"
#include "indexfile.h"

namespace NIndexerCore {

    //! reads key text and data from key-file
    //! @note it does not keep any previous data, it keeps data of the current key only;
    //!       actually it requires open key-file only, it has no calls to inv-file
    template <typename TInputIndex>
    class TInvKeyReaderImpl : private TNonCopyable {
    private:
        char Text[MAXKEY_BUF];
        i64 Count;
        ui32 Length;
        i64 Offset;
        const EHitFormat HitFormat;
        bool RawKeys;

        TInputIndex& File;

        //const TIndexInfo IndexInfo;
        const TSubIndexInfo SubIndexInfo;

    private:
        bool ReadKeyData() {
            Offset += hitsSize(Offset, Length, Count, SubIndexInfo);
            i64 val;
            if (File.GetFormat() == IYndexStorage::FINAL_FORMAT) {
                if (!File.ReadKeyData(val))
                    return false;
                Count = val;
                Y_ASSERT(Count >= 0);
            }
            if (!File.ReadKeyData(val))
                return false;
            Length = (ui32)val;
            Y_ASSERT(Length == val);
            return true;
        }
        static TBlob CreateBlob(const TMemoryMap& mapping, ui64 offset, size_t length) {
            return TBlob::FromMemoryMap(mapping, offset, length);
        }
        static TBlob CreateBlob(const TFile& file, ui64 offset, size_t length) {
            return TBlob::FromFileContent(file, offset, length);
        }
        static TBlob CreateBlob(const TBlob& blob, ui64 offset, size_t length) {
            Y_ASSERT(offset + length <= blob.Size());
            return TBlob::NoCopy(reinterpret_cast<const char*>(blob.Data()) + offset, length);
        }
    public:
        //! @note keyStream must be already open
        //! @param keyStream
        //! @param format       supposed that we always know what format we read
        explicit TInvKeyReaderImpl(TInputIndex& file)
            : Count(0)
            , Length(0)
            , Offset(0LL)
            , HitFormat(DetectHitFormat(file.GetVersion()))
            , RawKeys((file.GetVersion() & YNDEX_VERSION_MASK) == YNDEX_VERSION_RAW64_HITS)
            , File(file)
            , SubIndexInfo(InitSubIndexInfo(file.GetFormat(), file.GetVersion(), file.GetNumberOfBlocks() < 0))
        {
            *Text = 0;
        }
        TInvKeyReaderImpl(TInputIndex& file, bool hasSubIndex)
            : Count(0)
            , Length(0)
            , Offset(0LL)
            , HitFormat(DetectHitFormat(file.GetVersion()))
            , RawKeys((file.GetVersion() & YNDEX_VERSION_MASK) == YNDEX_VERSION_RAW64_HITS)
            , File(file)
            , SubIndexInfo(InitSubIndexInfo(file.GetFormat(), file.GetVersion(), hasSubIndex))
        {
            Y_ASSERT(File.IsOpen());
            *Text = 0;
        }
        ui32 GetIndexVersion() const {
            return File.GetVersion();
        }
        const TSubIndexInfo& GetSubIndexInfo() const {
            return SubIndexInfo;
        }
        i64 GetFilePosition() {
            return File.GetFilePosition();
        }
        bool NeedNextBlock() {
            return !File.Valid();
        }
        bool ReadNext() {
            if (!File.Valid()) {
                File.NextBlock();
                if (!File.Valid()) {
                    return false;
                }
            }
            if (!File.ReadKeyText(Text)) {
                return false;
            }
            if (!ReadKeyData()) {
                return false;
            }
            File.ReadKeyPadding();
            return true;
        }
        i64 GetCount() const {
            return Count;
        }
        ui32 GetLength() const {
            return Length;
        }
        i64 GetOffset() const {
            return Offset;
        }
        EHitFormat GetHitFormat() const {
            return HitFormat;
        }
        const char* GetKeyText() const {
            return Text;
        }
        int DecodeKey(TKeyLemmaInfo& lemmaInfo, char (*forms)[MAXKEY_BUF]) {
            return ::DecodeKey(Text, &lemmaInfo, forms);
        }
        //! returns size of hits including subindex
        ui32 GetSizeOfHits() const {
            return hitsSize(Offset, Length, Count, SubIndexInfo);
        }
        template <typename TMappingOrFile, typename TDecoder>
        void InitPosIterator(TMappingOrFile& file, TPosIterator<TDecoder>& it, i64 offset, ui32 length, i64 count) {
            TBlob blob = CreateBlob(file, offset, hitsSize(offset, length, count, SubIndexInfo));
            ::TIndexInfo ii;
            ii.SubIndexInfo = SubIndexInfo;
            ii.Version = File.GetVersion();
            it.Init(ii, blob, offset, offset, length, count);
        }
        template <typename TMappingOrFile, typename TDecoder>
        void InitPosIterator(TMappingOrFile& file, TPosIterator<TDecoder>& it) {
            InitPosIterator(file, it, GetOffset(), GetLength(), GetCount());
        }

        void SetInvOffset(i64 invOffset) {
            Offset = invOffset;
        }

        //! @note after call to this function ReadNext() must be called because
        //!       the Text, Count and Length members are reset
        void SkipTo(i64 keyOffset, i64 invOffset) {
            File.SeekKeyFile(keyOffset);
            *Text = 0;
            Count = 0;
            Length = 0;
            Offset = invOffset;
        }
    };

    typedef TInvKeyReaderImpl<TInputIndexFile> TInvKeyReader;

} // NIndexerCore
