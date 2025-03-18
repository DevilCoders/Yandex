#pragma once

#include <util/generic/strbuf.h>

namespace NCodecTrie {
    class TRawPair {
        char* Begin = nullptr;
        ui32 KeyLen = 0;
        ui32 ValLen = 0;

    public:
        TRawPair() = default;

        TRawPair(TStringBuf key, ui32 valLen)
            : Begin((char*)malloc(key.size() + valLen))
            , KeyLen(key.size())
            , ValLen(valLen)
        {
            memcpy(Begin, key.begin(), KeyLen);
        }

        TRawPair(const TRawPair& other)

        {
            Assign(other.GetKey(), other.GetVal());
        }

        TRawPair(TStringBuf key, TStringBuf val) {
            Assign(key, val);
        }

        void Swap(TRawPair& p) {
            DoSwap(Begin, p.Begin);
            DoSwap(KeyLen, p.KeyLen);
            DoSwap(ValLen, p.ValLen);
        }

        void Assign(TStringBuf key, TStringBuf val) {
            if (key.size() + val.size() > KeyLen + ValLen || !Begin) {
                Clear();
                Begin = (char*)malloc(key.size() + val.size());
            }

            KeyLen = key.size();
            ValLen = val.size();

            memcpy(Begin, key.begin(), KeyLen);
            memcpy(Begin + KeyLen, val.begin(), ValLen);
        }

        TStringBuf GetKey() const {
            return TStringBuf(Begin, KeyLen);
        }

        TStringBuf GetVal() const {
            return TStringBuf(Begin + KeyLen, ValLen);
        }

        void SetVal(TStringBuf val) {
            Y_VERIFY(Begin && val.size() == ValLen, " ");
            memcpy(Begin + KeyLen, val.begin(), ValLen);
        }

        TRawPair& operator=(const TRawPair& other) {
            if (this != &other)
                Assign(other.GetKey(), other.GetVal());
            return *this;
        }

        void Clear() {
            if (Begin) {
                free(Begin);
                Begin = nullptr;
            }
        }

        friend bool operator<(const TRawPair& a, const TRawPair& b) {
            return a.GetKey() < b.GetKey();
        }

        friend bool operator==(const TRawPair& a, const TRawPair& b) {
            return a.GetKey() == b.GetKey();
        }

        ~TRawPair() {
            Clear();
        }
    };

}

namespace std {
    inline void swap(NCodecTrie::TRawPair& a, NCodecTrie::TRawPair& b) {
        a.Swap(b);
    }
}
