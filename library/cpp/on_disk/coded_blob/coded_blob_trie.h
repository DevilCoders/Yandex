#pragma once

#include "coded_blob.h"

#include <library/cpp/codecs/tls_cache.h>
#include <library/cpp/on_disk/coded_blob/keys/keys_comptrie.h>

namespace NCodedBlob {
    const ui64 CODED_BLOB_TRIE_VERSION = 0;
    const char CODED_BLOB_TRIE_MAGIC[] = "CODEDTRI";

    class TCodedBlobTrie {
    public:
        enum ELoadMode {
            LM_RAM = 1,
            LM_MMAP = 2,
            LM_RAM_MMAP = 3
        };

        using TOffsetIterator = TCodedBlob::TOffsetIterator;

        class TPrimaryKeyIterator {
        public:
            TPrimaryKeyIterator(const TCodedBlobTrie* trie = nullptr)
                : Trie(trie)
                , Iterator(trie ? trie->Keys.Iterator() : TCompactTrieKeys::TIterator())
            {
            }

            TStringBuf GetCurrentKey() const {
                return Iterator.GetCurrentKey();
            }

            TStringBuf GetCurrentValue() const {
                return ValueHelper.GetCurrent(this);
            }

            ui64 GetCurrentOffset() const {
                return Iterator.GetCurrentOffset();
            }

            bool HasNext() const {
                return Iterator.HasNext();
            }

            bool Next() {
                if (!HasNext()) {
                    return false;
                }

                Iterator.Next();
                ValueHelper.Invalidate();
                return true;
            }

        private:
            TStringBuf FetchCurrent(NUtils::TValueTag, TBuffer& b) const {
                return Trie->Data.GetByOffset(GetCurrentOffset(), b);
            }

        private:
            NUtils::TValueHelper ValueHelper;

            const TCodedBlobTrie* Trie;
            TCompactTrieKeys::TIterator Iterator;

            friend NUtils::TValueHelper;
        };

    public:
        TOffsetIterator OffsetIterator() const {
            return TOffsetIterator(&Data);
        }

        TStringBuf GetByOffset(ui64 offset) const {
            return Data.GetByOffset(offset);
        }

        TStringBuf GetByOffset(ui64 offset, TBuffer& buffer) const {
            return Data.GetByOffset(offset, buffer);
        }

        TStringBuf GetByOffsetRaw(ui64 offset) const {
            return Data.GetByOffsetRaw(offset);
        }

        ui64 Size() const {
            return TotalSize;
        }

        size_t GetKeysSize() const {
            return Keys.Size();
        }

        size_t GetDataSize() const {
            return Data.Size();
        }

        ui64 Count() const {
            return Data.Count();
        }

        TString GetCodecName() const {
            return Data.GetCodecName();
        }

        TString GetSerializedCodecData() const {
            return Data.GetSerializedCodecData();
        }

    public:
        TCodedBlobTrie() = default;

        TCodedBlobTrie(const TBlob& b, ELoadMode loadmode) {
            Init(b, loadmode);
        }

        void Init(TBlob b, ELoadMode loadmode = LM_RAM);

        bool GetOffsetByPrimaryKey(TStringBuf key, ui64& offset) const {
            return Keys.GetOffsetByKey(key, offset);
        }

        bool GetOffsetByLongestPrefix(TStringBuf key, ui64& offset) const {
            return Keys.GetOffsetByLongestPrefix(key, offset);
        }

        bool GetByPrimaryKey(TStringBuf key, TStringBuf& payload) const {
            auto tmpBuffer = NCodecs::TBufferTlsCache::TlsInstance().Item();
            return GetByPrimaryKey(key, payload, tmpBuffer.Get());
        }

        bool GetByPrimaryKey(TStringBuf key, TStringBuf& payload, TBuffer& buffer) const {
            ui64 offset = 0;
            if (Y_UNLIKELY(!GetOffsetByPrimaryKey(key, offset))) {
                return false;
            }

            payload = GetByOffset(offset, buffer);
            return true;
        }

        bool GetByPrimaryKeyRaw(TStringBuf key, TStringBuf& payload) const {
            ui64 offset = 0;
            if (Y_UNLIKELY(!GetOffsetByPrimaryKey(key, offset))) {
                return false;
            }

            payload = GetByOffsetRaw(offset);
            return true;
        }

        template <typename TStringType>
        bool GetByPrimaryKey(TStringBuf key, TStringType& payload) const {
            TStringBuf data;
            if (Y_UNLIKELY(!GetByPrimaryKey(key, data))) {
                return false;
            }

            NAccessors::Assign(payload, data.data(), data.size());
            return true;
        }

        bool GetByLongestPrefix(TStringBuf key, TStringBuf& payload) const {
            auto tmpBuffer = NCodecs::TBufferTlsCache::TlsInstance().Item();
            return GetByLongestPrefix(key, payload, tmpBuffer.Get());
        }

        bool GetByLongestPrefix(TStringBuf key, TStringBuf& payload, TBuffer& buffer) const {
            ui64 offset = 0;
            if (Y_UNLIKELY(!GetOffsetByLongestPrefix(key, offset))) {
                return false;
            }

            payload = GetByOffset(offset, buffer);
            return true;
        }

        TPrimaryKeyIterator PrimaryKeyIterator() const {
            return TPrimaryKeyIterator(this);
        }

        ELoadMode GetLoadMode() const {
            return LoadMode;
        }

        TString ReportLoadMode() const;

        ui64 ApproxLoadedSize(bool ramOnly) const;

        static ui64 PredictLoadSize(TBlob b, ELoadMode mode);

    private:
        TCompactTrieKeys Keys;
        TCodedBlob Data;
        ui64 TotalSize = 0;
        ELoadMode LoadMode = LM_RAM;
    };

}
