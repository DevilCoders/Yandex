#pragma once

#include "attribute.h"
#include "buffer.h"
#include "error.h"
#include "tag.h"
#include "token.h"
#include "util.h"

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <util/system/compat.h>

namespace NHtml5 {
    // An enum representing the state of tokenization.
    enum ETokenizerState {
        LEX_DATA,
        LEX_CHAR_REF_IN_DATA,
        LEX_RCDATA,
        LEX_CHAR_REF_IN_RCDATA,
        LEX_RAWTEXT,
        LEX_SCRIPT,
        LEX_PLAINTEXT,
        LEX_TAG_OPEN,
        LEX_END_TAG_OPEN,
        LEX_TAG_NAME,
        LEX_RCDATA_LT,
        LEX_RCDATA_END_TAG_OPEN,
        LEX_RCDATA_END_TAG_NAME,
        LEX_RAWTEXT_LT,
        LEX_RAWTEXT_END_TAG_OPEN,
        LEX_RAWTEXT_END_TAG_NAME,
        LEX_SCRIPT_LT,
        LEX_SCRIPT_END_TAG_OPEN,
        LEX_SCRIPT_END_TAG_NAME,
        LEX_SCRIPT_ESCAPED_START,
        LEX_SCRIPT_ESCAPED_START_DASH,
        LEX_SCRIPT_ESCAPED,
        LEX_SCRIPT_ESCAPED_DASH,
        LEX_SCRIPT_ESCAPED_DASH_DASH,
        LEX_SCRIPT_ESCAPED_LT,
        LEX_SCRIPT_ESCAPED_END_TAG_OPEN,
        LEX_SCRIPT_ESCAPED_END_TAG_NAME,
        LEX_SCRIPT_DOUBLE_ESCAPED_START,
        LEX_SCRIPT_DOUBLE_ESCAPED,
        LEX_SCRIPT_DOUBLE_ESCAPED_DASH,
        LEX_SCRIPT_DOUBLE_ESCAPED_DASH_DASH,
        LEX_SCRIPT_DOUBLE_ESCAPED_LT,
        LEX_SCRIPT_DOUBLE_ESCAPED_END,
        LEX_BEFORE_ATTR_NAME,
        LEX_ATTR_NAME,
        LEX_AFTER_ATTR_NAME,
        LEX_BEFORE_ATTR_VALUE,
        LEX_ATTR_VALUE_DOUBLE_QUOTED,
        LEX_ATTR_VALUE_SINGLE_QUOTED,
        LEX_ATTR_VALUE_UNQUOTED,
        LEX_CHAR_REF_IN_ATTR_VALUE,
        LEX_AFTER_ATTR_VALUE_QUOTED,
        LEX_SELF_CLOSING_START_TAG,
        LEX_BOGUS_COMMENT,
        LEX_MARKUP_DECLARATION,
        LEX_COMMENT_START,
        LEX_COMMENT_START_DASH,
        LEX_COMMENT,
        LEX_COMMENT_END_DASH,
        LEX_COMMENT_END,
        LEX_COMMENT_END_BANG,
        LEX_DOCTYPE,
        LEX_BEFORE_DOCTYPE_NAME,
        LEX_DOCTYPE_NAME,
        LEX_AFTER_DOCTYPE_NAME,
        LEX_AFTER_DOCTYPE_PUBLIC_KEYWORD,
        LEX_BEFORE_DOCTYPE_PUBLIC_ID,
        LEX_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED,
        LEX_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED,
        LEX_AFTER_DOCTYPE_PUBLIC_ID,
        LEX_BETWEEN_DOCTYPE_PUBLIC_SYSTEM_ID,
        LEX_AFTER_DOCTYPE_SYSTEM_KEYWORD,
        LEX_BEFORE_DOCTYPE_SYSTEM_ID,
        LEX_DOCTYPE_SYSTEM_ID_DOUBLE_QUOTED,
        LEX_DOCTYPE_SYSTEM_ID_SINGLE_QUOTED,
        LEX_AFTER_DOCTYPE_SYSTEM_ID,
        LEX_BOGUS_DOCTYPE,
        LEX_CDATA
    };

    /**
     */
    struct TTokenizerOptions {
        // A callback for receiving tokenizer errors.
        IErrorHandler* Errors;
        //
        int MaxAttributes;
        // If true close <plaintext> and continue parse document as ordinal html.
        bool CompatiblePlainText;
        // if true convert <noindex> to comment token on tokenization level.
        bool NoindexToComment;

        TTokenizerOptions()
            : Errors(nullptr)
            , MaxAttributes(512)
            , CompatiblePlainText(false)
            , NoindexToComment(false)
        {
        }

        TTokenizerOptions& SetErrors(IErrorHandler* err) {
            Errors = err;
            return *this;
        }

        TTokenizerOptions& SetCompatiblePlainText(bool value) {
            CompatiblePlainText = value;
            return *this;
        }

        TTokenizerOptions& SetNoindexToComment(bool value) {
            NoindexToComment = value;
            return *this;
        }
    };

    /**
     */
    template <typename TInput>
    class TTokenizer {
        // An enum for the return value of each individual state.
        enum EStateResult {
            RETURN_ERROR,   // Return false (error) from the tokenizer.
            RETURN_SUCCESS, // Return true (success) from the tokenizer.
            NEXT_CHAR       // Proceed to the next character and continue lexing.
        };

        typedef TCodepointBuffer<TInput> TBuffer;

        static const int NoChar = -1;
        static const int EOF_CHAR = -1;

        static constexpr const char* NOINDEX_START_TEXT = "<!--noindex-->";
        static constexpr const size_t NOINDEX_START_TEXT_LEN = 14;
        static constexpr const char* NOINDEX_END_TEXT = "<!--/noindex-->";
        static constexpr const size_t NOINDEX_END_TEXT_LEN = 15;

    public:
        TTokenizer(const TTokenizerOptions& opts, const char* text, size_t text_length)
            : State_(LEX_DATA)
            , ReconsumeCurrentInput_(false)
            , IsCurrentNodeForeign_(false)
            , TemporaryBufferEmit_(nullptr)
            , TokenStart_(text)
            , Input_(text, text_length)
            , Options_(opts)
        {
            TagState_.Attributes.reserve(32);
            DocTypeStateInit();
        }

