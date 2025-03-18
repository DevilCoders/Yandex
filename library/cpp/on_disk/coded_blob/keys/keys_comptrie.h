#pragma once

#include <library/cpp/on_disk/coded_blob/common/coded_blob_utils.h>

#include <library/cpp/containers/comptrie/comptrie_trie.h>

namespace NCodedBlob {
    class TCompactTrieKeys {
    public:
        using TPrimaryKeys = TCompactTrie<char, ui64>;

    public:
        class TIterator {
            enum EIteratorState {
                IS_NOT_READY = 0,
                IS_EMPTY_KEY,
                IS_TRIE,
                IS_NONE
            };

        public:
            TIterator(const TCompactTrieKeys* keys = nullptr);

            TStringBuf GetCurrentKey() const {
                if (IS_EMPTY_KEY == IteratorState) {
                    return "";
                }

                return KeyHelper.GetCurrent(this);
            }

            ui64 GetCurrentOffset() const {
                return IS_EMPTY_KEY == IteratorState ? Keys->EmptyKeyOffset : Begin.GetValue();
            }

            bool HasNext() const {
                bool hasnext = false;
                switch (IteratorState) {
                    case IS_NONE:
                        return false;
                    case IS_NOT_READY:
                        hasnext |= Keys->HasEmptyKey();
                        break;
                    case IS_EMPTY_KEY:
                    case IS_TRIE:
                        break;
                }

                return hasnext || NextStep != End;
            }

            bool Next();

        private:
            TStringBuf FetchCurrent(NUtils::TKeyTag, TString& s) const {
                return s = Begin.GetKey();
            }

        private:
            NUtils::TKeyHelper KeyHelper;

            const TCompactTrieKeys* Keys;
            TPrimaryKeys::TConstIterator Begin;
            TPrimaryKeys::TConstIterator NextStep;
            TPrimaryKeys::TConstIterator End;
            EIteratorState IteratorState;

            friend NUtils::TKeyHelper;
        };

        void Init(TBlob b);

        TIterator Iterator() const {
            return TIterator(this);
        }

        size_t Size() const {
            return Keys.Data().Size();
        }

        bool GetOffsetByKey(TStringBuf key, ui64& offset) const {
            if (Y_UNLIKELY(!key)) {
                return GetOffsetByEmptyKey(offset);
            } else {
                return Keys.Find(key, &offset);
            }
        }

        bool GetOffsetByLongestPrefix(TStringBuf key, ui64& offset) const {
            if (Y_UNLIKELY(!key)) {
                return GetOffsetByEmptyKey(offset);
            } else {
                return Keys.FindLongestPrefix(key, &offset);
            }
        }

    private:
        bool HasEmptyKey() const {
            return NUtils::INVALID_OFFSET != EmptyKeyOffset;
        }

        bool GetOffsetByEmptyKey(ui64& offset) const {
            if (!HasEmptyKey()) {
                return false;
            }

            offset = EmptyKeyOffset;
            return true;
        }

    private:
        TPrimaryKeys Keys;
        ui64 EmptyKeyOffset = NUtils::INVALID_OFFSET;
    };

}
