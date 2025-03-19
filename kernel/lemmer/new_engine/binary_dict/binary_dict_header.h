#pragma once

#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>
#include <util/system/yassert.h>

namespace NNewLemmer {
    enum EField {
        F_GRAMMAR = 0,
        F_GRAMMAR_ADDRS,
        F_FLEXIES,
        F_FLEXIES_ADDRS,
        F_FLEX_TRIES,
        F_FLEX_TRIES_ADDRS,
        F_ENDS_TRIE,
        F_SCHEMES_LEMMA_FLEX,
        F_SCHEMES_STEM_GRAM,
        F_SCHEMES_BLOCK_ID,
        F_SCHEMES_FREQ_ID,
        F_BLOCKS_FORM_FLEX,
        F_BLOCKS_FLEX_GRAM,
        F_BLOCKS_FREQ_ID,
        F_END_INFO,
        F_END_DIA_MASK,
        F_END_FREQ_ID,
        F_ANAS_LISTS,
        F_FREQS,
        F_DEFAULT_FLEX_GRAM,
        F_FINGERPRINT = 30,
        F_MAX = F_FINGERPRINT
    };

    struct TEndInfoImpl {
        ui16 SchemeId : 16;
        ui16 StemLen : 6;
        ui16 HasPrefix : 1;
        ui16 IsFinal : 1;
        ui16 Rest : 3;
        ui16 Reserved : 1; //reserved for future use
        ui16 UseAlways : 1;
        ui16 OldFlexLen : 3;
        ui32 Pack() const {
            static_assert(sizeof(*this) == sizeof(ui32), "wrong size of EndInfo - it should be packable to ui32");
            return *(reinterpret_cast<const ui32*>(this));
        }
        TEndInfoImpl(size_t schemeId, size_t stemLen, bool hasPrefix, bool isFinal, size_t rest, size_t oldFlexLen)
            : SchemeId(schemeId)
            , StemLen(stemLen)
            , HasPrefix(hasPrefix)
            , IsFinal(isFinal)
            , Rest(rest)
            , Reserved(0)
            , UseAlways(0)
            , OldFlexLen(oldFlexLen)
        {
            Y_ENSURE(schemeId < (1 << 16));
            Y_ENSURE(stemLen < (1 << 6));
            Y_ENSURE(rest < (1 << 3));
            Y_ENSURE(rest <= 4);
            Y_ENSURE(oldFlexLen < (1 << 3));
        }
        TEndInfoImpl()
            : TEndInfoImpl(0, 0, 0, 0, 0, 0)
        {
        }
        bool operator==(const TEndInfoImpl& other) const {
            return SchemeId == other.SchemeId
                && StemLen == other.StemLen
                && HasPrefix == other.HasPrefix
                && IsFinal == other.IsFinal
                && Rest == other.Rest
                && UseAlways == other.UseAlways
                && OldFlexLen == other.OldFlexLen;
        }
    };

    constexpr ui32 BYTES[(size_t)F_MAX + 1] = {
        1, // F_GRAMMAR
        4, // F_GRAMMAR_ADDRS
        1, // F_FLEXIES
        4, // F_FLEXIES_ADDRS
        1, // F_FLEX_TRIES
        4, // F_FLEX_TRIES_ADDRS
        1, // F_ENDS_TRIE
        4, // F_SCHEMES_LEMMA_FLEX
        2, // F_SCHEMES_STEM_GRAM
        4, // F_SCHEMES_BLOCK_ID
        2, // F_SCHEMES_FREQ_ID
        4, // F_BLOCKS_FORM_FLEX
        2, // F_BLOCKS_FLEX_GRAM
        2, // F_BLOCKS_FREQ_ID
        4, // F_END_INFO, == sizeof(TEndInfoImpl)
        2, // F_END_DIA_MASK
        2, // F_END_FREQ_ID
        4, // F_ANAS_LISTS
        4, // F_FREQS
        0};

    constexpr ui32 HAS_NEXT_GRAMMAR_BIT = 1u << (8 * BYTES[F_GRAMMAR_ADDRS] - 1);
    constexpr ui32 HAS_NEXT_BLOCK_BIT = 1u << (8 * BYTES[F_BLOCKS_FORM_FLEX] - 1);
    constexpr ui32 HAS_NEXT_ANA_BIT = 1u << (8 * BYTES[F_ANAS_LISTS] - 1);

    struct THeader {
        static const ui16 CurrentMagic = 0x444C; // LD, stands for "lemmer dictionary"
        static const ui16 CurrentFormatVersion = 3;

        ui16 Magic;
        ui16 FormatVersion;
        ui32 Reserved;

        ui32 Offset[(size_t)F_MAX + 1];
        ui32 Length[(size_t)F_MAX + 1];

        void Init() {
            Magic = CurrentMagic;
            FormatVersion = CurrentFormatVersion;
            Reserved = 0;
            Zero(Offset);
            Zero(Length);
        }
    };

    template <typename T, typename S>
    static T UnpackImpl(const S* ptr) {
        return *reinterpret_cast<const T*>(ptr);
    }

    template <typename T, typename S>
    static const T* UnpackPtrImpl(const S* ptr) {
        return reinterpret_cast<const T*>(ptr);
    }

    class TData {
    public:
        TData()
            : Data(nullptr)
            , Size(0)
            , Header(nullptr)
        {
        }

        TData(const char* data, size_t size)
            : Data(data)
            , Size(size)
            , Header(reinterpret_cast<const THeader*>(data))
        {
            Y_ASSERT(data != nullptr);
            Y_ENSURE(Header->Magic == THeader::CurrentMagic);
            Y_ENSURE(Header->FormatVersion <= THeader::CurrentFormatVersion);
        }

        const char* operator[](EField field) const {
            return Data + Header->Offset[(size_t)field];
        }

        size_t Len(EField field) const {
            return Header->Length[(size_t)field];
        }

        bool Has(EField field) const {
            return Len(field) != 0;
        }

        TString Fingerprint() const;

        template <typename T>
        T Unpack(EField field) const {
            Y_ASSERT(BYTES[field] == 0);
            return UnpackImpl<T>((*this)[field]);
        }

        template <typename T>
        T Unpack(EField field, size_t itemId) const {
            Y_ASSERT(sizeof(T) == BYTES[field]);
            return UnpackImpl<T>((*this)[field] + itemId * BYTES[field]);
        }

        template <typename T>
        const T* UnpackPtr(EField field, size_t itemId) const {
            Y_ASSERT(BYTES[field] == 1 || sizeof(T) == BYTES[field]);
            return UnpackPtrImpl<T>((*this)[field] + itemId * BYTES[field]);
        }

        ui32 GetFreq(ui16 rank) const {
            return Has(F_FREQS) ? Unpack<ui32>(F_FREQS, rank) : 0;
        }

        ui16 GetFormatVersion() const {
            Y_ASSERT(Header);
            return Header->FormatVersion;
        }

    private:
        const char* Data;
        size_t Size;
        const THeader* Header;
    };

}