        // Lexes a single token from the specified buffer, filling the output with the
        // parsed GumboToken data structure.  Returns true for a successful
        // tokenization, false if a parse error occurs.
        bool Lex(TToken* output) {
            // Because of the spec requirements that...
            //
            // 1. Tokens be handled immediately by the parser upon emission.
            // 2. Some states (eg. CDATA, or various error conditions) require the
            // emission of multiple tokens in the same states.
            // 3. The tokenizer often has to reconsume the same character in a different
            // state.
            //
            // ...all state must be held in the GumboTokenizer struct instead of in local
            // variables in this function.  That allows us to return from this method with
            // a token, and then immediately jump back to the same state with the same
            // input if we need to return a different token.  The various emit_* functions
            // are responsible for changing state (eg. flushing the chardata buffer,
            // reading the next input character) to avoid an infinite loop.

            if (MaybeEmitFromTemporaryBuffer(output)) {
                return true;
            }

            while (1) {
                assert(!TemporaryBufferEmit_);
                const int c = Input_.Current();
                EStateResult result = RETURN_ERROR;

                switch (State_) {
                    case LEX_DATA:
                        result = HandleDataState(c, output);
                        break;
                    case LEX_CHAR_REF_IN_DATA:
                        result = HandleCharRefInDataState(c, output);
                        break;
                    case LEX_RCDATA:
                        result = HandleRcdataState(c, output);
                        break;
                    case LEX_CHAR_REF_IN_RCDATA:
                        result = HandleCharRefInRcdataState(c, output);
                        break;
                    case LEX_RAWTEXT:
                        result = HandleRawtextState(c, output);
                        break;
                    case LEX_SCRIPT:
                        result = HandleScriptState(c, output);
                        break;
                    case LEX_PLAINTEXT:
                        if (Options_.CompatiblePlainText) {
                            // It is not a standart behaviour below, but it is
                            // compatible with old parser.
                            result = HandleRawtextState(c, output);
                        } else {
                            result = HandlePlaintextState(c, output);
                        }
                        break;
                    case LEX_TAG_OPEN:
                        result = HandleTagOpenState(c, output);
                        break;
                    case LEX_END_TAG_OPEN:
                        result = HandleEndTagOpenState(c, output);
                        break;
                    case LEX_TAG_NAME:
                        result = HandleTagNameState(c, output);
                        break;
                    case LEX_RCDATA_LT:
                        result = HandleRcdataLtState(c, output);
                        break;
                    case LEX_RCDATA_END_TAG_OPEN:
                        result = HandleRcdataEndTagOpenState(c, output);
                        break;
                    case LEX_RCDATA_END_TAG_NAME:
                        result = HandleRcdataEndTagNameState(c, output);
                        break;
                    case LEX_RAWTEXT_LT:
                        result = HandleRawtextLtState(c, output);
                        break;
                    case LEX_RAWTEXT_END_TAG_OPEN:
                        result = HandleRawtextEndTagOpenState(c, output);
                        break;
                    case LEX_RAWTEXT_END_TAG_NAME:
                        result = HandleRawtextEndTagNameState(c, output);
                        break;
                    case LEX_SCRIPT_LT:
                        result = HandleScriptLtState(c, output);
                        break;
                    case LEX_SCRIPT_END_TAG_OPEN:
                        result = HandleScriptEndTagOpenState(c, output);
                        break;
                    case LEX_SCRIPT_END_TAG_NAME:
                        result = HandleScriptEndTagNameState(c, output);
                        break;
                    case LEX_SCRIPT_ESCAPED_START:
                        result = HandleScriptEscapedStartState(c, output);
                        break;
                    case LEX_SCRIPT_ESCAPED_START_DASH:
                        result = HandleScriptEscapedStartDashState(c, output);
                        break;
                    case LEX_SCRIPT_ESCAPED:
                        result = HandleScriptEscapedState(c, output);
                        break;
                    case LEX_SCRIPT_ESCAPED_DASH:
                        result = HandleScriptEscapedDashState(c, output);
                        break;
                    case LEX_SCRIPT_ESCAPED_DASH_DASH:
                        result = HandleScriptEscapedDashDashState(c, output);
                        break;
                    case LEX_SCRIPT_ESCAPED_LT:
                        result = HandleScriptEscapedLtState(c, output);
                        break;
                    case LEX_SCRIPT_ESCAPED_END_TAG_OPEN:
                        result = HandleScriptEscapedEndTagOpenState(c, output);
                        break;
                    case LEX_SCRIPT_ESCAPED_END_TAG_NAME:
                        result = HandleScriptEscapedEndTagNameState(c, output);
                        break;
                    case LEX_SCRIPT_DOUBLE_ESCAPED_START:
                        result = HandleScriptDoubleEscapedStartState(c, output);
                        break;
                    case LEX_SCRIPT_DOUBLE_ESCAPED:
                        result = HandleScriptDoubleEscapedState(c, output);
                        break;
                    case LEX_SCRIPT_DOUBLE_ESCAPED_DASH:
                        result = HandleScriptDoubleEscapedDashState(c, output);
                        break;
                    case LEX_SCRIPT_DOUBLE_ESCAPED_DASH_DASH:
                        result = HandleScriptDoubleEscapedDashDashState(c, output);
                        break;
                    case LEX_SCRIPT_DOUBLE_ESCAPED_LT:
                        result = HandleScriptDoubleEscapedLtState(c, output);
                        break;
                    case LEX_SCRIPT_DOUBLE_ESCAPED_END:
                        result = HandleScriptDoubleEscapedEndState(c, output);
                        break;
                    case LEX_BEFORE_ATTR_NAME:
                        result = HandleBeforeAttrNameState(c, output);
                        break;
                    case LEX_ATTR_NAME:
                        result = HandleAttrNameState(c, output);
                        break;
                    case LEX_AFTER_ATTR_NAME:
                        result = HandleAfterAttrNameState(c, output);
                        break;
                    case LEX_BEFORE_ATTR_VALUE:
                        result = HandleBeforeAttrValueState(c, output);
                        break;
                    case LEX_ATTR_VALUE_DOUBLE_QUOTED:
                        result = HandleAttrValueDoubleQuotedState(c, output);
                        break;
                    case LEX_ATTR_VALUE_SINGLE_QUOTED:
                        result = HandleAttrValueSingleQuotedState(c, output);
                        break;
                    case LEX_ATTR_VALUE_UNQUOTED:
                        result = HandleAttrValueUnquotedState(c, output);
                        break;
                    case LEX_CHAR_REF_IN_ATTR_VALUE:
                        result = HandleCharRefInAttrValueState(c, output);
                        break;
                    case LEX_AFTER_ATTR_VALUE_QUOTED:
                        result = HandleAfterAttrValueQuotedState(c, output);
                        break;
                    case LEX_SELF_CLOSING_START_TAG:
                        result = HandleSelfClosingStartTagState(c, output);
                        break;
                    case LEX_BOGUS_COMMENT:
                        result = HandleBogusCommentState(c, output);
                        break;
                    case LEX_MARKUP_DECLARATION:
                        result = HandleMarkupDeclarationState(c, output);
                        break;
                    case LEX_COMMENT_START:
                        result = HandleCommentStartState(c, output);
                        break;
                    case LEX_COMMENT_START_DASH:
                        result = HandleCommentStartDashState(c, output);
                        break;
                    case LEX_COMMENT:
                        result = HandleCommentState(c, output);
                        break;
                    case LEX_COMMENT_END_DASH:
                        result = HandleCommentEndDashState(c, output);
                        break;
                    case LEX_COMMENT_END:
                        result = HandleCommentEndState(c, output);
                        break;
                    case LEX_COMMENT_END_BANG:
                        result = HandleCommentEndBangState(c, output);
                        break;
                    case LEX_DOCTYPE:
                        result = HandleDoctypeState(c, output);
                        break;
                    case LEX_BEFORE_DOCTYPE_NAME:
                        result = HandleBeforeDoctypeNameState(c, output);
                        break;
                    case LEX_DOCTYPE_NAME:
                        result = HandleDoctypeNameState(c, output);
                        break;
                    case LEX_AFTER_DOCTYPE_NAME:
                        result = HandleAfterDoctypeNameState(c, output);
                        break;
                    case LEX_AFTER_DOCTYPE_PUBLIC_KEYWORD:
                        result = HandleAfterDoctypePublicKeywordState(c, output);
                        break;
                    case LEX_BEFORE_DOCTYPE_PUBLIC_ID:
                        result = HandleBeforeDoctypePublicIdState(c, output);
                        break;
                    case LEX_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED:
                        result = HandleDoctypePublicIdDoubleQuotedState(c, output);
                        break;
                    case LEX_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED:
                        result = HandleDoctypePublicIdSingleQuotedState(c, output);
                        break;
                    case LEX_AFTER_DOCTYPE_PUBLIC_ID:
                        result = HandleAfterDoctypePublicIdState(c, output);
                        break;
                    case LEX_BETWEEN_DOCTYPE_PUBLIC_SYSTEM_ID:
                        result = HandleBetweenDoctypePublicSystemIdState(c, output);
                        break;
                    case LEX_AFTER_DOCTYPE_SYSTEM_KEYWORD:
                        result = HandleAfterDoctypeSystemKeywordState(c, output);
                        break;
                    case LEX_BEFORE_DOCTYPE_SYSTEM_ID:
                        result = HandleBeforeDoctypeSystemIdState(c, output);
                        break;
                    case LEX_DOCTYPE_SYSTEM_ID_DOUBLE_QUOTED:
                        result = HandleDoctypeSystemIdDoubleQuotedState(c, output);
                        break;
                    case LEX_DOCTYPE_SYSTEM_ID_SINGLE_QUOTED:
                        result = HandleDoctypeSystemIdSingleQuotedState(c, output);
                        break;
                    case LEX_AFTER_DOCTYPE_SYSTEM_ID:
                        result = HandleAfterDoctypeSystemIdState(c, output);
                        break;
                    case LEX_BOGUS_DOCTYPE:
                        result = HandleBogusDoctypeState(c, output);
                        break;
                    case LEX_CDATA:
                        result = HandleCdataState(c, output);
                        break;
                }

                // We need to clear reconsume_current_input before returning to prevent
                // certain infinite loop states.
                const bool should_advance = !ReconsumeCurrentInput_;
                ReconsumeCurrentInput_ = false;

                switch (result) {
                    case RETURN_ERROR:
                        return false;
                    case RETURN_SUCCESS:
                        return true;
                    case NEXT_CHAR:
                        if (should_advance) {
                            Input_.Next();
                        }
                        break;
                }
            }
        }

        inline void SetIsCurrentNodeForeign(bool is_foreign) {
            IsCurrentNodeForeign_ = is_foreign;
        }

        inline void SetState(const ETokenizerState state) {
            State_ = state;
        }

    private:
        static inline ETokenType GetCharTokenType(int c) {
            switch (c) {
                case -1:
                    return TOKEN_EOF;
                case 0:
                    return TOKEN_NULL;
                case '\t':
                case '\n':
                case '\r':
                case '\f':
                case ' ':
                    return TOKEN_WHITESPACE;
                default:
                    return TOKEN_CHARACTER;
            }
        }

        static inline bool IsAlpha(int c) {
            // We don't use ISO C isupper/islower functions here because they
            // depend upon the program's locale, while the behavior of the HTML5 spec is
            // independent of which locale the program is run in.
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        }

        static inline int EnsureLowercase(int c) {
            return c >= 'A' && c <= 'Z' ? c + 0x20 : c;
        }

#ifndef NDEBUG
        // Checks to see if the temporary buffer equals a certain string.
        // Make sure this remains side-effect free; it's used in assertions.
        template <size_t Len>
        static inline bool CheckBufferEquals(const TBuffer& buf, const char (&text)[Len]) {
            size_t text_len = (Len - 1); // without null-terminator
            return text_len == buf.Length() && memcmp(buf.Data(), text, text_len) == 0;
        }
#endif

        // In some states, we speculatively start a tag, but don't know whether it'll be
        // emitted as tag token or as a series of character tokens until we finish it.
        // We need to abandon the tag we'd started & free its memory in that case to
        // avoid a memory leak.
        void AbandonCurrentTag() {
            TagState_.Attributes.clear();
            TagState_.Buffer.Reset();
        }

        // Adds an ERR_UNEXPECTED_CODE_POINT parse error to the parser's error struct.
        void AddParseError(EErrorType type) {
            if (!Options_.Errors) {
                return;
            }

            TError error;

            error.OriginalText = Input_.GetCharPointer();
            error.Type = type;
            error.Tokenizer.Codepoint = Input_.Current();
            switch (State_) {
                case LEX_DATA:
                    error.Tokenizer.State = ERR_TOKENIZER_DATA;
                    break;
                case LEX_CHAR_REF_IN_DATA:
                case LEX_CHAR_REF_IN_RCDATA:
                case LEX_CHAR_REF_IN_ATTR_VALUE:
                    error.Tokenizer.State = ERR_TOKENIZER_CHAR_REF;
                    break;
                case LEX_RCDATA:
                case LEX_RCDATA_LT:
                case LEX_RCDATA_END_TAG_OPEN:
                case LEX_RCDATA_END_TAG_NAME:
                    error.Tokenizer.State = ERR_TOKENIZER_RCDATA;
                    break;
                case LEX_RAWTEXT:
                case LEX_RAWTEXT_LT:
                case LEX_RAWTEXT_END_TAG_OPEN:
                case LEX_RAWTEXT_END_TAG_NAME:
                    error.Tokenizer.State = ERR_TOKENIZER_RAWTEXT;
                    break;
                case LEX_PLAINTEXT:
                    error.Tokenizer.State = ERR_TOKENIZER_PLAINTEXT;
                    break;
                case LEX_SCRIPT:
                case LEX_SCRIPT_LT:
                case LEX_SCRIPT_END_TAG_OPEN:
                case LEX_SCRIPT_END_TAG_NAME:
                case LEX_SCRIPT_ESCAPED_START:
                case LEX_SCRIPT_ESCAPED_START_DASH:
                case LEX_SCRIPT_ESCAPED:
                case LEX_SCRIPT_ESCAPED_DASH:
                case LEX_SCRIPT_ESCAPED_DASH_DASH:
                case LEX_SCRIPT_ESCAPED_LT:
                case LEX_SCRIPT_ESCAPED_END_TAG_OPEN:
                case LEX_SCRIPT_ESCAPED_END_TAG_NAME:
                case LEX_SCRIPT_DOUBLE_ESCAPED_START:
                case LEX_SCRIPT_DOUBLE_ESCAPED:
                case LEX_SCRIPT_DOUBLE_ESCAPED_DASH:
                case LEX_SCRIPT_DOUBLE_ESCAPED_DASH_DASH:
                case LEX_SCRIPT_DOUBLE_ESCAPED_LT:
                case LEX_SCRIPT_DOUBLE_ESCAPED_END:
                    error.Tokenizer.State = ERR_TOKENIZER_SCRIPT;
                    break;
                case LEX_TAG_OPEN:
                case LEX_END_TAG_OPEN:
                case LEX_TAG_NAME:
                case LEX_BEFORE_ATTR_NAME:
                    error.Tokenizer.State = ERR_TOKENIZER_TAG;
                    break;
                case LEX_SELF_CLOSING_START_TAG:
                    error.Tokenizer.State = ERR_TOKENIZER_SELF_CLOSING_TAG;
                    break;
                case LEX_ATTR_NAME:
                case LEX_AFTER_ATTR_NAME:
                case LEX_BEFORE_ATTR_VALUE:
                    error.Tokenizer.State = ERR_TOKENIZER_ATTR_NAME;
                    break;
                case LEX_ATTR_VALUE_DOUBLE_QUOTED:
                case LEX_ATTR_VALUE_SINGLE_QUOTED:
                case LEX_ATTR_VALUE_UNQUOTED:
                case LEX_AFTER_ATTR_VALUE_QUOTED:
                    error.Tokenizer.State = ERR_TOKENIZER_ATTR_VALUE;
                    break;
                case LEX_BOGUS_COMMENT:
                case LEX_COMMENT_START:
                case LEX_COMMENT_START_DASH:
                case LEX_COMMENT:
                case LEX_COMMENT_END_DASH:
                case LEX_COMMENT_END:
                case LEX_COMMENT_END_BANG:
                    error.Tokenizer.State = ERR_TOKENIZER_COMMENT;
                    break;
                case LEX_MARKUP_DECLARATION:
                case LEX_DOCTYPE:
                case LEX_BEFORE_DOCTYPE_NAME:
                case LEX_DOCTYPE_NAME:
                case LEX_AFTER_DOCTYPE_NAME:
                case LEX_AFTER_DOCTYPE_PUBLIC_KEYWORD:
                case LEX_BEFORE_DOCTYPE_PUBLIC_ID:
                case LEX_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED:
                case LEX_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED:
                case LEX_AFTER_DOCTYPE_PUBLIC_ID:
                case LEX_BETWEEN_DOCTYPE_PUBLIC_SYSTEM_ID:
                case LEX_AFTER_DOCTYPE_SYSTEM_KEYWORD:
                case LEX_BEFORE_DOCTYPE_SYSTEM_ID:
                case LEX_DOCTYPE_SYSTEM_ID_DOUBLE_QUOTED:
                case LEX_DOCTYPE_SYSTEM_ID_SINGLE_QUOTED:
                case LEX_AFTER_DOCTYPE_SYSTEM_ID:
                case LEX_BOGUS_DOCTYPE:
                    error.Tokenizer.State = ERR_TOKENIZER_DOCTYPE;
                    break;
                case LEX_CDATA:
                    error.Tokenizer.State = ERR_TOKENIZER_CDATA;
                    break;
            }

            OnTokenError(error);
        }

