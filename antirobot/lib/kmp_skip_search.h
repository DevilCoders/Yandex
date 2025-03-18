#pragma once

#include "ar_utils.h"

namespace NAntiRobot {
    /// Implements KMP Skip Search algorithm for substring search
    class TKmpSkipSearch {
    public:
        TKmpSkipSearch(const char* searchString);
        ~TKmpSkipSearch();

        /**
         * If the string stored in the field @a SearchString is a substring
         * of @a text the method returns a pointer to the the first occurrence of
         * @a SearchString in @a text. The returned pointer points to the first symbol
         * of @a SearchString.
         *
         * If @a SearchString is not a substring of @a text the method returns NULL.
         */
        const char* SearchInText(const char* text, size_t textLen) const;

        inline TStringBuf SearchInText(const TStringBuf& t) const noexcept {
            const char* c = SearchInText(t.data(), t.size());
            if (c)
                return t.SubStr(c - t.data());

            return TStringBuf();
        }

        inline bool InText(const TStringBuf& t) const noexcept {
            return (bool)SearchInText(t.data(), t.size());
        }

        inline bool SubStr(const char* begin, const char* end, const char*& result) {
            result = SearchInText(begin, end - begin);

            return !!result;
        }

        inline size_t Length() const noexcept {
            return SearchStringLen;
        }

    private:
        static const size_t MaxSearchStringSize = 50;
        static const size_t AlphabetSize = 255;

        size_t SearchStringLen;
        char SearchString[MaxSearchStringSize];

        int KmpNext[MaxSearchStringSize];
        int MpNext[MaxSearchStringSize];
        int List[MaxSearchStringSize];
        int Z[AlphabetSize];
    };
}
