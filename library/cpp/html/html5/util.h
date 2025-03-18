#pragma once

#include <stddef.h>

namespace NHtml5 {
    /**
     */
    template <typename T>
    struct TRange {
        T* Data;
        size_t Length;

        T* begin() {
            return Data;
        }
        T* end() {
            return Data + Length;
        }
    };

    /**
     * A struct representing a string or part of a string.  Strings within the
     * parser are represented by a char* and a length; the char* points into
     * an existing data buffer owned by some other code (often the original input).
     * GumboStringPieces are assumed (by convention) to be immutable, because they
     * may share data.  Use GumboStringBuffer if you need to construct a string.
     * Clients should assume that it is not NUL-terminated, and should always use
     * explicit lengths when manipulating them.
     */
    struct TStringPiece {
        /** A pointer to the beginning of the string.  NULL iff length == 0. */
        const char* Data;

        /** The length of the string fragment, in bytes.  May be zero. */
        size_t Length;

        static inline TStringPiece Empty() {
            const TStringPiece ret = {nullptr, 0};
            return ret;
        }
    };

}