        void OnTokenError(TError& error) {
            const TStringBuf errDescr = TStringBuf(ToString(error.Type));

            error.Description.Data = errDescr.data();
            error.Description.Length = errDescr.size();

            Options_.Errors->OnError(error);
        }

        // Adds an ERR_DUPLICATE_ATTR parse error to the parser's error struct.
        void AddDuplicateAttrError(TStringPiece /*attr_name*/, int original_index, int new_index) {
            if (!Options_.Errors) {
                return;
            }

            TError error;

            error.Type = ERR_DUPLICATE_ATTR;
            error.OriginalText = TagState_.OriginalText;
            error.DuplicateAttr.OriginalIndex = original_index;
            error.DuplicateAttr.NewIndex = new_index;
            //error.DuplicateAttr.Name = attr_name;
            InitializeTagBuffer();

            OnTokenError(error);
        }

        // Appends a codepoint to the current tag buffer.  If
        // reinitilize_position_on_first is set, this also initializes the tag buffer
        // start point; the only time you would *not* want to pass true for this
        // parameter is if you want the original_text to include character (like an
        // opening quote) that doesn't appear in the value.
        void AppendCharToTagBuffer(int codepoint, bool reinitilize_position_on_first) {
            TBuffer& buf = TagState_.Buffer;

            if (buf.Length() == 0 && reinitilize_position_on_first) {
                ResetTagBufferStartPoint();
            }
            buf.AppendChar((char)codepoint);
        }

        // Appends a codepoint to the temporary buffer.
        void AppendCharToTemporaryBuffer(int codepoint) {
            TemporaryBuffer_.AppendChar((char)codepoint);
        }

        // Starts recording characters in the temporary buffer.
        // Because this needs to reset the utf8iterator_mark to the beginning of the
        // text that will eventually be emitted, it needs to be called a couple of
        // states before the spec says "Set the temporary buffer to the empty string".
        // In general, this should be called whenever there's a transition to a
        // "less-than sign state".  The initial < and possibly / then need to be
        // appended to the temporary buffer, their presence needs to be accounted for in
        // states that compare the temporary buffer against a literal value, and
        // spec stanzas that say "emit a < and / character token along with a character
        // token for each character in the temporary buffer" need to be adjusted to
        // account for the presence of the < and / inside the temporary buffer.
        void ClearTemporaryBuffer() {
            assert(!TemporaryBufferEmit_);
            Input_.Mark();
            TemporaryBuffer_.Reset();
            // The temporary buffer and script data buffer are the same object in the
            // spec, so the script data buffer should be cleared as well.
            ScriptDataBuffer_.Reset();
        }

        // Fills in:
        // * The original_text GumboStringPiece with the portion of the original
        // buffer that corresponds to the tag buffer.
        void CopyOverOriginalTagText(TStringPiece* original_text) {
            original_text->Data = TagState_.OriginalText;
            original_text->Length = Input_.GetCharPointer() - TagState_.OriginalText;
            if (original_text->Data[original_text->Length - 1] == '\r') {
                // Since \r is skipped by the UTF-8 iterator, it can sometimes end up
                // appended to the end of original text even when it's really the first part
                // of the next character.  If we detect this situation, shrink the length of
                // the original text by 1 to remove the carriage return.

                //--original_text->length;
            }
        }

        void DocTypeStateInit() {
            DoctypeState_.ForceQuirks = false;
            DoctypeState_.HasPublicIdentifier = false;
            DoctypeState_.HasSystemIdentifier = false;
        }

        // Writes a single specified character to the output token.
        void EmitChar(int c, TToken* output) {
            output->Type = GetCharTokenType(c);
            output->v.Character = c;
            FinishToken(output);
        }

        // Emit continuous text block.
        // With isPlainText flag set to true, read text till the EOF.
        // (This is the special behaviour of html5 standart).
        EStateResult EmitText(TToken* output, bool isPlainText = false) {
            int c = Input_.Current();

            if (GetCharTokenType(c) != TOKEN_CHARACTER) {
                return EmitCurrentChar(output);
            }

            output->Type = TOKEN_CHARACTER;
            while (c != EOF_CHAR && c != '\0' && (isPlainText || c != '<')) {
                Input_.Next();
                c = Input_.Current();
            }

            ReconsumeCurrentInput_ = true;
            FinishToken(output);

            return RETURN_SUCCESS;
        }

        // Wraps the consume_char_ref function to handle its output and make the
        // appropriate TokenizerState modifications.  Returns RETURN_ERROR if a parse
        // error occurred, RETURN_SUCCESS otherwise.
        EStateResult EmitCharRef(int /*additional_allowed_char*/, bool /*is_in_attribute*/, TToken* output) {
            EmitChar('&', output);
            return RETURN_SUCCESS;

            /*
        if (!parser->_options->decode_entities) {
            emit_char(parser, '&', output);
            return RETURN_SUCCESS;
        }
        GumboTokenizerState* tokenizer = parser->_tokenizer_state;
        OneOrTwoCodepoints char_ref;
        bool status = consume_char_ref(
          parser, &tokenizer->_input, additional_allowed_char, false, &char_ref);
        if (char_ref.first != kGumboNoChar) {
        // consume_char_ref ends with the iterator pointing at the next character,
        // so we need to be sure not advance it again before reading the next token.
        tokenizer->_reconsume_current_input = true;
        emit_char(parser, char_ref.first, output);
        tokenizer->_buffered_emit_char = char_ref.second;
        } else {
        emit_char(parser, '&', output);
        }
        return status ? RETURN_SUCCESS : RETURN_ERROR;
        */
        }

        // Emits a comment token.  Comments use the temporary buffer to accumulate their
        // data, and then it's copied over and released to the 'text' field of the
        // GumboToken union.  Always returns RETURN_SUCCESS.
        EStateResult EmitComment(TToken* output) {
            FinishTemporaryBuffer(&Comment_);

            output->Type = TOKEN_COMMENT;
            output->v.Text.Data = Comment_.data();
            output->v.Text.Length = Comment_.size();
            FinishToken(output);
            return RETURN_SUCCESS;
        }

        // Writes the current input character out as a character token.
        // Always returns RETURN_SUCCESS.
        EStateResult EmitCurrentChar(TToken* output) {
            EmitChar(Input_.Current(), output);
            return RETURN_SUCCESS;
        }

        // Writes out the current <noindex> start or end tag as a comment token.
        // This is Yandex specific behavior.
        // Always returns RETURN_SUCCESS.
        EStateResult EmitNoindex(TToken* output) {
            assert(Options_.NoindexToComment && TagState_.Tag == TAG_NOINDEX);
            if (TagState_.IsStartTag) {
                output->Type = TOKEN_COMMENT;
                output->OriginalText.Data = NOINDEX_START_TEXT;
                output->OriginalText.Length = NOINDEX_START_TEXT_LEN;
                output->v.Text.Data = NOINDEX_START_TEXT + 4;
                output->v.Text.Length = NOINDEX_START_TEXT_LEN - 7;
            } else {
                output->Type = TOKEN_COMMENT;
                output->OriginalText.Data = NOINDEX_END_TEXT;
                output->OriginalText.Length = NOINDEX_END_TEXT_LEN;
                output->v.Text.Data = NOINDEX_END_TEXT + 4;
                output->v.Text.Length = NOINDEX_END_TEXT_LEN - 7;
            }
            TagState_.Buffer.Reset();
            // Do not call FinishTag and set pointers manually
            if (!ReconsumeCurrentInput_) {
                Input_.Next();
            }
            ResetTokenStartPoint();
            return RETURN_SUCCESS;
        }

        // Writes out the current tag as a start or end tag token.
        // Always returns RETURN_SUCCESS.
        EStateResult EmitCurrentTag(TToken* output) {
            if (Options_.NoindexToComment && TagState_.Tag == TAG_NOINDEX) {
                return EmitNoindex(output);
            }
            if (TagState_.IsStartTag) {
                output->Type = TOKEN_START_TAG;
                output->v.StartTag.Tag = TagState_.Tag;
                if (TagState_.Attributes.empty()) {
                    output->v.StartTag.Attributes.Data = nullptr;
                    output->v.StartTag.Attributes.Length = 0;
                } else {
                    output->v.StartTag.Attributes.Data = &TagState_.Attributes[0];
                    output->v.StartTag.Attributes.Length = TagState_.Attributes.size();
                }
                output->v.StartTag.IsSelfClosing = TagState_.IsSelfClosing;
                TagState_.LastStartTag = TagState_.Tag;
            } else {
                output->Type = TOKEN_END_TAG;
                output->v.EndTag = TagState_.Tag;

                // In end tags, ownership of the attributes vector is not transferred to the
                // token, but it's still initialized as normal, so it must be manually
                // deallocated.  There may also be attributes to destroy, in certain broken
                // cases like </div</th> (the "th" is an attribute there).

                if (!TagState_.Attributes.empty()) {
                    // An end tag token is emitted with attributes - it is a parse error.
                    AddParseError(ERR_CLOSE_TAG_WITH_ATTRS);
                    TagState_.Attributes.clear();
                }
            }
            TagState_.Buffer.Reset();
            FinishToken(output);
            assert(output->OriginalText.Length >= 2);
            assert(output->OriginalText.Data[0] == '<');
            assert(output->OriginalText.Data[output->OriginalText.Length - 1] == '>');
            return RETURN_SUCCESS;
        }

