#pragma once

#include "attribute.h"
#include "tag.h"

namespace NHtml5 {
    // An enum representing the type of token.
    enum ETokenType {
        TOKEN_DOCTYPE,
        TOKEN_START_TAG,
        TOKEN_END_TAG,
        TOKEN_COMMENT,
        TOKEN_WHITESPACE,
        TOKEN_CHARACTER,
        TOKEN_NULL,
        TOKEN_EOF
    };

    // Struct containing all information pertaining to doctype tokens.
    struct TTokenDocType {
        TStringPiece Name;
        TStringPiece PublicIdentifier;
        TStringPiece SystemIdentifier;
        bool ForceQuirks;

        inline bool HasPublicIdentifier() const {
            return PublicIdentifier.Data != nullptr;
        }

        inline bool HasSystemIdentifier() const {
            return SystemIdentifier.Data != nullptr;
        }
    };

    struct TTokenStartTag {
        ETag Tag;
        bool IsSelfClosing;
        TRange<TAttribute> Attributes;
    };

    /**
 * A data structure representing a single token in the input stream.  This
 * contains an enum for the type, the source position, a TStringPiece
 * pointing to the original text, and then a union for any parsed data.
 */
    struct TToken {
        ETokenType Type;
        TStringPiece OriginalText;
        union {
            TTokenDocType DocType;
            TTokenStartTag StartTag;
            TStringPiece Text;
            ETag EndTag;
            int Character;
        } v;
    };

}
