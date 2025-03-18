#pragma once

#include "insertion_mode.h"
#include "tag.h"
#include "token.h"
#include "util.h"

#include <util/generic/fwd.h>
#include <util/system/types.h>

#include <stdint.h>

namespace NHtml5 {
    enum EErrorType {
        ERR_UTF8_INVALID,
        ERR_UTF8_TRUNCATED,
        ERR_UTF8_NULL,
        ERR_NUMERIC_CHAR_REF_NO_DIGITS,
        ERR_NUMERIC_CHAR_REF_WITHOUT_SEMICOLON,
        ERR_NUMERIC_CHAR_REF_INVALID,
        ERR_NAMED_CHAR_REF_WITHOUT_SEMICOLON,
        ERR_NAMED_CHAR_REF_INVALID,
        ERR_TAG_STARTS_WITH_QUESTION,
        ERR_TAG_EOF,
        ERR_TAG_INVALID,
        ERR_CLOSE_TAG_EMPTY,
        ERR_CLOSE_TAG_EOF,
        ERR_CLOSE_TAG_INVALID,
        ERR_CLOSE_TAG_WITH_ATTRS,
        ERR_SCRIPT_EOF,
        ERR_ATTR_NAME_EOF,
        ERR_ATTR_NAME_INVALID,
        ERR_ATTR_DOUBLE_QUOTE_EOF,
        ERR_ATTR_SINGLE_QUOTE_EOF,
        ERR_ATTR_UNQUOTED_EOF,
        ERR_ATTR_UNQUOTED_RIGHT_BRACKET,
        ERR_ATTR_UNQUOTED_EQUALS,
        ERR_ATTR_AFTER_EOF,
        ERR_ATTR_AFTER_INVALID,
        ERR_ATTR_TOO_MANY,
        ERR_DUPLICATE_ATTR,
        ERR_SOLIDUS_EOF,
        ERR_SOLIDUS_INVALID,
        ERR_DASHES_OR_DOCTYPE,
        ERR_COMMENT_EOF,
        ERR_COMMENT_INVALID,
        ERR_COMMENT_BANG_AFTER_DOUBLE_DASH,
        ERR_COMMENT_DASH_AFTER_DOUBLE_DASH,
        ERR_COMMENT_SPACE_AFTER_DOUBLE_DASH,
        ERR_COMMENT_END_BANG_EOF,
        ERR_DOCTYPE_EOF,
        ERR_DOCTYPE_INVALID,
        ERR_DOCTYPE_SPACE,
        ERR_DOCTYPE_RIGHT_BRACKET,
        ERR_DOCTYPE_SPACE_OR_RIGHT_BRACKET,
        ERR_DOCTYPE_END,
        ERR_PARSER,
        ERR_UNACKNOWLEDGED_SELF_CLOSING_TAG,
    };

    // A simplified representation of the tokenizer state, designed to be more
    // useful to clients of this library than the internal representation.  This
    // condenses the actual states used in the tokenizer state machine into a few
    // values that will be familiar to users of HTML.
    enum ETokenizerErrorState {
        ERR_TOKENIZER_DATA,
        ERR_TOKENIZER_CHAR_REF,
        ERR_TOKENIZER_RCDATA,
        ERR_TOKENIZER_RAWTEXT,
        ERR_TOKENIZER_PLAINTEXT,
        ERR_TOKENIZER_SCRIPT,
        ERR_TOKENIZER_TAG,
        ERR_TOKENIZER_SELF_CLOSING_TAG,
        ERR_TOKENIZER_ATTR_NAME,
        ERR_TOKENIZER_ATTR_VALUE,
        ERR_TOKENIZER_MARKUP_DECLARATION,
        ERR_TOKENIZER_COMMENT,
        ERR_TOKENIZER_DOCTYPE,
        ERR_TOKENIZER_CDATA,
    };

    // Additional data for tokenizer errors.
    // This records the current state and codepoint encountered - this is usually
    // enough to reconstruct what went wrong and provide a friendly error message.
    struct TTokenizerError {
        // The bad codepoint encountered.
        int Codepoint;

        // The state that the tokenizer was in at the time.
        ETokenizerErrorState State;
    };

    // Additional data for duplicated attributes.
    struct TDuplicateAttrError {
        // The name of the attribute.  Owned by this struct.
        const char* Name;

        // The (0-based) index within the attributes vector of the original
        // occurrence.
        unsigned int OriginalIndex;

        // The (0-based) index where the new occurrence would be.
        unsigned int NewIndex;
    };

    // Additional data for parse errors.
    struct TParserError {
        // The type of input token that resulted in this error.
        ETokenType InputType;

        // The HTML tag of the input token.  TAG_UNKNOWN if this was not a tag token.
        ETag InputTag;

        // The insertion mode that the parser was in at the time.
        EInsertionMode ParserState;

        // The tag stack at the point of the error.  Note that this is an TVectorType
        // of ETag's *stored by value* - cast the void* to an ETag directly to
        // get at the tag.
        //TVectorType<ETag> TagStack;
    };

    /**
 * The overall error struct representing an error in decoding/tokenizing/parsing
 * the HTML.  This contains an enumerated type flag, a source position, and then
 * a union of fields containing data specific to the error.
 */
    struct TError {
        // The type of error.
        EErrorType Type;

        // Short error description. Memory is freed after IErrorHandler::OnError return.
        TStringPiece Description;

        // A pointer to the byte within the original source file text where the error
        // occurred (note that this is not the same as position.offset, as that gives
        // character-based instead of byte-based offsets).
        const char* OriginalText;

        // Type-specific error information.
        union {
            // The code point we encountered, for:
            // * ERR_UTF8_INVALID
            // * ERR_UTF8_TRUNCATED
            // * ERR_NUMERIC_CHAR_REF_WITHOUT_SEMICOLON
            // * ERR_NUMERIC_CHAR_REF_INVALID
            ui64 Codepoint;

            // Tokenizer errors.
            TTokenizerError Tokenizer;

            // Short textual data, for:
            // * ERR_NAMED_CHAR_REF_WITHOUT_SEMICOLON
            // * ERR_NAMED_CHAR_REF_INVALID
            TStringPiece Text;

            // Duplicate attribute data, for ERR_DUPLICATE_ATTR.
            TDuplicateAttrError DuplicateAttr;

            // Parser state, for ERR_PARSER and
            // ERR_UNACKNOWLEDGE_SELF_CLOSING_TAG.
            TParserError Parser;
        };
    };

    class IErrorHandler {
    public:
        virtual ~IErrorHandler() {
        }

        virtual void OnError(const TError& error) = 0;
    };

}

//autogenerated
const TString& ToString(NHtml5::EErrorType);