        // Writes out a doctype token, copying it from the tokenizer state.
        void EmitDoctype(TToken* output) {
            output->Type = TOKEN_DOCTYPE;
            output->v.DocType.ForceQuirks = DoctypeState_.ForceQuirks;
            output->v.DocType.Name.Data = DoctypeState_.Name.data();
            output->v.DocType.Name.Length = DoctypeState_.Name.size();
            if (DoctypeState_.HasPublicIdentifier) {
                output->v.DocType.PublicIdentifier.Data = DoctypeState_.PublicIdentifier.data();
                output->v.DocType.PublicIdentifier.Length = DoctypeState_.PublicIdentifier.size();
            } else {
                output->v.DocType.PublicIdentifier = TStringPiece::Empty();
            }
            if (DoctypeState_.HasSystemIdentifier) {
                output->v.DocType.SystemIdentifier.Data = DoctypeState_.SystemIdentifier.data();
                output->v.DocType.SystemIdentifier.Length = DoctypeState_.SystemIdentifier.size();
            } else {
                output->v.DocType.SystemIdentifier = TStringPiece::Empty();
            }
            FinishToken(output);
            DocTypeStateInit();
        }

        // Writes an EOF character token.  Always returns RETURN_SUCCESS.
        EStateResult EmitEOF(TToken* output) {
            EmitChar(EOF_CHAR, output);
            return RETURN_SUCCESS;
        }

        // Writes a replacement character token and records a parse error.
        // Always returns RETURN_ERROR, per gumbo_lex return value.
        EStateResult EmitReplacementChar(TToken* output) {
            // In all cases, this is because of a null byte in the input stream.
            AddParseError(ERR_UTF8_NULL);
            EmitChar(Input_.ReplacementChar(), output);
            return RETURN_ERROR;
        }

        // Sets up the tokenizer to begin flushing the temporary buffer.
        // This resets the input iterator stream to the start of the last tag, sets up
        // _temporary_buffer_emit, and then (if the temporary buffer is non-empty) emits
        // the first character in it.  It returns true if a character was emitted, false
        // otherwise.
        EStateResult EmitTemporaryBuffer(TToken* output) {
            assert(TemporaryBuffer_.Data());
            Input_.Reset();
            TemporaryBufferEmit_ = TemporaryBuffer_.Data();
            return MaybeEmitFromTemporaryBuffer(output);
        }

        // Creates a new attribute in the current tag, copying the current tag buffer to
        // the attribute's name.  The attribute's value starts out as the empty string
        // (following the "Boolean attributes" section of the spec) and is only
        // overwritten on finish_attribute_value().  If the attribute has already been
        // specified, the new attribute is dropped, a parse error is added, and the
        // function returns false.  Otherwise, this returns true.
        bool FinishAttributeName() {
            TVector<TAttribute>& attributes = TagState_.Attributes;

            // Check for bogus documents with too many attributes in some tag.
            if (attributes.size() >= static_cast<size_t>(Options_.MaxAttributes)) {
                TagState_.DropNextAttrValue = true;
                return false;
            }

            // May've been set by a previous attribute without a value; reset it here.
            TagState_.DropNextAttrValue = false;

            for (size_t i = 0; i < attributes.size(); ++i) {
                const TAttribute& attr = attributes[i];

                if (attr.OriginalName.Length != TagState_.Buffer.Length()) {
                    continue;
                }

                if (strnicmp(attr.OriginalName.Data, TagState_.Buffer.Data(), TagState_.Buffer.Length()) == 0) {
                    // Identical attribute; bail.
                    AddDuplicateAttrError(attr.OriginalName, i, attributes.size());
                    TagState_.DropNextAttrValue = true;
                    return false;
                }
            }

            TAttribute attr;
            attr.AttrNamespace = ATTR_NAMESPACE_NONE;
            CopyOverOriginalTagText(&attr.OriginalName);
            CopyOverOriginalTagText(&attr.OriginalValue);
            attributes.push_back(attr);
            InitializeTagBuffer();
            return true;
        }

        // Finishes an attribute value.  This sets the value of the most recently added
        // attribute to the current contents of the tag buffer.
        void FinishAttributeValue() {
            assert(!TagState_.Attributes.empty());

            if (TagState_.DropNextAttrValue) {
                // Duplicate attribute name detected in an earlier state, so we have to
                // ignore the value.
                TagState_.DropNextAttrValue = false;
                InitializeTagBuffer();
                return;
            }

            TAttribute* attr = &TagState_.Attributes.back();
            CopyOverOriginalTagText(&attr->OriginalValue);
            InitializeTagBuffer();
        }

        // Records the doctype public ID, assumed to be in the temporary buffer.
        // Convenience method that also sets has_public_identifier to true.
        void FinishDoctypePublicId() {
            DoctypeState_.HasPublicIdentifier = true;
            FinishTemporaryBuffer(&DoctypeState_.PublicIdentifier);
        }

        // Records the doctype system ID, assumed to be in the temporary buffer.
        // Convenience method that also sets has_system_identifier to true.
        void FinishDoctypeSystemId() {
            DoctypeState_.HasSystemIdentifier = true;
            FinishTemporaryBuffer(&DoctypeState_.SystemIdentifier);
        }

        // Moves some data from the temporary buffer over the the tag-based fields in
        // TagState.
        void FinishTagName() {
            TagState_.Tag = GetTagEnum(TagState_.Buffer.Data(), TagState_.Buffer.Length());
            InitializeTagBuffer();
        }

        // Moves the temporary buffer contents over to the specified output string,
        // and clears the temporary buffer.
        void FinishTemporaryBuffer(TString* output) {
            output->assign(TemporaryBuffer_.Data(), TemporaryBuffer_.Length());
            ClearTemporaryBuffer();
        }

        // Advances the iterator past the end of the token, and then fills in the
        // relevant position fields.  It's assumed that after every emit, the tokenizer
        // will immediately return (letting the tree-construction stage read the filled
        // in Token).  Thus, it's safe to advance the input stream here, since it will
        // bypass the advance at the bottom of the state machine loop.
        //
        // Since this advances the iterator and resets the current input, make sure to
        // call it after you've recorded any other data you need for the token.
        void FinishToken(TToken* token) {
            if (!ReconsumeCurrentInput_) {
                Input_.Next();
            }

            token->OriginalText.Data = TokenStart_;
            ResetTokenStartPoint();
            token->OriginalText.Length = TokenStart_ - token->OriginalText.Data;
            if (token->OriginalText.Length > 0 && token->OriginalText.Data[token->OriginalText.Length - 1] == '\r') {
                // The UTF8 iterator will ignore carriage returns in the input stream, which
                // means that the next token may start one past a \r character.  The pointer
                // arithmetic above results in that \r being appended to the original text
                // of the preceding token, so we have to adjust its length here to chop the
                // \r off.
                //--token->original_text.length;
            }
        }

        // (Re-)initialize the tag buffer.  This also resets the original_text pointer
        // and _start_pos field to point to the current position.
        void InitializeTagBuffer() {
            TagState_.Buffer.Reset();
            TagState_.OriginalText = Input_.GetCharPointer();
        }

        // Returns true if the current end tag matches the last start tag emitted.
        bool IsAppropriateEndTag() const {
            assert(!TagState_.IsStartTag);
            return TagState_.LastStartTag != TAG_LAST &&
                   TagState_.LastStartTag == GetTagEnum(TagState_.Buffer.Data(), TagState_.Buffer.Length());
        }

        // Checks to see we should be flushing accumulated characters in the temporary
        // buffer, and fills the output token with the next output character if so.
        // Returns true if a character has been emitted and the tokenizer should
        // immediately return, false if we're at the end of the temporary buffer and
        // should resume normal operation.
        EStateResult MaybeEmitFromTemporaryBuffer(TToken* output) {
            const char* c = TemporaryBufferEmit_;

            if (!c || c >= TemporaryBuffer_.Data() + TemporaryBuffer_.Length()) {
                TemporaryBufferEmit_ = nullptr;
                return RETURN_ERROR;
            }

            // emit_char also advances the input stream.  We need to do some juggling of
            // the _reconsume_current_input flag to get the proper behavior when emitting
            // previous tokens.  Basically, _reconsume_current_input should *never* be set
            // when emitting anything from the temporary buffer, since those characters
            // have already been advanced past.  However, it should be preserved so that
            // when the *next* character is encountered again, the tokenizer knows not to
            // advance past it.
            const bool savedReconsumeState = ReconsumeCurrentInput_;
            ReconsumeCurrentInput_ = false;
            EmitChar(*c, output);
            ++TemporaryBufferEmit_;
            ReconsumeCurrentInput_ = savedReconsumeState;
            return RETURN_SUCCESS;
        }

        // Sets the tag buffer original text and start point to the current iterator
        // position.  This is necessary because attribute names & values may have
        // whitespace preceeding them, and so we can't assume that the actual token
        // starting point was the end of the last tag buffer usage.
        inline void ResetTagBufferStartPoint() {
            TagState_.OriginalText = Input_.GetCharPointer();
        }

        // Sets the token original_text and position to the current iterator position.
        // This is necessary because [CDATA[ sections may include text that is ignored
        // by the tokenizer.
        inline void ResetTokenStartPoint() {
            TokenStart_ = Input_.GetCharPointer();
        }

        inline void ResetTokenStartPointToNext() {
            TokenStart_ = Input_.GetCharPointer() + 1;
        }

        // Initializes the tag_state to start a new tag, keeping track of the opening
        // positions and original text.  Takes a boolean indicating whether this is a
        // start or end tag.
        void StartNewTag(int c, bool is_start_tag) {
            assert(IsAlpha(c));
            c = EnsureLowercase(c);
            assert(IsAlpha(c));

            InitializeTagBuffer();
            TagState_.Buffer.AppendChar((char)c);

            TagState_.Attributes.clear();
            TagState_.DropNextAttrValue = false;
            TagState_.IsStartTag = is_start_tag;
            TagState_.IsSelfClosing = false;
        }

    private:
        EStateResult HandleDataState(int c, TToken* output) {
            switch (c) {
                case '&':
                    SetState(LEX_CHAR_REF_IN_DATA);
                    // The char_ref machinery expects to be on the & so it can mark that
                    // and return to it if the text isn't a char ref, so we need to
                    // reconsume it.
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                case '<':
                    SetState(LEX_TAG_OPEN);
                    ClearTemporaryBuffer();
                    AppendCharToTemporaryBuffer('<');
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    EmitChar(c, output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    return EmitEOF(output);
                default:
                    return EmitText(output);
            }
        }

        EStateResult HandleCharRefInDataState(int, TToken* output) {
            SetState(LEX_DATA);
            return EmitCharRef(' ', false, output);
        }

        EStateResult HandleRcdataState(int c, TToken* output) {
            switch (c) {
                case '&':
                    SetState(LEX_CHAR_REF_IN_RCDATA);
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                case '<':
                    SetState(LEX_RCDATA_LT);
                    ClearTemporaryBuffer();
                    AppendCharToTemporaryBuffer('<');
                    return NEXT_CHAR;
                case '\0':
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    return EmitEOF(output);
                default:
                    return EmitText(output);
            }
        }

        EStateResult HandleCharRefInRcdataState(int, TToken* output) {
            SetState(LEX_RCDATA);
            return EmitCharRef(' ', false, output);
        }

        EStateResult HandleRawtextState(int c, TToken* output) {
            switch (c) {
                case '<':
                    SetState(LEX_RAWTEXT_LT);
                    ClearTemporaryBuffer();
                    AppendCharToTemporaryBuffer('<');
                    return NEXT_CHAR;
                case '\0':
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    return EmitEOF(output);
                default:
                    return EmitText(output);
            }
        }

        EStateResult HandleScriptState(int c, TToken* output) {
            switch (c) {
                case '<':
                    SetState(LEX_SCRIPT_LT);
                    ClearTemporaryBuffer();
                    AppendCharToTemporaryBuffer('<');
                    return NEXT_CHAR;
                case '\0':
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    return EmitEOF(output);
                default:
                    return EmitText(output);
            }
        }

        EStateResult HandlePlaintextState(int c, TToken* output) {
            switch (c) {
                case '\0':
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    return EmitEOF(output);
                default:
                    return EmitText(output, true); //< Special behaviour for plaintext
            }
        }

        EStateResult HandleTagOpenState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "<"));
            switch (c) {
                case '!':
                    SetState(LEX_MARKUP_DECLARATION);
                    ClearTemporaryBuffer();
                    return NEXT_CHAR;
                case '/':
                    SetState(LEX_END_TAG_OPEN);
                    AppendCharToTemporaryBuffer('/');
                    return NEXT_CHAR;
                case '?':
                    SetState(LEX_BOGUS_COMMENT);
                    ClearTemporaryBuffer();
                    AppendCharToTemporaryBuffer('?');
                    AddParseError(ERR_TAG_STARTS_WITH_QUESTION);
                    return NEXT_CHAR;
                default:
                    if (IsAlpha(c)) {
                        SetState(LEX_TAG_NAME);
                        StartNewTag(c, true);
                        return NEXT_CHAR;
                    } else {
                        AddParseError(ERR_TAG_INVALID);
                        SetState(LEX_DATA);
                        EmitTemporaryBuffer(output);
                        return RETURN_ERROR;
                    }
            }
        }

        EStateResult HandleEndTagOpenState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "</"));
            switch (c) {
                case '>':
                    AddParseError(ERR_CLOSE_TAG_EMPTY);
                    ResetTokenStartPointToNext();
                    SetState(LEX_DATA);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_CLOSE_TAG_EOF);
                    SetState(LEX_DATA);
                    return EmitTemporaryBuffer(output);
                default:
                    if (IsAlpha(c)) {
                        SetState(LEX_TAG_NAME);
                        StartNewTag(c, false);
                    } else {
                        AddParseError(ERR_CLOSE_TAG_INVALID);
                        SetState(LEX_BOGUS_COMMENT);
                        ClearTemporaryBuffer();
                        AppendCharToTemporaryBuffer(c);
                    }
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleTagNameState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    FinishTagName();
                    SetState(LEX_BEFORE_ATTR_NAME);
                    return NEXT_CHAR;
                case '/':
                    FinishTagName();
                    SetState(LEX_SELF_CLOSING_START_TAG);
                    return NEXT_CHAR;
                case '>':
                    FinishTagName();
                    SetState(LEX_DATA);
                    return EmitCurrentTag(output);
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTagBuffer(Input_.ReplacementChar(), true);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_TAG_EOF);
                    AbandonCurrentTag();
                    SetState(LEX_DATA);
                    return NEXT_CHAR;
                default:
                    AppendCharToTagBuffer(EnsureLowercase(c), true);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleRcdataLtState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "<"));
            if (c == '/') {
                SetState(LEX_RCDATA_END_TAG_OPEN);
                AppendCharToTemporaryBuffer('/');
                return NEXT_CHAR;
            } else {
                SetState(LEX_RCDATA);
                ReconsumeCurrentInput_ = true;
                return EmitTemporaryBuffer(output);
            }
        }

        EStateResult HandleRcdataEndTagOpenState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "</"));
            if (IsAlpha(c)) {
                SetState(LEX_RCDATA_END_TAG_NAME);
                StartNewTag(c, false);
                AppendCharToTemporaryBuffer(EnsureLowercase(c));
                return NEXT_CHAR;
            } else {
                SetState(LEX_RCDATA);
                return EmitTemporaryBuffer(output);
            }
        }

        EStateResult HandleRcdataEndTagNameState(int c, TToken* output) {
            assert(TemporaryBuffer_.Length() >= 2);
            if (IsAlpha(c)) {
                AppendCharToTagBuffer(EnsureLowercase(c), true);
                AppendCharToTemporaryBuffer(c);
                return NEXT_CHAR;
            } else if (IsAppropriateEndTag()) {
                switch (c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case '\r':
                    case ' ':
                        FinishTagName();
                        SetState(LEX_BEFORE_ATTR_NAME);
                        return NEXT_CHAR;
                    case '/':
                        FinishTagName();
                        SetState(LEX_SELF_CLOSING_START_TAG);
                        return NEXT_CHAR;
                    case '>':
                        FinishTagName();
                        SetState(LEX_DATA);
                        return EmitCurrentTag(output);
                }
            }
            SetState(LEX_RCDATA);
            AbandonCurrentTag();
            return EmitTemporaryBuffer(output);
        }

        EStateResult HandleRawtextLtState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "<"));
            if (c == '/') {
                SetState(LEX_RAWTEXT_END_TAG_OPEN);
                AppendCharToTemporaryBuffer('/');
                return NEXT_CHAR;
            } else {
                SetState(LEX_RAWTEXT);
                ReconsumeCurrentInput_ = true;
                return EmitTemporaryBuffer(output);
            }
        }

        EStateResult HandleRawtextEndTagOpenState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "</"));
            if (IsAlpha(c)) {
                SetState(LEX_RAWTEXT_END_TAG_NAME);
                StartNewTag(c, false);
                AppendCharToTemporaryBuffer(c);
                return NEXT_CHAR;
            } else {
                SetState(LEX_RAWTEXT);
                return EmitTemporaryBuffer(output);
            }
        }

        EStateResult HandleRawtextEndTagNameState(int c, TToken* output) {
            assert(TemporaryBuffer_.Length() >= 2);
            if (IsAlpha(c)) {
                AppendCharToTagBuffer(EnsureLowercase(c), true);
                AppendCharToTemporaryBuffer(c);
                return NEXT_CHAR;
            } else if (IsAppropriateEndTag()) {
                switch (c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case '\r':
                    case ' ':
                        FinishTagName();
                        SetState(LEX_BEFORE_ATTR_NAME);
                        return NEXT_CHAR;
                    case '/':
                        FinishTagName();
                        SetState(LEX_SELF_CLOSING_START_TAG);
                        return NEXT_CHAR;
                    case '>':
                        FinishTagName();
                        SetState(LEX_DATA);
                        return EmitCurrentTag(output);
                }
            }
            SetState(LEX_RAWTEXT);
            AbandonCurrentTag();
            return EmitTemporaryBuffer(output);
        }

        EStateResult HandleScriptLtState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "<"));
            if (c == '/') {
                SetState(LEX_SCRIPT_END_TAG_OPEN);
                AppendCharToTemporaryBuffer('/');
                return NEXT_CHAR;
            } else if (c == '!') {
                SetState(LEX_SCRIPT_ESCAPED_START);
                AppendCharToTemporaryBuffer('!');
                return EmitTemporaryBuffer(output);
            } else {
                SetState(LEX_SCRIPT);
                ReconsumeCurrentInput_ = true;
                return EmitTemporaryBuffer(output);
            }
        }

        EStateResult HandleScriptEndTagOpenState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "</"));
            if (IsAlpha(c)) {
                SetState(LEX_SCRIPT_END_TAG_NAME);
                StartNewTag(c, false);
                AppendCharToTemporaryBuffer(c);
                return NEXT_CHAR;
            } else {
                SetState(LEX_SCRIPT);
                return EmitTemporaryBuffer(output);
            }
        }

        EStateResult HandleScriptEndTagNameState(int c, TToken* output) {
            assert(TemporaryBuffer_.Length() >= 2);
            if (IsAlpha(c)) {
                AppendCharToTagBuffer(EnsureLowercase(c), true);
                AppendCharToTemporaryBuffer(c);
                return NEXT_CHAR;
            } else if (IsAppropriateEndTag()) {
                switch (c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case '\r':
                    case ' ':
                        FinishTagName();
                        SetState(LEX_BEFORE_ATTR_NAME);
                        return NEXT_CHAR;
                    case '/':
                        FinishTagName();
                        SetState(LEX_SELF_CLOSING_START_TAG);
                        return NEXT_CHAR;
                    case '>':
                        FinishTagName();
                        SetState(LEX_DATA);
                        return EmitCurrentTag(output);
                }
            }
            SetState(LEX_SCRIPT);
            AbandonCurrentTag();
            return EmitTemporaryBuffer(output);
        }

        EStateResult HandleScriptEscapedStartState(int c, TToken* output) {
            if (c == '-') {
                SetState(LEX_SCRIPT_ESCAPED_START_DASH);
                return EmitCurrentChar(output);
            } else {
                SetState(LEX_SCRIPT);
                ReconsumeCurrentInput_ = true;
                return NEXT_CHAR;
            }
        }

        EStateResult HandleScriptEscapedStartDashState(int c, TToken* output) {
            if (c == '-') {
                SetState(LEX_SCRIPT_ESCAPED_DASH_DASH);
                return EmitCurrentChar(output);
            } else {
                SetState(LEX_SCRIPT);
                ReconsumeCurrentInput_ = true;
                return NEXT_CHAR;
            }
        }

        EStateResult HandleScriptEscapedState(int c, TToken* output) {
            switch (c) {
                case '-':
                    SetState(LEX_SCRIPT_ESCAPED_DASH);
                    return EmitCurrentChar(output);
                case '<':
                    SetState(LEX_SCRIPT_ESCAPED_LT);
                    ClearTemporaryBuffer();
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
                case '\0':
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    AddParseError(ERR_SCRIPT_EOF);
                    return EmitEOF(output);
                default:
                    return EmitCurrentChar(output);
            }
        }

        EStateResult HandleScriptEscapedDashState(int c, TToken* output) {
            switch (c) {
                case '-':
                    SetState(LEX_SCRIPT_ESCAPED_DASH_DASH);
                    return EmitCurrentChar(output);
                case '<':
                    SetState(LEX_SCRIPT_ESCAPED_LT);
                    ClearTemporaryBuffer();
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
                case '\0':
                    SetState(LEX_SCRIPT_ESCAPED);
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    AddParseError(ERR_SCRIPT_EOF);
                    SetState(LEX_DATA);
                    return NEXT_CHAR;
                default:
                    SetState(LEX_SCRIPT_ESCAPED);
                    return EmitCurrentChar(output);
            }
        }

        EStateResult HandleScriptEscapedDashDashState(int c, TToken* output) {
            switch (c) {
                case '-':
                    return EmitCurrentChar(output);
                case '<':
                    SetState(LEX_SCRIPT_ESCAPED_LT);
                    ClearTemporaryBuffer();
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_SCRIPT);
                    return EmitCurrentChar(output);
                case '\0':
                    SetState(LEX_SCRIPT_ESCAPED);
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    AddParseError(ERR_SCRIPT_EOF);
                    SetState(LEX_DATA);
                    return NEXT_CHAR;
                default:
                    SetState(LEX_SCRIPT_ESCAPED);
                    return EmitCurrentChar(output);
            }
        }

        EStateResult HandleScriptEscapedLtState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "<"));
            assert(!ScriptDataBuffer_.Length());
            if (c == '/') {
                SetState(LEX_SCRIPT_ESCAPED_END_TAG_OPEN);
                AppendCharToTemporaryBuffer(c);
                return NEXT_CHAR;
            } else if (IsAlpha(c)) {
                SetState(LEX_SCRIPT_DOUBLE_ESCAPED_START);
                AppendCharToTemporaryBuffer(c);
                ScriptDataBuffer_.AppendChar(EnsureLowercase(c));
                return EmitTemporaryBuffer(output);
            } else {
                SetState(LEX_SCRIPT_ESCAPED);
                return EmitTemporaryBuffer(output);
            }
        }

        EStateResult HandleScriptEscapedEndTagOpenState(int c, TToken* output) {
            assert(CheckBufferEquals(TemporaryBuffer_, "</"));
            if (IsAlpha(c)) {
                SetState(LEX_SCRIPT_ESCAPED_END_TAG_NAME);
                StartNewTag(c, false);
                AppendCharToTemporaryBuffer(c);
                return NEXT_CHAR;
            } else {
                SetState(LEX_SCRIPT_ESCAPED);
                return EmitTemporaryBuffer(output);
            }
        }

        EStateResult HandleScriptEscapedEndTagNameState(int c, TToken* output) {
            assert(TemporaryBuffer_.Length() >= 2);
            if (IsAlpha(c)) {
                AppendCharToTagBuffer(EnsureLowercase(c), true);
                AppendCharToTemporaryBuffer(c);
                return NEXT_CHAR;
            } else if (IsAppropriateEndTag()) {
                switch (c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case '\r':
                    case ' ':
                        FinishTagName();
                        SetState(LEX_BEFORE_ATTR_NAME);
                        return NEXT_CHAR;
                    case '/':
                        FinishTagName();
                        SetState(LEX_SELF_CLOSING_START_TAG);
                        return NEXT_CHAR;
                    case '>':
                        FinishTagName();
                        SetState(LEX_DATA);
                        return EmitCurrentTag(output);
                }
            }
            SetState(LEX_SCRIPT_ESCAPED);
            AbandonCurrentTag();
            return EmitTemporaryBuffer(output);
        }

        EStateResult HandleScriptDoubleEscapedStartState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                case '/':
                case '>':
                    if (ScriptDataBuffer_.Length() == 6 && strncmp(ScriptDataBuffer_.Data(), "script", 6) == 0) {
                        SetState(LEX_SCRIPT_DOUBLE_ESCAPED);
                    } else {
                        SetState(LEX_SCRIPT_ESCAPED);
                    }
                    return EmitCurrentChar(output);
                default:
                    if (IsAlpha(c)) {
                        ScriptDataBuffer_.AppendChar(EnsureLowercase(c));
                        return EmitCurrentChar(output);
                    } else {
                        SetState(LEX_SCRIPT_ESCAPED);
                        ReconsumeCurrentInput_ = true;
                        return NEXT_CHAR;
                    }
            }
        }

        EStateResult HandleScriptDoubleEscapedState(int c, TToken* output) {
            switch (c) {
                case '-':
                    SetState(LEX_SCRIPT_DOUBLE_ESCAPED_DASH);
                    return EmitCurrentChar(output);
                case '<':
                    SetState(LEX_SCRIPT_DOUBLE_ESCAPED_LT);
                    return EmitCurrentChar(output);
                case '\0':
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    AddParseError(ERR_SCRIPT_EOF);
                    SetState(LEX_DATA);
                    return NEXT_CHAR;
                default:
                    return EmitCurrentChar(output);
            }
        }

        EStateResult HandleScriptDoubleEscapedDashState(int c, TToken* output) {
            switch (c) {
                case '-':
                    SetState(LEX_SCRIPT_DOUBLE_ESCAPED_DASH_DASH);
                    return EmitCurrentChar(output);
                case '<':
                    SetState(LEX_SCRIPT_DOUBLE_ESCAPED_LT);
                    return EmitCurrentChar(output);
                case '\0':
                    SetState(LEX_SCRIPT_DOUBLE_ESCAPED);
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    AddParseError(ERR_SCRIPT_EOF);
                    SetState(LEX_DATA);
                    return NEXT_CHAR;
                default:
                    SetState(LEX_SCRIPT_DOUBLE_ESCAPED);
                    return EmitCurrentChar(output);
            }
        }

        EStateResult HandleScriptDoubleEscapedDashDashState(int c, TToken* output) {
            switch (c) {
                case '-':
                    return EmitCurrentChar(output);
                case '<':
                    SetState(LEX_SCRIPT_DOUBLE_ESCAPED_LT);
                    return EmitCurrentChar(output);
                case '>':
                    SetState(LEX_SCRIPT);
                    return EmitCurrentChar(output);
                case '\0':
                    SetState(LEX_SCRIPT_DOUBLE_ESCAPED);
                    return EmitReplacementChar(output);
                case EOF_CHAR:
                    AddParseError(ERR_SCRIPT_EOF);
                    SetState(LEX_DATA);
                    return NEXT_CHAR;
                default:
                    SetState(LEX_SCRIPT_DOUBLE_ESCAPED);
                    return EmitCurrentChar(output);
            }
        }

        EStateResult HandleScriptDoubleEscapedLtState(int c, TToken* output) {
            if (c == '/') {
                SetState(LEX_SCRIPT_DOUBLE_ESCAPED_END);
                ScriptDataBuffer_.Reset();
                return EmitCurrentChar(output);
            } else {
                SetState(LEX_SCRIPT_DOUBLE_ESCAPED);
                ReconsumeCurrentInput_ = true;
                return NEXT_CHAR;
            }
        }

        EStateResult HandleScriptDoubleEscapedEndState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                case '/':
                case '>':
                    if (ScriptDataBuffer_.Length() == 6 && strncmp(ScriptDataBuffer_.Data(), "script", 6) == 0) {
                        SetState(LEX_SCRIPT_ESCAPED);
                    } else {
                        SetState(LEX_SCRIPT_DOUBLE_ESCAPED);
                    }
                    return EmitCurrentChar(output);
                default:
                    if (IsAlpha(c)) {
                        ScriptDataBuffer_.AppendChar(EnsureLowercase(c));
                        return EmitCurrentChar(output);
                    } else {
                        SetState(LEX_SCRIPT_DOUBLE_ESCAPED);
                        ReconsumeCurrentInput_ = true;
                        return NEXT_CHAR;
                    }
            }
        }

        EStateResult HandleBeforeAttrNameState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    return NEXT_CHAR;
                case '/':
                    SetState(LEX_SELF_CLOSING_START_TAG);
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    return EmitCurrentTag(output);
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    SetState(LEX_ATTR_NAME);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_ATTR_NAME_EOF);
                    SetState(LEX_DATA);
                    AbandonCurrentTag();
                    return NEXT_CHAR;
                case '"':
                case '\'':
                case '<':
                case '=':
                    AddParseError(ERR_ATTR_NAME_INVALID);
                    [[fallthrough]];
                default:
                    SetState(LEX_ATTR_NAME);
                    AppendCharToTagBuffer(EnsureLowercase(c), true);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleAttrNameState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    FinishAttributeName();
                    SetState(LEX_AFTER_ATTR_NAME);
                    return NEXT_CHAR;
                case '/':
                    FinishAttributeName();
                    SetState(LEX_SELF_CLOSING_START_TAG);
                    return NEXT_CHAR;
                case '=':
                    FinishAttributeName();
                    SetState(LEX_BEFORE_ATTR_VALUE);
                    return NEXT_CHAR;
                case '>':
                    FinishAttributeName();
                    SetState(LEX_DATA);
                    return EmitCurrentTag(output);
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTagBuffer(Input_.ReplacementChar(), true);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    SetState(LEX_DATA);
                    AbandonCurrentTag();
                    AddParseError(ERR_ATTR_NAME_EOF);
                    return NEXT_CHAR;
                case '"':
                case '\'':
                case '<':
                    AddParseError(ERR_ATTR_NAME_INVALID);
                    [[fallthrough]];
                default:
                    AppendCharToTagBuffer(EnsureLowercase(c), true);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleAfterAttrNameState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    return NEXT_CHAR;
                case '/':
                    SetState(LEX_SELF_CLOSING_START_TAG);
                    return NEXT_CHAR;
                case '=':
                    SetState(LEX_BEFORE_ATTR_VALUE);
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    return EmitCurrentTag(output);
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    SetState(LEX_ATTR_NAME);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_ATTR_NAME_EOF);
                    SetState(LEX_DATA);
                    AbandonCurrentTag();
                    return NEXT_CHAR;
                case '"':
                case '\'':
                case '<':
                    AddParseError(ERR_ATTR_NAME_INVALID);
                    [[fallthrough]];
                default:
                    SetState(LEX_ATTR_NAME);
                    AppendCharToTagBuffer(EnsureLowercase(c), true);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleBeforeAttrValueState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    return NEXT_CHAR;
                case '"':
                    SetState(LEX_ATTR_VALUE_DOUBLE_QUOTED);
                    ResetTagBufferStartPoint();
                    return NEXT_CHAR;
                case '&':
                    SetState(LEX_ATTR_VALUE_UNQUOTED);
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                case '\'':
                    SetState(LEX_ATTR_VALUE_SINGLE_QUOTED);
                    ResetTagBufferStartPoint();
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    SetState(LEX_ATTR_VALUE_UNQUOTED);
                    AppendCharToTagBuffer(Input_.ReplacementChar(), true);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_ATTR_UNQUOTED_EOF);
                    SetState(LEX_DATA);
                    AbandonCurrentTag();
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_ATTR_UNQUOTED_RIGHT_BRACKET);
                    SetState(LEX_DATA);
                    EmitCurrentTag(output);
                    return RETURN_ERROR;
                case '<':
                case '=':
                case '`':
                    AddParseError(ERR_ATTR_UNQUOTED_EQUALS);
                    [[fallthrough]];
                default:
                    SetState(LEX_ATTR_VALUE_UNQUOTED);
                    AppendCharToTagBuffer(c, true);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleAttrValueDoubleQuotedState(int c, TToken* /*output*/) {
            switch (c) {
                case '"':
                    SetState(LEX_AFTER_ATTR_VALUE_QUOTED);
                    return NEXT_CHAR;
                case '&':
                    TagState_.AttrValueState = State_;
                    SetState(LEX_CHAR_REF_IN_ATTR_VALUE);
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTagBuffer(Input_.ReplacementChar(), false);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_ATTR_DOUBLE_QUOTE_EOF);
                    SetState(LEX_DATA);
                    AbandonCurrentTag();
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                default:
                    AppendCharToTagBuffer(c, false);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleAttrValueSingleQuotedState(int c, TToken* /*output*/) {
            switch (c) {
                case '\'':
                    SetState(LEX_AFTER_ATTR_VALUE_QUOTED);
                    return NEXT_CHAR;
                case '&':
                    TagState_.AttrValueState = State_;
                    SetState(LEX_CHAR_REF_IN_ATTR_VALUE);
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTagBuffer(Input_.ReplacementChar(), false);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_ATTR_SINGLE_QUOTE_EOF);
                    SetState(LEX_DATA);
                    AbandonCurrentTag();
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                default:
                    AppendCharToTagBuffer(c, false);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleAttrValueUnquotedState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    SetState(LEX_BEFORE_ATTR_NAME);
                    FinishAttributeValue();
                    return NEXT_CHAR;
                case '&':
                    TagState_.AttrValueState = State_;
                    SetState(LEX_CHAR_REF_IN_ATTR_VALUE);
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    FinishAttributeValue();
                    return EmitCurrentTag(output);
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTagBuffer(Input_.ReplacementChar(), true);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_ATTR_UNQUOTED_EOF);
                    SetState(LEX_DATA);
                    ReconsumeCurrentInput_ = true;
                    AbandonCurrentTag();
                    return NEXT_CHAR;
                case '<':
                case '=':
                case '"':
                case '\'':
                case '`':
                    AddParseError(ERR_ATTR_UNQUOTED_EQUALS);
                    [[fallthrough]];
                default:
                    AppendCharToTagBuffer(c, true);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleCharRefInAttrValueState(int /*c*/, TToken* /*output*/) {
            // Currently we don't decode any html-entities at parser level.
            AppendCharToTagBuffer('&', (TagState_.AttrValueState == LEX_ATTR_VALUE_UNQUOTED));
            SetState(TagState_.AttrValueState);
            return NEXT_CHAR;
        }

        EStateResult HandleAfterAttrValueQuotedState(int c, TToken* output) {
            FinishAttributeValue();
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    SetState(LEX_BEFORE_ATTR_NAME);
                    return NEXT_CHAR;
                case '/':
                    SetState(LEX_SELF_CLOSING_START_TAG);
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    return EmitCurrentTag(output);
                case EOF_CHAR:
                    AddParseError(ERR_ATTR_AFTER_EOF);
                    SetState(LEX_DATA);
                    AbandonCurrentTag();
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
                default:
                    AddParseError(ERR_ATTR_AFTER_INVALID);
                    SetState(LEX_BEFORE_ATTR_NAME);
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleSelfClosingStartTagState(int c, TToken* output) {
            switch (c) {
                case '>':
                    SetState(LEX_DATA);
                    TagState_.IsSelfClosing = true;
                    return EmitCurrentTag(output);
                case EOF_CHAR:
                    AddParseError(ERR_SOLIDUS_EOF);
                    SetState(LEX_DATA);
                    AbandonCurrentTag();
                    return NEXT_CHAR;
                default:
                    AddParseError(ERR_SOLIDUS_INVALID);
                    SetState(LEX_BEFORE_ATTR_NAME);
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleBogusCommentState(int c, TToken* output) {
            while (c != '>' && c != -1) {
                if (c == '\0') {
                    c = Input_.ReplacementChar();
                }
                AppendCharToTemporaryBuffer(c);
                Input_.Next();
                c = Input_.Current();
            }
            SetState(LEX_DATA);
            return EmitComment(output);
        }

        EStateResult HandleMarkupDeclarationState(int /*c*/, TToken* /*output*/) {
            if (Input_.MaybeConsumeMatch("--", sizeof("--") - 1, true)) {
                SetState(LEX_COMMENT_START);
                ReconsumeCurrentInput_ = true;
            } else if (Input_.MaybeConsumeMatch("DOCTYPE", sizeof("DOCTYPE") - 1, false)) {
                SetState(LEX_DOCTYPE);
                ReconsumeCurrentInput_ = true;
                // If we get here, we know we'll eventually emit a doctype token, so now is
                // the time to initialize the doctype strings.  (Not in doctype_state_init,
                // since then they'll leak if ownership never gets transferred to the
                // doctype token.

                DoctypeState_.Name.clear();
                DoctypeState_.PublicIdentifier.clear();
                DoctypeState_.SystemIdentifier.clear();
            } else if (IsCurrentNodeForeign_ && Input_.MaybeConsumeMatch("[CDATA[", sizeof("[CDATA[") - 1, true)) {
                SetState(LEX_CDATA);
                ResetTokenStartPoint();
                ReconsumeCurrentInput_ = true;
            } else {
                AddParseError(ERR_DASHES_OR_DOCTYPE);
                SetState(LEX_BOGUS_COMMENT);
                ReconsumeCurrentInput_ = true;
                ClearTemporaryBuffer();
            }
            return NEXT_CHAR;
        }

        EStateResult HandleCommentStartState(int c, TToken* output) {
            switch (c) {
                case '-':
                    SetState(LEX_COMMENT_START_DASH);
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_COMMENT_INVALID);
                    SetState(LEX_DATA);
                    EmitComment(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_COMMENT_EOF);
                    SetState(LEX_DATA);
                    EmitComment(output);
                    return RETURN_ERROR;
                default:
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleCommentStartDashState(int c, TToken* output) {
            switch (c) {
                case '-':
                    SetState(LEX_COMMENT_END);
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_COMMENT_INVALID);
                    SetState(LEX_DATA);
                    EmitComment(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_COMMENT_EOF);
                    SetState(LEX_DATA);
                    EmitComment(output);
                    return RETURN_ERROR;
                default:
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleCommentState(int c, TToken* output) {
            switch (c) {
                case '-':
                    SetState(LEX_COMMENT_END_DASH);
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_COMMENT_EOF);
                    SetState(LEX_DATA);
                    EmitComment(output);
                    return RETURN_ERROR;
                default:
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleCommentEndDashState(int c, TToken* output) {
            switch (c) {
                case '-':
                    SetState(LEX_COMMENT_END);
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_COMMENT_EOF);
                    SetState(LEX_DATA);
                    EmitComment(output);
                    return RETURN_ERROR;
                default:
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleCommentEndState(int c, TToken* output) {
            switch (c) {
                case '>':
                    SetState(LEX_DATA);
                    return EmitComment(output);
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case '!':
                    AddParseError(ERR_COMMENT_BANG_AFTER_DOUBLE_DASH);
                    SetState(LEX_COMMENT_END_BANG);
                    return NEXT_CHAR;
                case '-':
                    AddParseError(ERR_COMMENT_DASH_AFTER_DOUBLE_DASH);
                    AppendCharToTemporaryBuffer('-');
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_COMMENT_EOF);
                    SetState(LEX_DATA);
                    EmitComment(output);
                    return RETURN_ERROR;
                default:
                    AddParseError(ERR_COMMENT_INVALID);
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleCommentEndBangState(int c, TToken* output) {
            switch (c) {
                case '-':
                    SetState(LEX_COMMENT_END_DASH);
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer('!');
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    return EmitComment(output);
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer('!');
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_COMMENT_END_BANG_EOF);
                    SetState(LEX_DATA);
                    EmitComment(output);
                    return RETURN_ERROR;
                default:
                    SetState(LEX_COMMENT);
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer('-');
                    AppendCharToTemporaryBuffer('!');
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleDoctypeState(int c, TToken* output) {
            assert(!TemporaryBuffer_.Length());
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    SetState(LEX_BEFORE_DOCTYPE_NAME);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AddParseError(ERR_DOCTYPE_SPACE);
                    SetState(LEX_BEFORE_DOCTYPE_NAME);
                    ReconsumeCurrentInput_ = true;
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleBeforeDoctypeNameState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    SetState(LEX_DOCTYPE_NAME);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_DOCTYPE_RIGHT_BRACKET);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    SetState(LEX_DOCTYPE_NAME);
                    DoctypeState_.ForceQuirks = false;
                    AppendCharToTemporaryBuffer(EnsureLowercase(c));
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleDoctypeNameState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    SetState(LEX_AFTER_DOCTYPE_NAME);
                    FinishTemporaryBuffer(&DoctypeState_.Name);
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    FinishTemporaryBuffer(&DoctypeState_.Name);
                    EmitDoctype(output);
                    return RETURN_SUCCESS;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    FinishTemporaryBuffer(&DoctypeState_.Name);
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    DoctypeState_.ForceQuirks = false;
                    AppendCharToTemporaryBuffer(EnsureLowercase(c));
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleAfterDoctypeNameState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    EmitDoctype(output);
                    return RETURN_SUCCESS;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    if (Input_.MaybeConsumeMatch("PUBLIC", sizeof("PUBLIC") - 1, false)) {
                        SetState(LEX_AFTER_DOCTYPE_PUBLIC_KEYWORD);
                        ReconsumeCurrentInput_ = true;
                    } else if (Input_.MaybeConsumeMatch("SYSTEM", sizeof("SYSTEM") - 1, false)) {
                        SetState(LEX_AFTER_DOCTYPE_SYSTEM_KEYWORD);
                        ReconsumeCurrentInput_ = true;
                    } else {
                        AddParseError(ERR_DOCTYPE_SPACE_OR_RIGHT_BRACKET);
                        SetState(LEX_BOGUS_DOCTYPE);
                        DoctypeState_.ForceQuirks = true;
                    }
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleAfterDoctypePublicKeywordState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    SetState(LEX_BEFORE_DOCTYPE_PUBLIC_ID);
                    return NEXT_CHAR;
                case '"':
                    AddParseError(ERR_DOCTYPE_INVALID);
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED);
                    return NEXT_CHAR;
                case '\'':
                    AddParseError(ERR_DOCTYPE_INVALID);
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED);
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_DOCTYPE_RIGHT_BRACKET);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AddParseError(ERR_DOCTYPE_INVALID);
                    SetState(LEX_BOGUS_DOCTYPE);
                    DoctypeState_.ForceQuirks = true;
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleBeforeDoctypePublicIdState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    return NEXT_CHAR;
                case '"':
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED);
                    return NEXT_CHAR;
                case '\'':
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED);
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_DOCTYPE_END);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AddParseError(ERR_DOCTYPE_INVALID);
                    SetState(LEX_BOGUS_DOCTYPE);
                    DoctypeState_.ForceQuirks = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
            }
        }

        EStateResult HandleDoctypePublicIdDoubleQuotedState(int c, TToken* output) {
            switch (c) {
                case '"':
                    SetState(LEX_AFTER_DOCTYPE_PUBLIC_ID);
                    FinishDoctypePublicId();
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_DOCTYPE_END);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    FinishDoctypePublicId();
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    FinishDoctypePublicId();
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleDoctypePublicIdSingleQuotedState(int c, TToken* output) {
            switch (c) {
                case '\'':
                    SetState(LEX_AFTER_DOCTYPE_PUBLIC_ID);
                    FinishDoctypePublicId();
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_DOCTYPE_END);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    FinishDoctypePublicId();
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    FinishDoctypePublicId();
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleAfterDoctypePublicIdState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    SetState(LEX_BETWEEN_DOCTYPE_PUBLIC_SYSTEM_ID);
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    EmitDoctype(output);
                    return RETURN_SUCCESS;
                case '"':
                    AddParseError(ERR_DOCTYPE_INVALID);
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_SYSTEM_ID_DOUBLE_QUOTED);
                    return NEXT_CHAR;
                case '\'':
                    AddParseError(ERR_DOCTYPE_INVALID);
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_SYSTEM_ID_SINGLE_QUOTED);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    FinishDoctypePublicId();
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AddParseError(ERR_DOCTYPE_INVALID);
                    SetState(LEX_BOGUS_DOCTYPE);
                    DoctypeState_.ForceQuirks = true;
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleBetweenDoctypePublicSystemIdState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    EmitDoctype(output);
                    return RETURN_SUCCESS;
                case '"':
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_SYSTEM_ID_DOUBLE_QUOTED);
                    return NEXT_CHAR;
                case '\'':
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_SYSTEM_ID_SINGLE_QUOTED);
                    return NEXT_CHAR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AddParseError(ERR_DOCTYPE_INVALID);
                    SetState(LEX_BOGUS_DOCTYPE);
                    DoctypeState_.ForceQuirks = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
            }
        }

        EStateResult HandleAfterDoctypeSystemKeywordState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    SetState(LEX_BEFORE_DOCTYPE_SYSTEM_ID);
                    return NEXT_CHAR;
                case '"':
                    AddParseError(ERR_DOCTYPE_INVALID);
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_SYSTEM_ID_DOUBLE_QUOTED);
                    return NEXT_CHAR;
                case '\'':
                    AddParseError(ERR_DOCTYPE_INVALID);
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_SYSTEM_ID_SINGLE_QUOTED);
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_DOCTYPE_END);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AddParseError(ERR_DOCTYPE_INVALID);
                    SetState(LEX_BOGUS_DOCTYPE);
                    DoctypeState_.ForceQuirks = true;
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleBeforeDoctypeSystemIdState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    return NEXT_CHAR;
                case '"':
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_SYSTEM_ID_DOUBLE_QUOTED);
                    return NEXT_CHAR;
                case '\'':
                    assert(CheckBufferEquals(TemporaryBuffer_, ""));
                    SetState(LEX_DOCTYPE_SYSTEM_ID_SINGLE_QUOTED);
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_DOCTYPE_END);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AddParseError(ERR_DOCTYPE_INVALID);
                    SetState(LEX_BOGUS_DOCTYPE);
                    DoctypeState_.ForceQuirks = true;
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleDoctypeSystemIdDoubleQuotedState(int c, TToken* output) {
            switch (c) {
                case '"':
                    SetState(LEX_AFTER_DOCTYPE_SYSTEM_ID);
                    FinishDoctypeSystemId();
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_DOCTYPE_END);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    FinishDoctypeSystemId();
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    FinishDoctypeSystemId();
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleDoctypeSystemIdSingleQuotedState(int c, TToken* output) {
            switch (c) {
                case '\'':
                    SetState(LEX_AFTER_DOCTYPE_SYSTEM_ID);
                    FinishDoctypeSystemId();
                    return NEXT_CHAR;
                case '\0':
                    AddParseError(ERR_UTF8_NULL);
                    AppendCharToTemporaryBuffer(Input_.ReplacementChar());
                    return NEXT_CHAR;
                case '>':
                    AddParseError(ERR_DOCTYPE_END);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    FinishDoctypeSystemId();
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_INVALID);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    FinishDoctypeSystemId();
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AppendCharToTemporaryBuffer(c);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleAfterDoctypeSystemIdState(int c, TToken* output) {
            switch (c) {
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                case ' ':
                    return NEXT_CHAR;
                case '>':
                    SetState(LEX_DATA);
                    EmitDoctype(output);
                    return RETURN_SUCCESS;
                case EOF_CHAR:
                    AddParseError(ERR_DOCTYPE_EOF);
                    SetState(LEX_DATA);
                    DoctypeState_.ForceQuirks = true;
                    ReconsumeCurrentInput_ = true;
                    EmitDoctype(output);
                    return RETURN_ERROR;
                default:
                    AddParseError(ERR_DOCTYPE_INVALID);
                    SetState(LEX_BOGUS_DOCTYPE);
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleBogusDoctypeState(int c, TToken* output) {
            switch (c) {
                case '>':
                    SetState(LEX_DATA);
                    EmitDoctype(output);
                    return RETURN_ERROR;
                case EOF_CHAR:
                    SetState(LEX_DATA);
                    EmitDoctype(output);
                    ReconsumeCurrentInput_ = true;
                    return RETURN_ERROR;
                default:
                    return NEXT_CHAR;
            }
        }

        EStateResult HandleCdataState(int c, TToken* output) {
            if (c == EOF_CHAR || Input_.MaybeConsumeMatch("]]>", sizeof("]]>") - 1, true)) {
                ReconsumeCurrentInput_ = true;
                ResetTokenStartPoint();
                SetState(LEX_DATA);
                return NEXT_CHAR;
            } else {
                return EmitCurrentChar(output);
            }
        }

    private:
        struct TTagState {
            // A buffer to accumulate characters for various GumboStringPiece fields.
            TBuffer Buffer;

            // A pointer to the start of the original text corresponding to the contents
            // of the buffer.
            const char* OriginalText;

            // The current tag enum, computed once the tag name state has finished so that
            // the buffer can be re-used for building up attributes.
            ETag Tag;

            // The current list of attributes.  This is copied (and ownership of its data
            // transferred) to the GumboStartTag token upon completion of the tag.  New
            // attributes are added as soon as their attribute name state is complete, and
            // values are filled in by operating on _attributes.data[attributes.length-1].
            TVector<TAttribute> Attributes;

            // If true, the next attribute value to be finished should be dropped.  This
            // happens if a duplicate attribute name is encountered - we want to consume
            // the attribute value, but shouldn't overwrite the existing value.
            bool DropNextAttrValue;

            // The state that caused the tokenizer to switch into a character reference in
            // attribute value state.  This is used to set the additional allowed
            // character, and is switched back to on completion.  Initialized as the
            // tokenizer enters the character reference state.
            ETokenizerState AttrValueState;

            // The last start tag to have been emitted by the tokenizer.  This is
            // necessary to check for appropriate end tags.
            ETag LastStartTag;

            // If true, then this is a start tag.  If false, it's an end tag.  This is
            // necessary to generate the appropriate token type at tag-closing time.
            bool IsStartTag;

            // If true, then this tag is "self-closing" and doesn't have an end tag.
            bool IsSelfClosing;
        };

        struct TDoctypeState {
            TString Name;
            TString PublicIdentifier;
            TString SystemIdentifier;
            bool ForceQuirks;
            bool HasPublicIdentifier;
            bool HasSystemIdentifier;
        };

        // The current lexer state.  Starts in LEX_DATA.
        ETokenizerState State_;

        // A flag indicating whether the current input character needs to reconsumed
        // in another state, or whether the next input character should be read for
        // the next iteration of the state loop.  This is set when the spec reads
        // "Reconsume the current input character in..."
        bool ReconsumeCurrentInput_;

        // A flag indicating whether the current node is a foreign element.  This is
        // set by gumbo_tokenizer_set_is_current_node_foreign and checked in the
        // markup declaration state.
        bool IsCurrentNodeForeign_;

        // A temporary buffer to accumulate characters, as described by the "temporary
        // buffer" phrase in the tokenizer spec.  We use this in a somewhat unorthodox
        // way: we record the specific character to go into the buffer, which may
        // sometimes be a lowercased version of the actual input character.  However,
        // we *also* use utf8iterator_mark() to record the position at tag start.
        // When we start flushing the temporary buffer, we set _temporary_buffer_emit
        // to the start of it, and then increment it for each call to the tokenizer.
        // We also call utf8iterator_reset(), and utf8iterator_next() through the
        // input stream, so that tokens emitted by emit_char have the correct position
        // and original text.
        TBuffer TemporaryBuffer_;

        // The current cursor position we're emitting from within
        // _temporary_buffer.data.  NULL whenever we're not flushing the buffer.
        const char* TemporaryBufferEmit_;

        // The temporary buffer is also used by the spec to check whether we should
        // enter the script data double escaped state, but we can't use the same
        // buffer for both because we have to flush out "<s" as emits while still
        // maintaining the context that will eventually become "script".  This is a
        // separate buffer that's used in place of the temporary buffer for states
        // that may enter the script data double escape start state.
        TBuffer ScriptDataBuffer_;

        // Pointer to the beginning of the current token in the original buffer; used
        // to record the original text.
        const char* TokenStart_;

        // Current tag state.
        TTagState TagState_;

        // Doctype state.  We use the temporary buffer to accumulate characters (it's
        // not used for anything else in the doctype states), and then freshly
        // allocate the strings in the doctype token, then copy it over on emit.
        TDoctypeState DoctypeState_;

        //
        TString Comment_;

        // The iterator over the tokenizer input.
        TInput Input_;

        // Tokenizer options
        const TTokenizerOptions Options_;
    };

}
