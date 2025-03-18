#include "parser.h"

#include <util/system/compat.h>

#include <algorithm>
#include <cstdlib>
#include <ctype.h>

namespace NHtml5 {
///////////////////////////////////////////////////////////////////////////////

#define STATIC_STRING(literal) \
    { literal, sizeof(literal) - 1 }
#define TERMINATOR \
    { "", 0 }

    class TButtonScopeTags {
    public:
        ui64 TagSet[(TAG_LAST >> 6) + 1];

    public:
        TButtonScopeTags() {
            memset(&TagSet[0], 0, sizeof(TagSet));
            Set(TAG_APPLET);
            Set(TAG_CAPTION);
            Set(TAG_HTML);
            Set(TAG_TABLE);
            Set(TAG_TD);
            Set(TAG_TH);
            Set(TAG_MARQUEE);
            Set(TAG_OBJECT);
            Set(TAG_MI);
            Set(TAG_MO);
            Set(TAG_MN);
            Set(TAG_MS);
            Set(TAG_MTEXT);
            Set(TAG_ANNOTATION_XML);
            Set(TAG_FOREIGNOBJECT);
            Set(TAG_DESC);
            Set(TAG_TITLE);
            Set(TAG_BUTTON);
        }

        bool IsSet(ETag tag) {
            return TagSet[tag >> 6] & (1UL << (tag & 0x3F));
        }

    private:
        void Set(ETag tag) {
            TagSet[tag >> 6] |= (1UL << (tag & 0x3F));
        }
    };

    TButtonScopeTags ButtonScopeTags;

    static const TStringPiece kDoctypeHtml = STATIC_STRING("html");
    /*
static const TStringPiece kPublicIdHtml4_0 = STATIC_STRING(
    "-//W3C//DTD HTML 4.0//EN");
static const TStringPiece kPublicIdHtml4_01 = STATIC_STRING(
    "-//W3C//DTD HTML 4.01//EN");
static const TStringPiece kPublicIdXhtml1_0 = STATIC_STRING(
    "-//W3C//DTD XHTML 1.0 Strict//EN");
static const TStringPiece kPublicIdXhtml1_1 = STATIC_STRING(
    "-//W3C//DTD XHTML 1.1//EN");
static const TStringPiece kSystemIdRecHtml4_0 = STATIC_STRING(
    "http://www.w3.org/TR/REC-html40/strict.dtd");
static const TStringPiece kSystemIdHtml4 = STATIC_STRING(
    "http://www.w3.org/TR/html4/strict.dtd");
static const TStringPiece kSystemIdXhtmlStrict1_1 = STATIC_STRING(
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd");
static const TStringPiece kSystemIdXhtml1_1 = STATIC_STRING(
    "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd");
static const TStringPiece kSystemIdLegacyCompat = STATIC_STRING(
    "about:legacy-compat");
*/

    // The doctype arrays have an explicit terminator because we want to pass them
    // to a helper function, and passing them as a pointer discards sizeof
    // information.  The SVG arrays are used only by one-off functions, and so loops
    // over them use sizeof directly instead of a terminator.

    static const TStringPiece kQuirksModePublicIdPrefixes[] = {
        STATIC_STRING("+//Silmaril//dtd html Pro v0r11 19970101//"),
        STATIC_STRING("-//AdvaSoft Ltd//DTD HTML 3.0 asWedit + extensions//"),
        STATIC_STRING("-//AS//DTD HTML 3.0 asWedit + extensions//"),
        STATIC_STRING("-//IETF//DTD HTML 2.0 Level 1//"),
        STATIC_STRING("-//IETF//DTD HTML 2.0 Level 2//"),
        STATIC_STRING("-//IETF//DTD HTML 2.0 Strict Level 1//"),
        STATIC_STRING("-//IETF//DTD HTML 2.0 Strict Level 2//"),
        STATIC_STRING("-//IETF//DTD HTML 2.0 Strict//"),
        STATIC_STRING("-//IETF//DTD HTML 2.0//"),
        STATIC_STRING("-//IETF//DTD HTML 2.1E//"),
        STATIC_STRING("-//IETF//DTD HTML 3.0//"),
        STATIC_STRING("-//IETF//DTD HTML 3.2 Final//"),
        STATIC_STRING("-//IETF//DTD HTML 3.2//"),
        STATIC_STRING("-//IETF//DTD HTML 3//"),
        STATIC_STRING("-//IETF//DTD HTML Level 0//"),
        STATIC_STRING("-//IETF//DTD HTML Level 1//"),
        STATIC_STRING("-//IETF//DTD HTML Level 2//"),
        STATIC_STRING("-//IETF//DTD HTML Level 3//"),
        STATIC_STRING("-//IETF//DTD HTML Strict Level 0//"),
        STATIC_STRING("-//IETF//DTD HTML Strict Level 1//"),
        STATIC_STRING("-//IETF//DTD HTML Strict Level 2//"),
        STATIC_STRING("-//IETF//DTD HTML Strict Level 3//"),
        STATIC_STRING("-//IETF//DTD HTML Strict//"),
        STATIC_STRING("-//IETF//DTD HTML//"),
        STATIC_STRING("-//Metrius//DTD Metrius Presentational//"),
        STATIC_STRING("-//Microsoft//DTD Internet Explorer 2.0 HTML Strict//"),
        STATIC_STRING("-//Microsoft//DTD Internet Explorer 2.0 HTML//"),
        STATIC_STRING("-//Microsoft//DTD Internet Explorer 2.0 Tables//"),
        STATIC_STRING("-//Microsoft//DTD Internet Explorer 3.0 HTML Strict//"),
        STATIC_STRING("-//Microsoft//DTD Internet Explorer 3.0 HTML//"),
        STATIC_STRING("-//Microsoft//DTD Internet Explorer 3.0 Tables//"),
        STATIC_STRING("-//Netscape Comm. Corp.//DTD HTML//"),
        STATIC_STRING("-//Netscape Comm. Corp.//DTD Strict HTML//"),
        STATIC_STRING("-//O'Reilly and Associates//DTD HTML 2.0//"),
        STATIC_STRING("-//O'Reilly and Associates//DTD HTML Extended 1.0//"),
        STATIC_STRING("-//O'Reilly and Associates//DTD HTML Extended Relaxed 1.0//"),
        STATIC_STRING("-//SoftQuad Software//DTD HoTMetaL PRO 6.0::19990601::)"
                      "extensions to HTML 4.0//"),
        STATIC_STRING("-//SoftQuad//DTD HoTMetaL PRO 4.0::19971010::"
                      "extensions to HTML 4.0//"),
        STATIC_STRING("-//Spyglass//DTD HTML 2.0 Extended//"),
        STATIC_STRING("-//SQ//DTD HTML 2.0 HoTMetaL + extensions//"),
        STATIC_STRING("-//Sun Microsystems Corp.//DTD HotJava HTML//"),
        STATIC_STRING("-//Sun Microsystems Corp.//DTD HotJava Strict HTML//"),
        STATIC_STRING("-//W3C//DTD HTML 3 1995-03-24//"),
        STATIC_STRING("-//W3C//DTD HTML 3.2 Draft//"),
        STATIC_STRING("-//W3C//DTD HTML 3.2 Final//"),
        STATIC_STRING("-//W3C//DTD HTML 3.2//"),
        STATIC_STRING("-//W3C//DTD HTML 3.2S Draft//"),
        STATIC_STRING("-//W3C//DTD HTML 4.0 Frameset//"),
        STATIC_STRING("-//W3C//DTD HTML 4.0 Transitional//"),
        STATIC_STRING("-//W3C//DTD HTML Experimental 19960712//"),
        STATIC_STRING("-//W3C//DTD HTML Experimental 970421//"),
        STATIC_STRING("-//W3C//DTD W3 HTML//"),
        STATIC_STRING("-//W3O//DTD W3 HTML 3.0//"),
        STATIC_STRING("-//WebTechs//DTD Mozilla HTML 2.0//"),
        STATIC_STRING("-//WebTechs//DTD Mozilla HTML//"),
        TERMINATOR};

    static const TStringPiece kQuirksModePublicIdExactMatches[] = {
        STATIC_STRING("-//W3O//DTD W3 HTML Strict 3.0//EN//"),
        STATIC_STRING("-/W3C/DTD HTML 4.0 Transitional/EN"),
        STATIC_STRING("HTML"),
        TERMINATOR};

    static const TStringPiece kQuirksModeSystemIdExactMatches[] = {
        STATIC_STRING("http://www.ibm.com/data/dtd/v11/ibmxhtml1-transitional.dtd"),
        TERMINATOR};

    static const TStringPiece kLimitedQuirksPublicIdPrefixes[] = {
        STATIC_STRING("-//W3C//DTD XHTML 1.0 Frameset//"),
        STATIC_STRING("-//W3C//DTD XHTML 1.0 Transitional//"),
        TERMINATOR};

    static const TStringPiece kLimitedQuirksRequiresSystemIdPublicIdPrefixes[] = {
        STATIC_STRING("-//W3C//DTD HTML 4.01 Frameset//"),
        STATIC_STRING("-//W3C//DTD HTML 4.01 Transitional//"),
        TERMINATOR};

#undef TERMINATOR
#undef STATIC_STRING

    ///////////////////////////////////////////////////////////////////////////////

    // The "scope marker" for the list of active formatting elements.  We use a
    // pointer to this as a generic marker element, since the particular element
    // scope doesn't matter.
    static const TNode kActiveFormattingScopeMarker = TNode();

    // The tag_is and tag_in function use true & false to denote start & end tags,
    // but for readability, we define constants for them here.
    static const bool kStartTag = true;
    static const bool kEndTag = false;

    template <typename THead>
    static inline bool TagInList(const ETag tag, const THead head) {
        return tag == head;
    }

    template <typename THead, typename... TTags>
    static inline bool TagInList(const ETag tag, THead head, TTags... args) {
        return tag == head || TagInList(tag, args...);
    }

    // Returns true if the specified token is either a start or end tag (specified
    // by is_start) with one of the tag types in the varargs list.  This functions
    // as a sentinel since no portion of the spec references tags that are not in the spec.
    template <typename... TTags>
    static inline bool TagIn(const TToken* token, bool is_start, TTags... tags) {
        ETag token_tag;
        if (is_start && token->Type == TOKEN_START_TAG) {
            token_tag = token->v.StartTag.Tag;
        } else if (!is_start && token->Type == TOKEN_END_TAG) {
            token_tag = token->v.EndTag;
        } else {
            return false;
        }

        return TagInList(token_tag, tags...);
    }

    // Like TagIn, but for the single-tag case.
    static inline bool TagIs(const TToken* token, bool is_start, ETag tag) {
        if (is_start) {
            return token->Type == TOKEN_START_TAG && token->v.StartTag.Tag == tag;
        } else {
            return token->Type == TOKEN_END_TAG && token->v.EndTag == tag;
        }
    }

    // Like tag_in, but checks for the tag of a node, rather than a token.
    template <typename... TTags>
    static inline bool NodeTagIn(const TNode* node, TTags... tags) {
        assert(node != nullptr);
        if (node->Type == NODE_ELEMENT) {
            return TagInList(node->Element.Tag, tags...);
        }
        return false;
    }

    // Like node_tag_in, but for the single-tag case.
    static inline bool NodeTagIs(const TNode* node, ETag tag) {
        return node->Type == NODE_ELEMENT && node->Element.Tag == tag;
    }

    template <typename T>
    static inline ssize_t VectorIndexOf(const T& elem, const TVector<T>& vector) {
        for (int i = vector.size(); --i >= 0;) {
            if (vector[i] == elem) {
                return i;
            }
        }
        return -1;
    }

    template <template <typename> class T>
    static const TAttribute* GetAttribute(const T<TAttribute>& attributes, const TStringPiece& name) {
        for (unsigned int i = 0; i < attributes.Length; ++i) {
            const TAttribute& attr = attributes.Data[i];
            if (attr.OriginalName.Length == name.Length && strnicmp(attr.OriginalName.Data, name.Data, name.Length) == 0) {
                return &attr;
            }
        }
        return nullptr;
    }

    template <template <typename> class T>
    static const TAttribute* GetAttribute(const T<TAttribute>& attributes, const char* name) {
        TStringPiece str;
        str.Data = name;
        str.Length = strlen(name);
        return GetAttribute(attributes, str);
    }

    template <template <typename> class T>
    static bool HasAttribute(const T<TAttribute>& attributes, const TStringPiece& name) {
        return GetAttribute(attributes, name) != nullptr;
    }

    static TStringPiece UnquotedValue(const TStringPiece& val) {
        if (val.Data && val.Length > 1) {
            if (val.Data[0] == '\'' || val.Data[0] == '\"') {
                TStringPiece ret;
                ret.Data = val.Data + 1;
                ret.Length = val.Length - 2;
                return ret;
            }
        }
        return val;
    }

    // Checks if the value of the specified attribute is a case-insensitive match
    // for the specified string.
    template <template <typename> class T>
    static bool IsAttributeMatches(const T<TAttribute>& attributes, const char* name, const char* value) {
        const TAttribute* attr = GetAttribute(attributes, name);
        if (!attr) {
            return false;
        }

        if (attr->OriginalName.Data == attr->OriginalValue.Data) {
            return false;
        }

        const TStringPiece unquoted = UnquotedValue(attr->OriginalValue);

        return (unquoted.Length == strlen(value) && strnicmp(unquoted.Data, value, unquoted.Length) == 0);
    }

    // Checks if the value of the specified attribute is a case-sensitive match
    // for the specified string.
    template <template <typename> class T>
    static bool AttributeMatchesCaseSensitive(const T<TAttribute>& attributes, const char* name, const char* value) {
        const TAttribute* attr = GetAttribute(attributes, name);
        if (!attr) {
            return false;
        }

        if (attr->OriginalName.Data == attr->OriginalValue.Data && value[0] != '\0') {
            return false;
        }

        const TStringPiece unquoted = UnquotedValue(attr->OriginalValue);

        return (unquoted.Length == strlen(value) &&
                strncmp(unquoted.Data, value, unquoted.Length) == 0);
    }

    template <template <typename> class T>
    static bool AttributeMatchesCaseSensitive(const T<TAttribute>& attributes, const TAttribute& item) {
        const TAttribute* attr = GetAttribute(attributes, item.OriginalName);
        if (!attr) {
            return false;
        }

        if (attr->OriginalName.Data == attr->OriginalValue.Data) {
            return item.OriginalName.Data == item.OriginalValue.Data;
        }

        const TStringPiece unquoted_attr = UnquotedValue(attr->OriginalValue);
        const TStringPiece unquoted_item = UnquotedValue(item.OriginalValue);

        return (unquoted_attr.Length == unquoted_item.Length &&
                strncmp(unquoted_attr.Data, unquoted_item.Data, unquoted_attr.Length) == 0);
    }

    // Checks if the specified attribute vectors are identical.
    template <template <typename> class T>
    static bool AllAttributesMatch(const T<TAttribute>& attr1, const T<TAttribute>& attr2) {
        int num_unmatched_attr2_elements = attr2.Length;
        for (unsigned int i = 0; i < attr1.Length; ++i) {
            const TAttribute& attr = attr1.Data[i];
            if (AttributeMatchesCaseSensitive(attr2, attr)) {
                --num_unmatched_attr2_elements;
            } else {
                return false;
            }
        }
        return num_unmatched_attr2_elements == 0;
    }

    static bool TokenHasAttribute(const TToken* token, const char* name) {
        assert(token->Type == TOKEN_START_TAG);
        return GetAttribute(token->v.StartTag.Attributes, name) != nullptr;
    }

    static inline ETag GetEndTag(const TToken* token) {
        assert(token->Type == TOKEN_END_TAG);
        return token->v.EndTag;
    }

    static inline ETag GetStartTag(const TToken* token) {
        assert(token->Type == TOKEN_START_TAG);
        return token->v.StartTag.Tag;
    }

    // Returns true if the given needle is in the given array of literal
    // GumboStringPieces.  If exact_match is true, this requires that they match
    // exactly; otherwise, this performs a prefix match to check if any of the
    // elements in haystack start with needle.  This always performs a
    // case-insensitive match.
    static bool IsInStaticList(const TStringPiece& needle, const TStringPiece* haystack, bool exact_match) {
        for (int i = 0; haystack[i].Length != 0; ++i) {
            if (needle.Length != haystack[i].Length) {
                continue;
            }
            if (exact_match) {
                if (strncmp(needle.Data, haystack[i].Data, needle.Length) == 0) {
                    return true;
                }
            } else {
                if (strnicmp(needle.Data, haystack[i].Data, needle.Length) == 0) {
                    return true;
                }
            }
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////

    void TParser::PushButtonFlags(const TNode* node) {
        if (node->Element.Tag == TAG_P) {
            ClosePTagImplicitlyFlags_.push_back(true);
        } else {
            if (ButtonScopeTags.IsSet(node->Element.Tag)) {
                ClosePTagImplicitlyFlags_.push_back(false);
            }
        }
    }

    void TParser::PopButtonFlags() {
        ETag tag = OpenElements_.back()->Element.Tag;
        if ((tag == TAG_P) || ButtonScopeTags.IsSet(tag)) {
            ClosePTagImplicitlyFlags_.pop_back();
        }
    }

    void TParser::RebuildButtonFlags() {
        ClosePTagImplicitlyFlags_.clear();
        for (auto node : OpenElements_) {
            PushButtonFlags(node);
        }
    }

    void TParser::CheckAndRebuildButtonFlags(ETag tag) {
        if ((tag == TAG_P) || ButtonScopeTags.IsSet(tag)) {
            RebuildButtonFlags();
        }
    }

    void TParser::TTextNodeBufferState::Clear() {
        Buffer.Reset();
        StartOriginalText = nullptr;
        EndOriginalText = nullptr;
        Type = NODE_WHITESPACE;
    }

    bool TParser::TTextNodeBufferState::Empty() const {
        return StartOriginalText == EndOriginalText;
    }

    ///////////////////////////////////////////////////////////////////////////////

    TParser::TParser(const TParserOptions& opts, const char* text, size_t text_length)
        : Options_(opts)
        , Tokenizer_(TTokenizerOptions()
                         .SetCompatiblePlainText(opts.CompatiblePlainText)
                         .SetNoindexToComment(opts.NoindexToComment),
                     text, text_length)
        , InsertionMode_(INSERTION_MODE_INITIAL)
        , HeadElement_(nullptr)
        , FormElement_(nullptr)
        , ReprocessCurrentToken_(false)
        , FramesetOk_(true)
        , IgnoreNextLinefeed_(false)
        , FosterParentInsertions_(false)
        , CurrentToken_(nullptr)
        , ClosedBodyTag_(false)
        , ClosedHtmlTag_(false)
    {
        CreateNodeVector_ = [this](size_t len) -> TNode** {
            return Output_->CreateVector<TNode*>(len);
        };

        CreateAttributeVector_ = [this](size_t len) -> TAttribute* {
            return Output_->CreateVector<TAttribute>(len);
        };

        ActiveFormattingElements_.reserve(16);
        OpenElements_.reserve(16);
        ClosePTagImplicitlyFlags_.reserve(16);
        TextNode_.Clear();
    }

    void TParser::Parse(TOutput* output) {
        Output_ = output;
        Output_->Document = nullptr;
        Output_->Root = nullptr;

        {
            TNode* document_node = CreateNode(NODE_DOCUMENT);
            document_node->ParseFlags = INSERTION_BY_PARSER;
            document_node->Document.Children.Initialize(1, CreateNodeVector_);

            // Must be initialized explicitly, as there's no guarantee that we'll see a
            // doc type token.
            TDocument* document = &document_node->Document;
            document->HasDoctype = false;
            document->Name = TStringPiece::Empty();
            document->PublicIdentifier = TStringPiece::Empty();
            document->SystemIdentifier = TStringPiece::Empty();
            document->OriginalText = TStringPiece::Empty();

            Output_->Document = document_node;
        }

        TToken token;
        do {
            token = TToken{};
            TNode* current_node1 = GetCurrentNode();

            Tokenizer_.SetIsCurrentNodeForeign(current_node1 && current_node1->Element.TagNamespace != NAMESPACE_HTML);
            Tokenizer_.Lex(&token);

            CurrentToken_ = &token;

            do {
                ReprocessCurrentToken_ = false;

                if (IgnoreNextLinefeed_ && token.Type == TOKEN_WHITESPACE && token.v.Character == '\n') {
                    IgnoreNextLinefeed_ = false;
                } else {
                    // This needs to be reset both here and in the conditional above to catch both
                    // the case where the next token is not whitespace (so we don't ignore
                    // whitespace in the middle of <pre> tags) and where there are multiple
                    // whitespace tokens (so we don't ignore the second one).
                    IgnoreNextLinefeed_ = false;

                    const TNode* current_node2 = GetCurrentNode();
                    assert(!current_node2 || current_node2->Type == NODE_ELEMENT);

                    if (token.Type == TOKEN_END_TAG) {
                        if (token.v.EndTag == TAG_BODY) {
                            ClosedBodyTag_ = true;
                        }
                        if (token.v.EndTag == TAG_HTML) {
                            ClosedHtmlTag_ = true;
                        }
                    }

                    if ((!current_node2) || (current_node2->Element.TagNamespace == NAMESPACE_HTML) || (IsMathmlIntegrationPoint(current_node2) && (token.Type == TOKEN_CHARACTER || token.Type == TOKEN_WHITESPACE || token.Type == TOKEN_NULL || (token.Type == TOKEN_START_TAG && !TagIn(&token, kStartTag, TAG_MGLYPH, TAG_MALIGNMARK)))) || (current_node2->Element.TagNamespace == NAMESPACE_MATHML && NodeTagIs(current_node2, TAG_ANNOTATION_XML) && TagIs(&token, kStartTag, TAG_SVG)) || (IsHtmlIntegrationPoint(current_node2) && (token.Type == TOKEN_START_TAG || token.Type == TOKEN_CHARACTER || token.Type == TOKEN_NULL || token.Type == TOKEN_WHITESPACE)) || token.Type == TOKEN_EOF) {
                        HandleHtmlContent(&token);
                    } else {
                        HandleInForeignContent(&token);
                    }
                }
            } while (ReprocessCurrentToken_);
        } while (token.Type != TOKEN_EOF);

        //
        // Finish parsing
        //

        while (PopCurrentNode()) {
            // Pop them all.
        }
    }

    void TParser::AddFormattingElement(TNode* node) {
        assert(node == &kActiveFormattingScopeMarker || node->Type == NODE_ELEMENT);
        // Hunt for identical elements.
        int earliest_identical_element = ActiveFormattingElements_.size();
        int num_identical_elements = CountFormattingElementsOfTag(node, &earliest_identical_element);

        // Noah's Ark clause: if there're at least 3, remove the earliest.
        if (num_identical_elements >= 3) {
            ActiveFormattingElements_.erase(
                ActiveFormattingElements_.begin() + earliest_identical_element);
        }

        ActiveFormattingElements_.push_back(node);
    }

    bool TParser::AdoptionAgencyAlgorithm(const TToken* token, ETag closing_tag) {
        // Steps 1-3 & 16:
        for (int i = 0; i < 8; ++i) {
            // Step 4.
            TNode* formatting_node = nullptr;
            int formatting_node_in_open_elements = -1;
            for (int j = ActiveFormattingElements_.size(); --j >= 0;) {
                TNode* current_node = ActiveFormattingElements_[j];
                if (current_node == &kActiveFormattingScopeMarker) {
                    // Last scope marker; abort the algorithm.
                    return false;
                }
                if (NodeTagIs(current_node, closing_tag)) {
                    // Found it.
                    formatting_node = current_node;
                    formatting_node_in_open_elements = VectorIndexOf(formatting_node, OpenElements_);
                    break;
                }
            }
            if (!formatting_node) {
                // No matching tag; not a parse error outright, but fall through to the
                // "any other end tag" clause (which may potentially add a parse error,
                // but not always).
                return HandleAnyOtherEndTagInBody(token);
            }

            if (formatting_node_in_open_elements == -1) {
                ActiveFormattingElements_.erase(
                    std::remove(ActiveFormattingElements_.begin(), ActiveFormattingElements_.end(), formatting_node), ActiveFormattingElements_.end());
                return false;
            }

            if (!HasAnElementInScope(formatting_node->Element.Tag)) {
                //add_parse_error(parser, token);
                return false;
            }
            if (formatting_node != GetCurrentNode()) {
                //add_parse_error(parser, token);  // But continue onwards.
            }
            assert(formatting_node);
            assert(!NodeTagIs(formatting_node, TAG_HTML));
            assert(!NodeTagIs(formatting_node, TAG_BODY));

            // Step 5 & 6.
            TNode* furthest_block = nullptr;
            for (size_t j = formatting_node_in_open_elements; j < OpenElements_.size(); ++j) {
                assert(j > 0);
                TNode* current = OpenElements_[j];
                if (IsSpecialNode(current)) {
                    // Step 5.
                    furthest_block = current;
                    break;
                }
            }
            if (!furthest_block) {
                // Step 6.
                for (TNode* node = PopCurrentNode(); node != formatting_node;) {
                    node = PopCurrentNode();
                }
                ActiveFormattingElements_.erase(
                    std::remove(ActiveFormattingElements_.begin(), ActiveFormattingElements_.end(), formatting_node), ActiveFormattingElements_.end());
                return false;
            }
            assert(!NodeTagIs(furthest_block, TAG_HTML));
            assert(furthest_block);

            // Step 7.
            // Elements may be moved and reparented by this algorithm, so
            // common_ancestor is not necessarily the same as formatting_node->parent.
            TNode* common_ancestor = OpenElements_[VectorIndexOf(formatting_node, OpenElements_) - 1];

            // Step 8.
            ssize_t bookmark = VectorIndexOf(formatting_node, ActiveFormattingElements_);
            // Step 9.
            TNode* node = furthest_block;
            TNode* last_node = furthest_block;
            // Must be stored explicitly, in case node is removed from the stack of open
            // elements, to handle step 9.4.
            ssize_t saved_node_index = VectorIndexOf(node, OpenElements_);
            assert(saved_node_index > 0);
            // Step 9.1-9.3 & 9.11.
            for (int j = 0; j < 3; ++j) {
                // Step 9.4.
                ssize_t node_index = VectorIndexOf(node, OpenElements_);
                if (node_index == -1) {
                    node_index = saved_node_index;
                }
                saved_node_index = --node_index;
                assert(node_index > 0);
                node = OpenElements_[node_index];
                assert(node->Parent);
                // Step 9.5.
                if (VectorIndexOf(node, ActiveFormattingElements_) == -1) {
                    OpenElements_.erase(OpenElements_.begin() + node_index);
                    CheckAndRebuildButtonFlags(node->Element.Tag);
                    continue;
                } else if (node == formatting_node) {
                    // Step 9.6.
                    break;
                }
                // Step 9.7.
                int formatting_index = VectorIndexOf(node, ActiveFormattingElements_);
                node = CloneElementNode(node, INSERTION_ADOPTION_AGENCY_CLONED);
                ActiveFormattingElements_[formatting_index] = node;
                OpenElements_[node_index] = node;
                // Step 9.8.
                if (last_node == furthest_block) {
                    bookmark = formatting_index + 1;
                    assert(bookmark <= ssize_t(ActiveFormattingElements_.size()));
                }
                // Step 9.9.
                last_node->ParseFlags = EParseFlags(last_node->ParseFlags | INSERTION_ADOPTION_AGENCY_MOVED);
                RemoveFromParent(last_node);
                AppendNode(node, last_node);
                // Step 9.10.
                last_node = node;
            }

            // Step 10.
            RemoveFromParent(last_node);
            last_node->ParseFlags = EParseFlags(last_node->ParseFlags | INSERTION_ADOPTION_AGENCY_MOVED);
            if (NodeTagIn(common_ancestor, TAG_TABLE, TAG_TBODY, TAG_TFOOT, TAG_THEAD, TAG_TR)) {
                FosterParentElement(last_node);
            } else {
                AppendNode(common_ancestor, last_node);
            }

            // Step 11.
            TNode* new_formatting_node = CloneElementNode(formatting_node, INSERTION_ADOPTION_AGENCY_CLONED);
            formatting_node->ParseFlags = EParseFlags(formatting_node->ParseFlags | INSERTION_IMPLICIT_END_TAG);

            // Step 12.  Instead of appending nodes one-by-one, we swap the children
            // vector of furthest_block with the empty children of new_formatting_node,
            // reducing memory traffic and allocations.  We still have to reset their
            // parent pointers, though.

            TVectorType<TNode*> temp = new_formatting_node->Element.Children;
            new_formatting_node->Element.Children = furthest_block->Element.Children;
            furthest_block->Element.Children = temp;

            temp = new_formatting_node->Element.Children;
            for (size_t k = 0; k < temp.Length; ++k) {
                TNode* child = temp.Data[k];
                child->Parent = new_formatting_node;
            }

            // Step 13.
            AppendNode(furthest_block, new_formatting_node);

            // Step 14.
            // If the formatting node was before the bookmark, it may shift over all
            // indices after it, so we need to explicitly find the index and possibly
            // adjust the bookmark.
            int formatting_node_index = VectorIndexOf(formatting_node, ActiveFormattingElements_);
            assert(formatting_node_index != -1);
            if (formatting_node_index < bookmark) {
                --bookmark;
            }
            ActiveFormattingElements_.erase(ActiveFormattingElements_.begin() + formatting_node_index);
            assert(bookmark >= 0);
            assert(bookmark <= ssize_t(ActiveFormattingElements_.size()));
            ActiveFormattingElements_.insert(ActiveFormattingElements_.begin() + bookmark, new_formatting_node);

            // Step 15.
            {
                auto it = std::find(OpenElements_.begin(), OpenElements_.end(), formatting_node);
                if (it != OpenElements_.end()) {
                    ETag tag = (*it)->Element.Tag;
                    OpenElements_.erase(it);
                    CheckAndRebuildButtonFlags(tag);
                }
            }
            ssize_t insert_at = VectorIndexOf(furthest_block, OpenElements_) + 1;
            assert(insert_at >= 0);
            assert(insert_at <= ssize_t(OpenElements_.size()));
            OpenElements_.insert(OpenElements_.begin() + insert_at, new_formatting_node);
            CheckAndRebuildButtonFlags(new_formatting_node->Element.Tag);
        }
        return true;
    }

    void TParser::AppendCommentNode(TNode* node, const TToken* token) {
        MaybeFlushTextNodeBuffer();
        TNode* comment = CreateNode(NODE_COMMENT);
        comment->Type = NODE_COMMENT;
        comment->ParseFlags = INSERTION_NORMAL;
        comment->Text.Text = Output_->CreateString(token->v.Text);
        comment->Text.OriginalText = token->OriginalText;
        AppendNode(node, comment);
    }

    void TParser::AppendNode(TNode* parent, TNode* node) {
        assert(node->Parent == nullptr);
        assert(node->IndexWithinParent == size_t(-1));

        TVectorType<TNode*>* children;
        if (parent->Type == NODE_ELEMENT) {
            children = &parent->Element.Children;
        } else {
            assert(parent->Type == NODE_DOCUMENT);
            children = &parent->Document.Children;
        }

        // Merge text nodes.
        if (children->Length > 0) {
            TNode* sibling = children->Data[children->Length - 1];
            if (MaybeMergeNodes(sibling, node))
                return;
        }

        node->Parent = parent;
        node->IndexWithinParent = children->Length;
        children->PushBack(node, CreateNodeVector_);
        assert(node->IndexWithinParent < children->Length);
    }

    void TParser::ClearActiveFormattingElements() {
        if (ActiveFormattingElements_.empty()) {
            return;
        }
        for (ssize_t i = ActiveFormattingElements_.size() - 1; i > 0; --i) {
            if (ActiveFormattingElements_[i] == &kActiveFormattingScopeMarker) {
                ActiveFormattingElements_.erase(
                    ActiveFormattingElements_.begin() + i,
                    ActiveFormattingElements_.end());
                return;
            }
        }
        ActiveFormattingElements_.clear();
    }

    void TParser::ClearStackToTableContext() {
        while (!NodeTagIn(GetCurrentNode(), TAG_HTML, TAG_TABLE)) {
            PopCurrentNode();
        }
    }

    void TParser::ClearStackToTableBodyContext() {
        while (!NodeTagIn(GetCurrentNode(), TAG_HTML, TAG_TBODY, TAG_TFOOT, TAG_THEAD)) {
            PopCurrentNode();
        }
    }

    void TParser::ClearStackToTableRowContext() {
        while (!NodeTagIn(GetCurrentNode(), TAG_HTML, TAG_TR)) {
            PopCurrentNode();
        }
    }

    TNode* TParser::CloneElementNode(const TNode* node, EParseFlags reason) {
        assert(node->Type == NODE_ELEMENT);

        TNode* new_node = Output_->CreateNode(node->Type);
        *new_node = *node;
        new_node->Parent = nullptr;
        new_node->IndexWithinParent = -1;
        // Clear the INSERTION_IMPLICIT_END_TAG flag, as the cloned node may
        // have a separate end tag.
        new_node->ParseFlags = EParseFlags(new_node->ParseFlags & ~INSERTION_IMPLICIT_END_TAG);
        new_node->ParseFlags = EParseFlags(new_node->ParseFlags | reason | INSERTION_BY_PARSER);

        TElement* element = &new_node->Element;
        element->Children.Initialize(1, CreateNodeVector_);
        element->Attributes.Assign(node->Element.Attributes, CreateAttributeVector_);
        assert(new_node->Element.OriginalTag.Data == node->Element.OriginalTag.Data);
        assert(new_node->Element.OriginalTag.Length == node->Element.OriginalTag.Length);
        return new_node;
    }

    bool TParser::CloseCaption() {
        if (!HasAnElementInTableScope(TAG_CAPTION)) {
            //add_parse_error(parser, token);
            return false;
        }

        GenerateImpliedEndTags(TAG_LAST);
        bool result = true;
        if (!NodeTagIs(GetCurrentNode(), TAG_CAPTION)) {
            //add_parse_error(parser, token);
            result = false;
        }
        for (TNode* node = PopCurrentNode(); !NodeTagIs(node, TAG_CAPTION);) {
            node = PopCurrentNode();
        }
        ClearActiveFormattingElements();
        InsertionMode_ = INSERTION_MODE_IN_TABLE;
        return result;
    }

    bool TParser::CloseTable() {
        if (!HasAnElementInTableScope(TAG_TABLE)) {
            return false;
        }
        for (TNode* node = PopCurrentNode(); !NodeTagIs(node, TAG_TABLE);) {
            node = PopCurrentNode();
        }
        ResetInsertionModeAppropriately();
        return true;
    }

    bool TParser::CloseTableCell(const TToken*, ETag cell_tag) {
        bool result = true;
        GenerateImpliedEndTags(TAG_LAST);
        const TNode* node = GetCurrentNode();
        if (!NodeTagIs(node, cell_tag)) {
            //add_parse_error(parser, token);
            result = false;
        }
        do {
            node = PopCurrentNode();
        } while (!NodeTagIs(node, cell_tag));

        ClearActiveFormattingElements();
        InsertionMode_ = INSERTION_MODE_IN_ROW;
        return result;
    }

    bool TParser::CloseCurrentCell(const TToken* token) {
        if (HasAnElementInTableScope(TAG_TD)) {
            assert(!HasAnElementInTableScope(TAG_TH));
            return CloseTableCell(token, TAG_TD);
        } else {
            assert(HasAnElementInTableScope(TAG_TH));
            return CloseTableCell(token, TAG_TH);
        }
    }

    void TParser::CloseCurrentSelect() {
        for (TNode* node = PopCurrentNode(); !NodeTagIs(node, TAG_SELECT);) {
            node = PopCurrentNode();
        }
        ResetInsertionModeAppropriately();
    }

    EQuirksMode TParser::ComputeQuirksMode(const TTokenDocType* doctype) {
        if (doctype->ForceQuirks ||
            (doctype->Name.Length != kDoctypeHtml.Length || strncmp(doctype->Name.Data, kDoctypeHtml.Data, doctype->Name.Length) != 0) ||
            IsInStaticList(doctype->PublicIdentifier, kQuirksModePublicIdPrefixes, false) ||
            IsInStaticList(doctype->PublicIdentifier, kQuirksModePublicIdExactMatches, true) ||
            IsInStaticList(doctype->SystemIdentifier, kQuirksModeSystemIdExactMatches, true) ||
            (IsInStaticList(doctype->PublicIdentifier, kLimitedQuirksRequiresSystemIdPublicIdPrefixes, false) && !doctype->HasSystemIdentifier()))
        {
            return DOCTYPE_QUIRKS;
        } else if (
            IsInStaticList(doctype->PublicIdentifier, kLimitedQuirksPublicIdPrefixes, false) ||
            (IsInStaticList(doctype->PublicIdentifier, kLimitedQuirksRequiresSystemIdPublicIdPrefixes, false) && doctype->HasSystemIdentifier()))
 {
            return DOCTYPE_LIMITED_QUIRKS;
        }
        return DOCTYPE_NO_QUIRKS;
    }

    int TParser::CountFormattingElementsOfTag(const TNode* desired_node, int* earliest_matching_index) {
        const TElement* desired_element = &desired_node->Element;
        TVector<TNode*>& elements = ActiveFormattingElements_;
        int num_identical_elements = 0;
        for (int i = elements.size(); --i >= 0;) {
            TNode* node = elements[i];
            if (node == &kActiveFormattingScopeMarker) {
                break;
            }
            assert(node->Type == NODE_ELEMENT);
            TElement* element = &node->Element;
            if (NodeTagIs(node, desired_element->Tag) &&
                element->TagNamespace == desired_element->TagNamespace && AllAttributesMatch(element->Attributes, desired_element->Attributes))
            {
                num_identical_elements++;
                *earliest_matching_index = i;
            }
        }
        return num_identical_elements;
    }

    TNode* TParser::CreateElement(ETag tag) {
        TNode* node = CreateNode(NODE_ELEMENT);
        TElement* element = &node->Element;
        element->Children.Initialize(1, CreateNodeVector_);
        element->Attributes.Initialize(0, CreateAttributeVector_);
        element->Tag = tag;
        element->TagNamespace = NAMESPACE_HTML;
        element->OriginalTag.Data = nullptr;
        element->OriginalTag.Length = 0;
        element->OriginalEndTag.Data = nullptr;
        element->OriginalEndTag.Length = 0;
        return node;
    }

    TNode* TParser::CreateElementFromToken(const TToken* token, ENamespace tag_namespace) {
        assert(token->Type == TOKEN_START_TAG);
        const TTokenStartTag* start_tag = &token->v.StartTag;

        TNode* node = CreateNode(NODE_ELEMENT);
        TElement* element = &node->Element;
        element->Children.Initialize(1, CreateNodeVector_);
        element->Tag = start_tag->Tag;
        element->TagNamespace = tag_namespace;
        element->Attributes.Assign(token->v.StartTag.Attributes, CreateAttributeVector_);

        assert(token->OriginalText.Length >= 2);
        assert(token->OriginalText.Data[0] == '<');
        assert(token->OriginalText.Data[token->OriginalText.Length - 1] == '>');
        element->OriginalTag = token->OriginalText;
        element->OriginalEndTag.Data = nullptr;
        element->OriginalEndTag.Length = 0;

        return node;
    }

    TNode* TParser::CreateNode(ENodeType type) {
        TNode* node = Output_->CreateNode(type);
        node->Parent = nullptr;
        node->IndexWithinParent = -1;
        node->ParseFlags = INSERTION_NORMAL;
        return node;
    }

    bool TParser::FindLastAnchorIndex(int* anchor_index) const {
        for (int i = ActiveFormattingElements_.size(); --i >= 0;) {
            TNode* node = ActiveFormattingElements_[i];
            if (node == &kActiveFormattingScopeMarker) {
                return false;
            }
            if (NodeTagIs(node, TAG_A)) {
                *anchor_index = i;
                return true;
            }
        }
        return false;
    }

    void TParser::FosterParentElement(TNode* node) {
        assert(OpenElements_.size() > 2);

        node->ParseFlags = EParseFlags(node->ParseFlags | INSERTION_FOSTER_PARENTED);
        TNode* foster_parent_element = OpenElements_[0];
        assert(foster_parent_element->Type == NODE_ELEMENT);
        assert(NodeTagIs(foster_parent_element, TAG_HTML));
        for (int i = OpenElements_.size(); --i > 1;) {
            TNode* table_element = OpenElements_[i];
            if (NodeTagIs(table_element, TAG_TABLE)) {
                foster_parent_element = table_element->Parent;
                if (!foster_parent_element || foster_parent_element->Type != NODE_ELEMENT) {
                    // Table has no parent; spec says it's possible if a script manipulated
                    // the DOM, although I don't think we have to worry about this case.
                    foster_parent_element = OpenElements_[i - 1];
                    break;
                }
                assert(foster_parent_element->Type == NODE_ELEMENT);
                assert(foster_parent_element->Element.Children.Data[table_element->IndexWithinParent] == table_element);
                InsertNode(foster_parent_element, table_element->IndexWithinParent, node);
                return;
            }
        }
        if (node->Type == NODE_ELEMENT) {
            OpenElements_.push_back(node);
            PushButtonFlags(node);
        }
        AppendNode(foster_parent_element, node);
    }

    void TParser::GenerateImpliedEndTags(ETag exception) {
        while (true) {
            const TNode* current = GetCurrentNode();

            if (NodeTagIs(current, exception) ||
                !NodeTagIn(current, TAG_DD, TAG_DT, TAG_LI, TAG_OPTION,
                           TAG_OPTGROUP, TAG_P, TAG_RP, TAG_RT))
            {
                break;
            }

            PopCurrentNode();
        }
    }

    EInsertionMode TParser::GetAppropriateInsertionMode(const TNode* node, bool is_last) const {
        assert(node->Type == NODE_ELEMENT);
        switch (node->Element.Tag) {
            case TAG_SELECT:
                return INSERTION_MODE_IN_SELECT;
            case TAG_TD:
            case TAG_TH:
                return is_last ? INSERTION_MODE_IN_BODY : INSERTION_MODE_IN_CELL;
            case TAG_TR:
                return INSERTION_MODE_IN_ROW;
            case TAG_TBODY:
            case TAG_THEAD:
            case TAG_TFOOT:
                return INSERTION_MODE_IN_TABLE_BODY;
            case TAG_CAPTION:
                return INSERTION_MODE_IN_CAPTION;
            case TAG_COLGROUP:
                return INSERTION_MODE_IN_COLUMN_GROUP;
            case TAG_TABLE:
                return INSERTION_MODE_IN_TABLE;
            case TAG_HEAD:
            case TAG_BODY:
                return INSERTION_MODE_IN_BODY;
            case TAG_FRAMESET:
                return INSERTION_MODE_IN_FRAMESET;
            case TAG_HTML:
                return INSERTION_MODE_BEFORE_HEAD;
            default:
                return is_last ? INSERTION_MODE_IN_BODY : INSERTION_MODE_INITIAL;
        }
    }

    TNode* TParser::GetDocumentNode() {
        return Output_->Document;
    }

    TNode* TParser::GetCurrentNode() const {
        if (OpenElements_.empty()) {
            assert(!Output_->Root);
            return nullptr;
        }
        assert(!OpenElements_.empty());
        return OpenElements_.back();
    }

    // The following functions are all defined by the "has an element in __ scope"
    // sections of the HTML5 spec:
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#has-an-element-in-the-specific-scope
    // The basic idea behind them is that they check for an element of the given tag
    // name, contained within a scope formed by a set of other tag names.  For
    // example, "has an element in list scope" looks for an element of the given tag
    // within the nearest enclosing <ol> or <ul>, along with a bunch of generic
    // element types that serve to "firewall" their content from the rest of the
    // document.
    template <typename... TTags>
    static bool HasAnElementInSpecificScope(const ETag tag,
                                            const TVector<TNode*>& openElements,
                                            const bool negate,
                                            const TTags... tags) {
        for (ssize_t i = openElements.size(); --i >= 0;) {
            const TNode* node = openElements[i];
            assert(node->Type == NODE_ELEMENT);
            const ETag node_tag = node->Element.Tag;
            if (node_tag == tag) {
                return true;
            }
            if (negate != TagInList(node_tag, tags...)) {
                return false;
            }
        }

        return false;
    }

    template <size_t Len, typename... TTags>
    static bool HasAnElementInSpecificScope(const ETag (&expected)[Len],
                                            const TVector<TNode*>& openElements,
                                            bool negate,
                                            TTags... tags) {
        for (ssize_t i = openElements.size(); --i >= 0;) {
            const TNode* node = openElements[i];
            assert(node->Type == NODE_ELEMENT);
            const ETag node_tag = node->Element.Tag;
            for (size_t j = 0; j < Len; ++j) {
                if (node_tag == expected[j]) {
                    return true;
                }
            }
            if (negate != TagInList(node_tag, tags...)) {
                return false;
            }
        }

        return false;
    }

    bool TParser::HasAnElementInScope(ETag tag) const {
        return HasAnElementInSpecificScope(
            tag, OpenElements_, false, TAG_APPLET, TAG_CAPTION, TAG_HTML,
            TAG_TABLE, TAG_TD, TAG_TH, TAG_MARQUEE,
            TAG_OBJECT, TAG_MI, TAG_MO, TAG_MN, TAG_MS,
            TAG_MTEXT, TAG_ANNOTATION_XML, TAG_FOREIGNOBJECT,
            TAG_DESC, TAG_TITLE);
    }

    bool TParser::HasAnElementInScopeWithHeadingTags() const {
        const ETag heading[] =
            {TAG_H1, TAG_H2, TAG_H3, TAG_H4, TAG_H5, TAG_H6};

        return HasAnElementInSpecificScope(
            heading, OpenElements_, false,
            TAG_APPLET, TAG_CAPTION, TAG_HTML,
            TAG_TABLE, TAG_TD, TAG_TH, TAG_MARQUEE,
            TAG_OBJECT, TAG_MI, TAG_MO, TAG_MN, TAG_MS,
            TAG_MTEXT, TAG_ANNOTATION_XML, TAG_FOREIGNOBJECT,
            TAG_DESC, TAG_TITLE);
    }

    bool TParser::HasAnElementInButtonScope(ETag tag) const {
        return HasAnElementInSpecificScope(
            tag, OpenElements_, false, TAG_APPLET, TAG_CAPTION, TAG_HTML,
            TAG_TABLE, TAG_TD, TAG_TH, TAG_MARQUEE,
            TAG_OBJECT, TAG_MI, TAG_MO, TAG_MN, TAG_MS,
            TAG_MTEXT, TAG_ANNOTATION_XML, TAG_FOREIGNOBJECT,
            TAG_DESC, TAG_TITLE, TAG_BUTTON);
    }

    bool TParser::HasAnElementInListScope(ETag tag) const {
        return HasAnElementInSpecificScope(
            tag, OpenElements_, false, TAG_APPLET, TAG_CAPTION, TAG_HTML,
            TAG_TABLE, TAG_TD, TAG_TH, TAG_MARQUEE,
            TAG_OBJECT, TAG_MI, TAG_MO, TAG_MN, TAG_MS,
            TAG_MTEXT, TAG_ANNOTATION_XML, TAG_FOREIGNOBJECT,
            TAG_DESC, TAG_TITLE, TAG_OL, TAG_UL);
    }

    bool TParser::HasAnElementInSelectScope(ETag tag) const {
        return HasAnElementInSpecificScope(tag, OpenElements_, true, TAG_OPTGROUP, TAG_OPTION);
    }

    bool TParser::HasAnElementInTableScope(ETag tag) const {
        return HasAnElementInSpecificScope(tag, OpenElements_, false, TAG_HTML, TAG_TABLE);
    }

    bool TParser::HasNodeInScope(const TNode* node) const {
        for (int i = OpenElements_.size(); --i >= 0;) {
            const TNode* current = OpenElements_[i];
            assert(current->Type == NODE_ELEMENT);
            if (current == node) {
                return true;
            }
            if (NodeTagIn(
                    current, TAG_APPLET, TAG_CAPTION, TAG_HTML,
                    TAG_TABLE, TAG_TD, TAG_TH, TAG_MARQUEE,
                    TAG_OBJECT, TAG_MI, TAG_MO, TAG_MN,
                    TAG_MS, TAG_MTEXT, TAG_ANNOTATION_XML,
                    TAG_FOREIGNOBJECT, TAG_DESC, TAG_TITLE))
            {
                return false;
            }
        }
        assert(false);
        return false;
    }

    bool TParser::ImplicitlyCloseTags(ETag target) {
        GenerateImpliedEndTags(target);
        bool result = true;
        if (!NodeTagIs(GetCurrentNode(), target)) {
            //add_parse_error(parser, token);
            result = false;
        }
        for (TNode* node = PopCurrentNode(); !NodeTagIs(node, target);) {
            node = PopCurrentNode();
        }
        return result;
    }

    void TParser::InsertElement(TNode* node, bool is_reconstructing_formatting_elements) {
        // NOTE(jdtang): The text node buffer must always be flushed before inserting
        // a node, otherwise we're handling nodes in a different order than the spec
        // mandated.  However, one clause of the spec (character tokens in the body)
        // requires that we reconstruct the active formatting elements *before* adding
        // the character, and reconstructing the active formatting elements may itself
        // result in the insertion of new elements (which should be pushed onto the
        // stack of open elements before the buffer is flushed).  We solve this (for
        // the time being, the spec has been rewritten for <template> and the new
        // version may be simpler here) with a boolean flag to this method.
        if (!is_reconstructing_formatting_elements) {
            MaybeFlushTextNodeBuffer();
        }
        if (FosterParentInsertions_ && NodeTagIn(GetCurrentNode(), TAG_TABLE, TAG_TBODY, TAG_TFOOT, TAG_THEAD, TAG_TR)) {
            FosterParentElement(node);
            OpenElements_.push_back(node);
            PushButtonFlags(node);
            return;
        }

        // This is called to insert the root HTML element, but get_current_node
        // assumes the stack of open elements is non-empty, so we need special
        // handling for this case.
        AppendNode(Output_->Root ? GetCurrentNode() : Output_->Document, node);
        OpenElements_.push_back(node);
        PushButtonFlags(node);
    }

    TNode* TParser::InsertElementFromToken(const TToken* token) {
        TNode* element = CreateElementFromToken(token, NAMESPACE_HTML);
        InsertElement(element, false);
        return element;
    }

    TNode* TParser::InsertElementOfTagType(ETag tag, EParseFlags reason) {
        TNode* element = CreateElement(tag);
        element->ParseFlags = EParseFlags(element->ParseFlags | INSERTION_BY_PARSER | reason);
        InsertElement(element, false);
        return element;
    }

    TNode* TParser::InsertForeignElement(const TToken* token, ENamespace tag_namespace) {
        assert(token->Type == TOKEN_START_TAG);
        TNode* element = CreateElementFromToken(token, tag_namespace);
        InsertElement(element, false);
        //if (token_has_attribute(token, "xmlns") &&!attribute_matches_case_sensitive(
        //                                                &token->v.start_tag.attributes, "xmlns",
        //                                                kLegalXmlns[tag_namespace]))
        //{
        //    // TODO(jdtang): Since there're multiple possible error codes here, we
        //    // eventually need reason codes to differentiate them.
        //    //add_parse_error(parser, token);
        //}
        //if (token_has_attribute(token, "xmlns:xlink") && !attribute_matches_case_sensitive(
        //                                                    &token->v.start_tag.attributes,
        //                                                    "xmlns:xlink", "http://www.w3.org/1999/xlink")) {
        //    add_parse_error(parser, token);
        //}
        return element;
    }

    void TParser::InsertNode(TNode* parent, size_t index, TNode* node) {
        assert(node->Parent == nullptr);
        assert(node->IndexWithinParent == size_t(-1));
        assert(parent->Type == NODE_ELEMENT);
        TVectorType<TNode*>* children = &parent->Element.Children;
        assert(index < children->Length);

        if (index != 0) {
            TNode* sibling = children->Data[index - 1];
            if (MaybeMergeNodes(sibling, node))
                return;
        }

        node->Parent = parent;
        node->IndexWithinParent = index;
        children->InsertAt(node, index, CreateNodeVector_);
        assert(node->IndexWithinParent < children->Length);

        for (size_t i = index + 1; i < children->Length; ++i) {
            TNode* sibling = children->Data[i];
            sibling->IndexWithinParent = i;
            assert(sibling->IndexWithinParent < children->Length);
        }
    }

    void TParser::ProcessCharacterToken(const TToken* token) {
        assert(token->Type == TOKEN_WHITESPACE || token->Type == TOKEN_CHARACTER);

        if (Options_.PointersToOriginal && token->OriginalText.Data != TextNode_.EndOriginalText) {
            MaybeFlushTextNodeBuffer();
        }

        //
        // Append text data to buffer.
        //

        if (TextNode_.Empty()) {
            // Initialize position fields.
            TextNode_.StartOriginalText = token->OriginalText.Data;
            TextNode_.EndOriginalText = token->OriginalText.Data;
        }

        if (token->Type == TOKEN_WHITESPACE) {
            if (!Options_.PointersToOriginal) {
                TextNode_.Buffer.AppendChar(token->v.Character);
            }
        } else {
            if (!Options_.PointersToOriginal) {
                TextNode_.Buffer.AppendString(token->OriginalText);
            }
            TextNode_.Type = NODE_TEXT;
        }

        TextNode_.EndOriginalText += token->OriginalText.Length;
    }

    bool TParser::IsHtmlIntegrationPoint(const TNode* node) const {
        return (NodeTagIn(node, TAG_FOREIGNOBJECT, TAG_DESC, TAG_TITLE) && node->Element.TagNamespace == NAMESPACE_SVG) ||
               (NodeTagIs(node, TAG_ANNOTATION_XML) && (IsAttributeMatches(node->Element.Attributes,
                                                                           "encoding", "text/html") ||
                                                        IsAttributeMatches(node->Element.Attributes,
                                                                           "encoding", "application/xhtml+xml")));
    }

    bool TParser::IsMathmlIntegrationPoint(const TNode* node) const {
        return node->Element.TagNamespace == NAMESPACE_MATHML &&
               NodeTagIn(node, TAG_MI, TAG_MO, TAG_MN, TAG_MS, TAG_MTEXT);
    }

    bool TParser::IsOpenElement(const TNode* node) const {
        for (ssize_t i = OpenElements_.size(); --i >= 0;) {
            if (OpenElements_[i] == node) {
                return true;
            }
        }
        return false;
    }

    bool TParser::IsSpecialNode(const TNode* node) const {
        assert(node->Type == NODE_ELEMENT);
        switch (node->Element.TagNamespace) {
            case NAMESPACE_HTML:
                return NodeTagIn(node,
                                 TAG_ADDRESS, TAG_APPLET, TAG_AREA,
                                 TAG_ARTICLE, TAG_ASIDE, TAG_BASE,
                                 TAG_BASEFONT, TAG_BGSOUND, TAG_BLOCKQUOTE,
                                 TAG_BODY, TAG_BR, TAG_BUTTON, TAG_CAPTION,
                                 TAG_CENTER, TAG_COL, TAG_COLGROUP,
                                 TAG_DD, TAG_DETAILS, TAG_DIR,
                                 TAG_DIV, TAG_DL, TAG_DT, TAG_EMBED,
                                 TAG_FIELDSET, TAG_FIGCAPTION, TAG_FIGURE,
                                 TAG_FOOTER, TAG_FORM, TAG_FRAME,
                                 TAG_FRAMESET, TAG_H1, TAG_H2, TAG_H3,
                                 TAG_H4, TAG_H5, TAG_H6, TAG_HEAD,
                                 TAG_HEADER, TAG_HGROUP, TAG_HR, TAG_HTML,
                                 TAG_IFRAME, TAG_IMG, TAG_INPUT, TAG_ISINDEX,
                                 TAG_LI, TAG_LINK, TAG_LISTING, TAG_MAIN, TAG_MARQUEE,
                                 TAG_MENU, TAG_MENUITEM, TAG_META, TAG_NAV, TAG_NOEMBED,
                                 TAG_NOFRAMES, TAG_NOSCRIPT, TAG_OBJECT,
                                 TAG_OL, TAG_P, TAG_PARAM, TAG_PLAINTEXT,
                                 TAG_PRE, TAG_SCRIPT, TAG_SECTION, TAG_SELECT, TAG_SOURCE,
                                 TAG_STYLE, TAG_SUMMARY, TAG_TABLE, TAG_TBODY,
                                 TAG_TD, TAG_TEXTAREA, TAG_TFOOT, TAG_TH,
                                 TAG_THEAD, TAG_TITLE, TAG_TR, TAG_TRACK, TAG_UL,
                                 TAG_WBR, TAG_XMP);
            case NAMESPACE_MATHML:
                return NodeTagIn(node,
                                 TAG_MI, TAG_MO, TAG_MN, TAG_MS,
                                 TAG_MTEXT, TAG_ANNOTATION_XML);
            case NAMESPACE_SVG:
                return NodeTagIn(node, TAG_FOREIGNOBJECT, TAG_DESC);
        }
        abort();
    }

    bool TParser::MaybeAddDoctypeError(const TToken* token) {
        const TTokenDocType* doctype = &token->v.DocType;
        const bool html_doctype = doctype->Name.Data != nullptr && !strncmp(doctype->Name.Data, "html", 4);
        Y_UNUSED(html_doctype);
        bool res = false;
        /*
               (!html_doctype || doctype->has_public_identifier || (doctype->has_system_identifier && !strcmp(doctype->system_identifier, kSystemIdLegacyCompat.data)))
                &&
               !(html_doctype && (doctype_matches(doctype, &kPublicIdHtml4_0,  &kSystemIdRecHtml4_0, true) ||
                                  doctype_matches(doctype, &kPublicIdHtml4_01, &kSystemIdHtml4,      true) ||
                                  doctype_matches(doctype, &kPublicIdXhtml1_0, &kSystemIdXhtmlStrict1_1, false) ||
                                  doctype_matches(doctype, &kPublicIdXhtml1_1, &kSystemIdXhtml1_1,   false)));
                */
        if (res) {
            //add_parse_error(parser, token);
            return false;
        }
        return true;
    }

    void TParser::MaybeFlushTextNodeBuffer() {
        if (TextNode_.Empty()) {
            return;
        }

        assert(TextNode_.EndOriginalText != nullptr);
        assert(TextNode_.Type == NODE_WHITESPACE || TextNode_.Type == NODE_TEXT);
        TNode* text_node = CreateNode(TextNode_.Type);
        TText* text_node_data = &text_node->Text;

        text_node_data->OriginalText.Data = TextNode_.StartOriginalText;
        text_node_data->OriginalText.Length = TextNode_.EndOriginalText - TextNode_.StartOriginalText;
        if (Options_.PointersToOriginal) {
            text_node_data->Text = text_node_data->OriginalText;
        } else {
            text_node_data->Text = Output_->CreateString(TextNode_.Buffer.Data(), TextNode_.Buffer.Length());
        }

        if (FosterParentInsertions_ && NodeTagIn(GetCurrentNode(), TAG_TABLE, TAG_TBODY, TAG_TFOOT, TAG_THEAD, TAG_TR)) {
            FosterParentElement(text_node);
        } else {
            AppendNode(Output_->Root ? GetCurrentNode() : Output_->Document, text_node);
        }
        TextNode_.Clear();
        assert(TextNode_.Buffer.Length() == 0);
    }

    void TParser::MaybeImplicitlyCloseListTag(bool is_li) {
        FramesetOk_ = false;
        for (int i = OpenElements_.size(); --i >= 0;) {
            const TNode* node = OpenElements_[i];
            bool is_list_tag = is_li ? NodeTagIs(node, TAG_LI) : NodeTagIn(node, TAG_DD, TAG_DT);
            if (is_list_tag) {
                ImplicitlyCloseTags(node->Element.Tag);
                return;
            }
            if (IsSpecialNode(node) && !NodeTagIn(node, TAG_ADDRESS, TAG_DIV, TAG_P)) {
                return;
            }
        }
    }

    bool TParser::MaybeImplicitlyClosePTag() {
        if ((ClosePTagImplicitlyFlags_.size() > 0) && (ClosePTagImplicitlyFlags_.back())) {
            return ImplicitlyCloseTags(TAG_P);
        }
        return true;
    }

    bool TParser::MaybeMergeNodes(TNode* destination, TNode* source) const {
        if (Options_.PointersToOriginal) {
            return false;
        }
        if (source->Type != NODE_TEXT || destination->Type != NODE_TEXT) {
            return false;
        }
        destination->Text.Text = Output_->ConcatString(destination->Text.Text, source->Text.Text);
        return true;
    }

    void TParser::MergeAttributes(TToken* token, TNode* node) const {
        assert(token->Type == TOKEN_START_TAG);
        assert(node->Type == NODE_ELEMENT);
        const TRange<TAttribute>* token_attr = &token->v.StartTag.Attributes;
        TVectorType<TAttribute>* node_attr = &node->Element.Attributes;

        if (node->ParseFlags & INSERTION_IMPLIED) {
            node->ParseFlags = EParseFlags(node->ParseFlags & ~INSERTION_IMPLIED);
            node->ParseFlags = EParseFlags(node->ParseFlags | INSERTION_MERGED_ATTRIBUTES);
            node->Element.OriginalTag = token->OriginalText;
        }

        if (node->Element.Attributes.Data == nullptr) {
            node->Element.Attributes.Assign(*token_attr, CreateAttributeVector_);
        } else {
            for (size_t i = 0; i < token_attr->Length; ++i) {
                const TAttribute& attr = token_attr->Data[i];
                if (!HasAttribute(*node_attr, attr.OriginalName)) {
                    node_attr->PushBack(attr, CreateAttributeVector_);
                }
            }
        }
    }

    TNode* TParser::PopCurrentNode() {
        MaybeFlushTextNodeBuffer();
        if (OpenElements_.empty()) {
            return nullptr;
        }
        assert(NodeTagIs(OpenElements_[0], TAG_HTML));
        TNode* const current_node = OpenElements_.back();
        PopButtonFlags();
        OpenElements_.pop_back();
        assert(current_node->Type == NODE_ELEMENT);

        const bool is_closed_body_or_html_tag =
            (ClosedBodyTag_ && NodeTagIs(current_node, TAG_BODY)) ||
            (ClosedHtmlTag_ && NodeTagIs(current_node, TAG_HTML));
        if (!is_closed_body_or_html_tag) {
            if (CurrentToken_->Type != TOKEN_END_TAG || !NodeTagIs(current_node, CurrentToken_->v.EndTag)) {
                current_node->ParseFlags = EParseFlags(current_node->ParseFlags | INSERTION_IMPLICIT_END_TAG);
            }

            RecordEndOfElement(CurrentToken_, &current_node->Element);
        }

        return current_node;
    }

    void TParser::ReconstructActiveFormattingElements() {
        // Step 1
        if (ActiveFormattingElements_.empty()) {
            return;
        }

        // Step 2 & 3
        size_t i = ActiveFormattingElements_.size() - 1;
        const TNode* element = ActiveFormattingElements_[i];
        if (element == &kActiveFormattingScopeMarker || IsOpenElement(element)) {
            return;
        }

        // Step 6
        do {
            if (i == 0) {
                // Step 4
                i = -1; // Incremented to 0 below.
                break;
            }
            // Step 5
            element = ActiveFormattingElements_[--i];
        } while (element != &kActiveFormattingScopeMarker && !IsOpenElement(element));

        ++i;
        for (; i < ActiveFormattingElements_.size(); ++i) {
            // Step 7 & 8.
            assert(!ActiveFormattingElements_.empty());
            assert(i < ActiveFormattingElements_.size());
            element = ActiveFormattingElements_[i];
            assert(element != &kActiveFormattingScopeMarker);
            TNode* clone = CloneElementNode(element, INSERTION_RECONSTRUCTED_FORMATTING_ELEMENT);
            // Step 9.
            InsertElement(clone, true);
            // Step 10.
            ActiveFormattingElements_[i] = clone;
        }
    }

    void TParser::RecordEndOfElement(const TToken* current_token, TElement* element) const {
        if (current_token->Type == TOKEN_END_TAG) {
            element->OriginalEndTag = current_token->OriginalText;
        } else {
            element->OriginalEndTag.Data = nullptr;
            element->OriginalEndTag.Length = 0;
        }
    }

    void TParser::RemoveFromParent(TNode* node) {
        if (!node->Parent) {
            assert(node->IndexWithinParent == size_t(-1));
            // The node may not have a parent if, for example, it is a newly-cloned copy
            // of an active formatting element.  DOM manipulations continue with the
            // orphaned fragment of the DOM tree until it's appended/foster-parented to
            // the common ancestor at the end of the adoption agency algorithm.
            return;
        }
        assert(node->Parent->Type == NODE_ELEMENT);
        TVectorType<TNode*>* children = &node->Parent->Element.Children;
        const size_t index = node->IndexWithinParent;

        children->RemoveAt(index);
        node->Parent = nullptr;
        node->IndexWithinParent = -1;
        for (size_t i = index; i < children->Length; ++i) {
            TNode* child = children->Data[i];
            child->IndexWithinParent = i;
        }
    }

    void TParser::ResetInsertionModeAppropriately() {
        for (int i = OpenElements_.size(); --i >= 0;) {
            EInsertionMode mode = GetAppropriateInsertionMode(OpenElements_[i], i == 0);
            if (mode != INSERTION_MODE_INITIAL) {
                InsertionMode_ = mode;
                return;
            }
        }
        // Should never get here, because is_last will be set on the last iteration
        // and will force INSERTION_MODE_IN_BODY.
        assert(0);
    }

    void TParser::RunGenericParsingAlgorithm(TToken* token, ETokenizerState lexer_state) {
        InsertElementFromToken(token);
        Tokenizer_.SetState(lexer_state);
        OriginalInsertionMode_ = InsertionMode_;
        InsertionMode_ = INSERTION_MODE_TEXT;
    }

    bool TParser::HandleInitial(TToken* token) {
        TDocument* document = &Output_->Document->Document;
        if (token->Type == TOKEN_WHITESPACE) {
            return true;
        } else if (token->Type == TOKEN_COMMENT) {
            AppendCommentNode(Output_->Document, token);
            return true;
        } else if (token->Type == TOKEN_DOCTYPE) {
            document->HasDoctype = true;
            document->Name = Output_->CreateString(token->v.DocType.Name);
            document->PublicIdentifier = token->v.DocType.HasPublicIdentifier()
                                             ? Output_->CreateString(token->v.DocType.PublicIdentifier)
                                             : TStringPiece::Empty();
            document->SystemIdentifier = token->v.DocType.HasSystemIdentifier()
                                             ? Output_->CreateString(token->v.DocType.SystemIdentifier)
                                             : TStringPiece::Empty();
            document->DocTypeQuirksMode = ComputeQuirksMode(&token->v.DocType);
            document->OriginalText = token->OriginalText;
            InsertionMode_ = INSERTION_MODE_BEFORE_HTML;
            return MaybeAddDoctypeError(token);
        }
        //add_parse_error(parser, token);
        document->DocTypeQuirksMode = DOCTYPE_QUIRKS;
        InsertionMode_ = INSERTION_MODE_BEFORE_HTML;
        ReprocessCurrentToken_ = true;
        return true;
    }

    bool TParser::HandleBeforeHtml(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                if (GetStartTag(token) == TAG_HTML) {
                    Output_->Root = InsertElementFromToken(token);
                    InsertionMode_ = INSERTION_MODE_BEFORE_HEAD;
                    return true;
                }
                break;
            case TOKEN_END_TAG:
                switch (GetEndTag(token)) {
                    case TAG_HEAD:
                    case TAG_BODY:
                    case TAG_HTML:
                    case TAG_BR:
                        break;
                    default:
                        //add_parse_error(parser, token);
                        return false;
                }
                break;
            case TOKEN_COMMENT:
                AppendCommentNode(Output_->Document, token);
                return true;
            case TOKEN_WHITESPACE:
                return false;
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
            case TOKEN_EOF:
                break;
        }

        Output_->Root = InsertElementOfTagType(TAG_HTML, INSERTION_IMPLIED);
        InsertionMode_ = INSERTION_MODE_BEFORE_HEAD;
        ReprocessCurrentToken_ = true;
        return true;
    }

    bool TParser::HandleBeforeHead(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                if (GetStartTag(token) == TAG_HEAD) {
                    HeadElement_ = InsertElementFromToken(token);
                    InsertionMode_ = INSERTION_MODE_IN_HEAD;
                    return true;
                }
                break;
            case TOKEN_END_TAG:
                switch (GetEndTag(token)) {
                    case TAG_HEAD:
                    case TAG_BODY:
                    case TAG_HTML:
                    case TAG_BR:
                        break;
                    default:
                        //add_parse_error(parser, token);
                        return false;
                }
                break;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_WHITESPACE:
                return true;
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
            case TOKEN_EOF:
                break;
        }

        HeadElement_ = InsertElementOfTagType(TAG_HEAD, INSERTION_IMPLIED);
        InsertionMode_ = INSERTION_MODE_IN_HEAD;
        ReprocessCurrentToken_ = true;
        return true;
    }

    bool TParser::HandleInHead(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                switch (GetStartTag(token)) {
                    case TAG_HTML:
                        return HandleInBody(token);
                    case TAG_BASE:
                    case TAG_BASEFONT:
                    case TAG_BGSOUND:
                    case TAG_LINK:
                        InsertElementFromToken(token);
                        PopCurrentNode();
                        return true;
                    case TAG_META:
                        InsertElementFromToken(token);
                        PopCurrentNode();

                        // NOTE(jdtang): Gumbo handles only UTF-8, so the encoding clause of the
                        // spec doesn't apply.  If clients want to handle meta-tag re-encoding, they
                        // should specifically look for that string in the document and re-encode it
                        // before passing to Gumbo.
                        return true;
                    case TAG_TITLE:
                        RunGenericParsingAlgorithm(token, LEX_RCDATA);
                        return true;
                    case TAG_NOFRAMES:
                    case TAG_STYLE:
                        RunGenericParsingAlgorithm(token, LEX_RAWTEXT);
                        return true;
                    case TAG_NOSCRIPT:
                        if (Options_.EnableScripting) {
                            RunGenericParsingAlgorithm(token, LEX_RAWTEXT);
                        } else {
                            InsertElementFromToken(token);
                            InsertionMode_ = INSERTION_MODE_IN_HEAD_NOSCRIPT;
                        }
                        return true;
                    case TAG_SCRIPT:
                        RunGenericParsingAlgorithm(token, LEX_SCRIPT);
                        return true;
                    case TAG_HEAD:
                        //add_parse_error(parser, token);
                        return false;
                    // TODO case TAG_TEMPLATE:
                    //    break;
                    default:
                        break;
                }
                break;
            case TOKEN_END_TAG:
                switch (GetEndTag(token)) {
                    case TAG_HEAD: {
                        TNode* head = PopCurrentNode();
                        Y_UNUSED(head);
                        assert(NodeTagIs(head, TAG_HEAD));
                        InsertionMode_ = INSERTION_MODE_AFTER_HEAD;
                        return true;
                    }
                    case TAG_BODY:
                    case TAG_HTML:
                    case TAG_BR:
                        break;
                    // TODO case TAG_TEMPLATE:
                    //    break;
                    default:
                        //add_parse_error(parser, token);
                        return false;
                }
                break;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_WHITESPACE:
                ProcessCharacterToken(token);
                return true;
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
            case TOKEN_EOF:
                break;
        }

        const TNode* node = PopCurrentNode();
        assert(NodeTagIs(node, TAG_HEAD));
        Y_UNUSED(node);
        InsertionMode_ = INSERTION_MODE_AFTER_HEAD;
        ReprocessCurrentToken_ = true;
        return true;
    }

    bool TParser::HandleInHeadNoscript(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                switch (GetStartTag(token)) {
                    case TAG_HTML:
                        return HandleInBody(token);
                    case TAG_BASEFONT:
                    case TAG_BGSOUND:
                    case TAG_LINK:
                    case TAG_META:
                    case TAG_NOFRAMES:
                    case TAG_STYLE:
                        return HandleInHead(token);
                    case TAG_HEAD:
                    case TAG_NOSCRIPT:
                        //add_parse_error(parser, token);
                        return false;
                    default:
                        break;
                }
                break;
            case TOKEN_END_TAG:
                switch (GetEndTag(token)) {
                    case TAG_NOSCRIPT: {
                        const TNode* node = PopCurrentNode();
                        assert(NodeTagIs(node, TAG_NOSCRIPT));
                        assert(NodeTagIs(GetCurrentNode(), TAG_HEAD));
                        Y_UNUSED(node);
                        InsertionMode_ = INSERTION_MODE_IN_HEAD;
                        return true;
                    }
                    case TAG_BR:
                        break;
                    default:
                        //add_parse_error(parser, token);
                        return false;
                }
                break;
            case TOKEN_WHITESPACE:
            case TOKEN_COMMENT:
                return HandleInHead(token);
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
            case TOKEN_EOF:
                break;
        }

        //add_parse_error(parser, token);
        const TNode* node = PopCurrentNode();
        assert(NodeTagIs(node, TAG_NOSCRIPT));
        assert(NodeTagIs(GetCurrentNode(), TAG_HEAD));
        Y_UNUSED(node);
        InsertionMode_ = INSERTION_MODE_IN_HEAD;
        ReprocessCurrentToken_ = true;
        return false;
    }

    bool TParser::HandleAfterHead(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                switch (GetStartTag(token)) {
                    case TAG_HTML:
                        return HandleInBody(token);
                    case TAG_BODY:
                        InsertElementFromToken(token);
                        FramesetOk_ = false;
                        InsertionMode_ = INSERTION_MODE_IN_BODY;
                        return true;
                    case TAG_FRAMESET:
                        InsertElementFromToken(token);
                        InsertionMode_ = INSERTION_MODE_IN_FRAMESET;
                        return true;
                    case TAG_BASE:
                    case TAG_BASEFONT:
                    case TAG_BGSOUND:
                    case TAG_LINK:
                    case TAG_META:
                    case TAG_NOFRAMES:
                    case TAG_SCRIPT:
                    case TAG_STYLE:
                    // TODO case TAG_TEMPLATE:
                    case TAG_TITLE: {
                        //add_parse_error(parser, token);
                        assert(HeadElement_ != nullptr);
                        // This must be flushed before we push the head element on, as there may be
                        // pending character tokens that should be attached to the root.
                        MaybeFlushTextNodeBuffer();
                        OpenElements_.push_back(HeadElement_);
                        PushButtonFlags(HeadElement_);
                        bool result = HandleInHead(token);
                        {
                            auto it = std::find(OpenElements_.begin(), OpenElements_.end(), HeadElement_);
                            if (it != OpenElements_.end()) {
                                ETag tag = (*it)->Element.Tag;
                                OpenElements_.erase(it);
                                CheckAndRebuildButtonFlags(tag);
                            }
                        }
                        return result;
                    }
                    case TAG_HEAD:
                        //add_parse_error(parser, token);
                        return false;
                    default:
                        break;
                }
                break;
            case TOKEN_END_TAG:
                switch (GetEndTag(token)) {
                    case TAG_BODY:
                    case TAG_HTML:
                    case TAG_BR:
                        break;
                    // TODO case TAG_TEMPLATE:
                    //    return HandleInHead(token);
                    default:
                        //add_parse_error(parser, token);
                        return false;
                }
                break;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_WHITESPACE:
                ProcessCharacterToken(token);
                return true;
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
            case TOKEN_EOF:
                break;
        }

        InsertElementOfTagType(TAG_BODY, INSERTION_IMPLIED);
        InsertionMode_ = INSERTION_MODE_IN_BODY;
        ReprocessCurrentToken_ = true;
        return true;
    }

    bool TParser::HandleInBody(TToken* token) {
        assert(!OpenElements_.empty());
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_WHITESPACE:
                ReconstructActiveFormattingElements();
                ProcessCharacterToken(token);
                return true;
            case TOKEN_CHARACTER:
                ReconstructActiveFormattingElements();
                ProcessCharacterToken(token);
                FramesetOk_ = false;
                return true;
            case TOKEN_NULL:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_EOF:
                for (size_t i = 0; i < OpenElements_.size(); ++i) {
                    if (!NodeTagIn(OpenElements_[i], TAG_DD, TAG_DT, TAG_LI, TAG_P,
                                   TAG_TBODY, TAG_TD, TAG_TFOOT, TAG_TH,
                                   TAG_THEAD, TAG_TR, TAG_BODY, TAG_HTML))
                    {
                        //add_parse_error(parser, token);
                        return false;
                    }
                }
                return true;
            case TOKEN_START_TAG:
                switch (GetStartTag(token)) {
                    case TAG_HTML:
                        assert(Output_->Root != nullptr);
                        assert(Output_->Root->Type == NODE_ELEMENT);
                        //add_parse_error(parser, token);
                        MergeAttributes(token, Output_->Root);
                        return false;

                    case TAG_BASE:
                    case TAG_BASEFONT:
                    case TAG_BGSOUND:
                    case TAG_LINK:
                    case TAG_META:
                    case TAG_NOFRAMES:
                    case TAG_SCRIPT:
                    case TAG_STYLE:
                    case TAG_TITLE:
                        return HandleInHead(token);

                    case TAG_BODY: {
                        //add_parse_error(parser, token);
                        if (OpenElements_.size() < 2 || !NodeTagIs(OpenElements_[1], TAG_BODY)) {
                            return false;
                        }
                        FramesetOk_ = false;
                        MergeAttributes(token, OpenElements_[1]);
                        return false;
                    }

                    case TAG_FRAMESET: {
                        //add_parse_error(parser, token);
                        if (OpenElements_.size() < 2 || !NodeTagIs(OpenElements_[1], TAG_BODY) || !FramesetOk_) {
                            return false;
                        }
                        // Save the body node for later removal.
                        TNode* body_node = OpenElements_[1];

                        // Pop all nodes except root HTML element.
                        TNode* node;
                        do {
                            node = PopCurrentNode();
                        } while (node != body_node);

                        // Removing & destroying the body node is going to kill any nodes that have
                        // been added to the list of active formatting elements, and so we should
                        // clear it to prevent a use-after-free if the list of active formatting
                        // elements is reconstructed afterwards.  This may happen if whitespace
                        // follows the </frameset>.
                        ClearActiveFormattingElements();

                        // Remove the body node.  We may want to factor this out into a generic
                        // helper, but right now this is the only code that needs to do this.
                        TVectorType<TNode*>* children = &Output_->Root->Element.Children;
                        for (size_t i = 0; i < children->Length; ++i) {
                            if (children->Data[i] == body_node) {
                                children->RemoveAt(i);
                                break;
                            }
                        }
                        //XXX destroy_node(parser, body_node);

                        // Insert the <frameset>, and switch the insertion mode.
                        InsertElementFromToken(token);
                        InsertionMode_ = INSERTION_MODE_IN_FRAMESET;
                        return true;
                    }

                    case TAG_ADDRESS:
                    case TAG_ARTICLE:
                    case TAG_ASIDE:
                    case TAG_BLOCKQUOTE:
                    case TAG_CENTER:
                    case TAG_DETAILS:
                    case TAG_DIR:
                    case TAG_DIV:
                    case TAG_DL:
                    case TAG_FIELDSET:
                    case TAG_FIGCAPTION:
                    case TAG_FIGURE:
                    case TAG_FOOTER:
                    case TAG_HEADER:
                    case TAG_HGROUP:
                    case TAG_MAIN:
                    case TAG_MENU:
                    case TAG_NAV:
                    case TAG_OL:
                    case TAG_P:
                    case TAG_SECTION:
                    case TAG_SUMMARY:
                    case TAG_UL: {
                        const bool result = MaybeImplicitlyClosePTag();
                        InsertElementFromToken(token);
                        return result;
                    }

                    case TAG_H1:
                    case TAG_H2:
                    case TAG_H3:
                    case TAG_H4:
                    case TAG_H5:
                    case TAG_H6: {
                        bool result = MaybeImplicitlyClosePTag();
                        if (NodeTagIn(GetCurrentNode(), TAG_H1, TAG_H2, TAG_H3, TAG_H4, TAG_H5, TAG_H6)) {
                            //add_parse_error(parser, token);
                            PopCurrentNode();
                            result = false;
                        }
                        InsertElementFromToken(token);
                        return result;
                    }

                    case TAG_PRE:
                    case TAG_LISTING: {
                        const bool result = MaybeImplicitlyClosePTag();
                        InsertElementFromToken(token);
                        IgnoreNextLinefeed_ = true;
                        FramesetOk_ = false;
                        return result;
                    }

                    case TAG_FORM: {
                        if (FormElement_ != nullptr) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        const bool result = MaybeImplicitlyClosePTag();
                        FormElement_ = InsertElementFromToken(token);
                        return result;
                    }

                    case TAG_LI: {
                        MaybeImplicitlyCloseListTag(true);
                        const bool result = MaybeImplicitlyClosePTag();
                        InsertElementFromToken(token);
                        return result;
                    }

                    case TAG_DD:
                    case TAG_DT: {
                        MaybeImplicitlyCloseListTag(false);
                        const bool result = MaybeImplicitlyClosePTag();
                        InsertElementFromToken(token);
                        return result;
                    }

                    case TAG_PLAINTEXT: {
                        const bool result = MaybeImplicitlyClosePTag();
                        InsertElementFromToken(token);
                        Tokenizer_.SetState(LEX_PLAINTEXT);
                        return result;
                    }

                    case TAG_BUTTON: {
                        if (HasAnElementInScope(TAG_BUTTON)) {
                            //add_parse_error(parser, token);
                            ImplicitlyCloseTags(TAG_BUTTON);
                            ReprocessCurrentToken_ = true;
                            return false;
                        }
                        ReconstructActiveFormattingElements();
                        InsertElementFromToken(token);
                        FramesetOk_ = false;
                        return true;
                    }

                    case TAG_A: {
                        bool success = true;
                        int last_a;
                        int has_matching_a = FindLastAnchorIndex(&last_a);
                        if (has_matching_a) {
                            assert(has_matching_a == 1);
                            //add_parse_error(parser, token);
                            AdoptionAgencyAlgorithm(token, TAG_A);
                            // The adoption agency algorithm usually removes all instances of <a>
                            // from the list of active formatting elements, but in case it doesn't,
                            // we're supposed to do this.  (The conditions where it might not are
                            // listed in the spec.)

                            if (FindLastAnchorIndex(&last_a)) {
                                TNode* last_element = ActiveFormattingElements_[last_a];
                                ActiveFormattingElements_.erase(ActiveFormattingElements_.begin() + last_a);
                                {
                                    auto it = std::find(OpenElements_.begin(), OpenElements_.end(), last_element);
                                    if (it != OpenElements_.end()) {
                                        ETag tag = (*it)->Element.Tag;
                                        OpenElements_.erase(it);
                                        CheckAndRebuildButtonFlags(tag);
                                    }
                                }
                            }
                            success = false;
                        }
                        ReconstructActiveFormattingElements();
                        AddFormattingElement(InsertElementFromToken(token));
                        return success;
                    }

                    case TAG_B:
                    case TAG_BIG:
                    case TAG_CODE:
                    case TAG_EM:
                    case TAG_FONT:
                    case TAG_I:
                    case TAG_S:
                    case TAG_SMALL:
                    case TAG_STRIKE:
                    case TAG_STRONG:
                    case TAG_TT:
                    case TAG_U:
                        ReconstructActiveFormattingElements();
                        AddFormattingElement(InsertElementFromToken(token));
                        return true;

                    case TAG_NOBR: {
                        bool result = true;
                        ReconstructActiveFormattingElements();
                        if (HasAnElementInScope(TAG_NOBR)) {
                            result = false;
                            //add_parse_error(parser, token);
                            AdoptionAgencyAlgorithm(token, TAG_NOBR);
                            ReconstructActiveFormattingElements();
                        }
                        InsertElementFromToken(token);
                        AddFormattingElement(GetCurrentNode());
                        return result;
                    }

                    case TAG_APPLET:
                    case TAG_MARQUEE:
                    case TAG_OBJECT:
                        ReconstructActiveFormattingElements();
                        InsertElementFromToken(token);
                        AddFormattingElement((TNode*)&kActiveFormattingScopeMarker);
                        FramesetOk_ = false;
                        return true;

                    case TAG_TABLE:
                        if (GetDocumentNode()->Document.DocTypeQuirksMode != DOCTYPE_QUIRKS) {
                            MaybeImplicitlyClosePTag();
                        }
                        InsertElementFromToken(token);
                        FramesetOk_ = false;
                        InsertionMode_ = INSERTION_MODE_IN_TABLE;
                        return true;

                    case TAG_AREA:
                    case TAG_BR:
                    case TAG_EMBED:
                    case TAG_IMG:
                    case TAG_IMAGE:
                    case TAG_KEYGEN:
                    case TAG_WBR: {
                        bool success = true;
                        if (GetStartTag(token) == TAG_IMAGE) {
                            success = false;
                            //add_parse_error(parser, token);
                            token->v.StartTag.Tag = TAG_IMG;
                        }
                        ReconstructActiveFormattingElements();
                        TNode* node = InsertElementFromToken(token);
                        if (GetStartTag(token) == TAG_IMAGE) {
                            success = false;
                            //add_parse_error(parser, token);
                            node->Element.Tag = TAG_IMG;
                            node->ParseFlags = EParseFlags(node->ParseFlags | INSERTION_FROM_IMAGE);
                        }
                        PopCurrentNode();
                        FramesetOk_ = false;
                        return success;
                    }

                    case TAG_TEXTAREA:
                        RunGenericParsingAlgorithm(token, LEX_RCDATA);
                        IgnoreNextLinefeed_ = true;
                        FramesetOk_ = false;
                        return true;

                    case TAG_XMP: {
                        const bool result = MaybeImplicitlyClosePTag();
                        ReconstructActiveFormattingElements();
                        FramesetOk_ = false;
                        RunGenericParsingAlgorithm(token, LEX_RAWTEXT);
                        return result;
                    }

                    case TAG_IFRAME:
                        FramesetOk_ = false;
                        RunGenericParsingAlgorithm(token, LEX_RAWTEXT);
                        return true;

                    case TAG_NOEMBED:
                        RunGenericParsingAlgorithm(token, LEX_RAWTEXT);
                        return true;

                    case TAG_NOSCRIPT:
                        if (Options_.EnableScripting) {
                            RunGenericParsingAlgorithm(token, LEX_RAWTEXT);
                            return true;
                        }
                        break;

                    case TAG_SELECT: {
                        ReconstructActiveFormattingElements();
                        InsertElementFromToken(token);
                        FramesetOk_ = false;
                        EInsertionMode state = InsertionMode_;
                        if (state == INSERTION_MODE_IN_TABLE ||
                            state == INSERTION_MODE_IN_CAPTION ||
                            state == INSERTION_MODE_IN_TABLE_BODY ||
                            state == INSERTION_MODE_IN_ROW ||
                            state == INSERTION_MODE_IN_CELL)
                        {
                            InsertionMode_ = INSERTION_MODE_IN_SELECT_IN_TABLE;
                        } else {
                            InsertionMode_ = INSERTION_MODE_IN_SELECT;
                        }
                        return true;
                    }

                    case TAG_OPTION:
                    case TAG_OPTGROUP: {
                        if (NodeTagIs(GetCurrentNode(), TAG_OPTION)) {
                            PopCurrentNode();
                        }
                        ReconstructActiveFormattingElements();
                        InsertElementFromToken(token);
                        return true;
                    }

                    case TAG_RP:
                    case TAG_RT: {
                        bool success = true;
                        if (HasAnElementInScope(TAG_RUBY)) {
                            GenerateImpliedEndTags(TAG_LAST);
                        }
                        if (!NodeTagIs(GetCurrentNode(), TAG_RUBY)) {
                            //add_parse_error(parser, token);
                            success = false;
                        }
                        InsertElementFromToken(token);
                        return success;
                    }

                    case TAG_MATH: {
                        ReconstructActiveFormattingElements();
                        //adjust_mathml_attributes(parser, token);
                        //adjust_foreign_attributes(parser, token);
                        InsertForeignElement(token, NAMESPACE_MATHML);
                        if (token->v.StartTag.IsSelfClosing) {
                            PopCurrentNode();
                        }
                        return true;
                    }

                    case TAG_SVG: {
                        ReconstructActiveFormattingElements();
                        //adjust_svg_attributes(parser, token);
                        //adjust_foreign_attributes(parser, token);
                        InsertForeignElement(token, NAMESPACE_SVG);
                        if (token->v.StartTag.IsSelfClosing) {
                            PopCurrentNode();
                        }
                        return true;
                    }

                    case TAG_CAPTION:
                    case TAG_COL:
                    case TAG_COLGROUP:
                    case TAG_FRAME:
                    case TAG_HEAD:
                    case TAG_TBODY:
                    case TAG_TD:
                    case TAG_TFOOT:
                    case TAG_TH:
                    case TAG_THEAD:
                    case TAG_TR:
                        //add_parse_error(parser, token);
                        return false;

                    case TAG_INPUT: {
                        if (!IsAttributeMatches(token->v.StartTag.Attributes, "type", "hidden")) {
                            // Must be before the element is inserted, as that takes ownership of the
                            // token's attribute vector.
                            FramesetOk_ = false;
                        }
                        ReconstructActiveFormattingElements();
                        InsertElementFromToken(token);
                        PopCurrentNode();
                        return true;
                    }

                    case TAG_MENUITEM:
                    case TAG_PARAM:
                    case TAG_SOURCE:
                    case TAG_TRACK: {
                        InsertElementFromToken(token);
                        PopCurrentNode();
                        return true;
                    }

                    case TAG_HR: {
                        const bool result = MaybeImplicitlyClosePTag();
                        InsertElementFromToken(token);
                        PopCurrentNode();
                        FramesetOk_ = false;
                        return result;
                    }

                    case TAG_ISINDEX: {
                        //add_parse_error(parser, token);
                        if (FormElement_ != nullptr) {
                            return false;
                        }

                        MaybeImplicitlyClosePTag();
                        FramesetOk_ = false;

                        TRange<TAttribute>* token_attrs = &token->v.StartTag.Attributes;
                        const TAttribute* prompt_attr = GetAttribute(*token_attrs, "prompt");
                        const TAttribute* action_attr = GetAttribute(*token_attrs, "action");
                        const TAttribute* name_attr = GetAttribute(*token_attrs, "name");

                        TNode* form = InsertElementOfTagType(TAG_FORM, INSERTION_FROM_ISINDEX);
                        if (action_attr) {
                            if (form->Element.Attributes.Data == nullptr) {
                                form->Element.Attributes.Initialize(1, CreateAttributeVector_);
                            }
                            form->Element.Attributes.PushBack(*action_attr, CreateAttributeVector_);
                        }
                        InsertElementOfTagType(TAG_HR, INSERTION_FROM_ISINDEX);
                        PopCurrentNode(); // <hr>

                        InsertElementOfTagType(TAG_LABEL, INSERTION_FROM_ISINDEX);
                        TextNode_.Buffer.Reset();
                        TextNode_.StartOriginalText = token->OriginalText.Data;
                        TextNode_.EndOriginalText = token->OriginalText.Data + token->OriginalText.Length;
                        TextNode_.Type = NODE_TEXT;
                        if (prompt_attr) {
                            TextNode_.Buffer.AppendString(UnquotedValue(prompt_attr->OriginalValue));
                        } else {
                            TextNode_.Buffer.AppendString("This is a searchable index. Enter search keywords: ");
                        }

                        TNode* input = InsertElementOfTagType(TAG_INPUT, INSERTION_FROM_ISINDEX);
                        if (input->Element.Attributes.Data == nullptr) {
                            input->Element.Attributes.Initialize(2, CreateAttributeVector_);
                        }
                        for (size_t i = 0; i < token_attrs->Length; ++i) {
                            const TAttribute* attr = &token_attrs->Data[i];
                            if (attr != prompt_attr && attr != action_attr && attr != name_attr) {
                                input->Element.Attributes.PushBack(*attr, CreateAttributeVector_);
                            }
                        }

                        // All attributes have been successfully transferred and nulled out at this
                        // point, so the call to ignore_token will free the memory for it without
                        // touching the attributes.
                        TAttribute name;
                        name.AttrNamespace = ATTR_NAMESPACE_NONE;
                        name.OriginalName = Output_->CreateString("name");
                        name.OriginalValue = Output_->CreateString("isindex");
                        input->Element.Attributes.PushBack(name, CreateAttributeVector_);

                        PopCurrentNode(); // <input>
                        PopCurrentNode(); // <label>
                        InsertElementOfTagType(TAG_HR, INSERTION_FROM_ISINDEX);
                        PopCurrentNode(); // <hr>
                        PopCurrentNode(); // <form>
                        return false;
                    }

                    default:
                        break;
                }
                ReconstructActiveFormattingElements();
                InsertElementFromToken(token);
                return true;
            case TOKEN_END_TAG:
                switch (GetEndTag(token)) {
                    case TAG_BODY:
                    case TAG_HTML: {
                        if (!HasAnElementInScope(TAG_BODY)) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        bool success = true;
                        for (size_t i = 0; i < OpenElements_.size(); ++i) {
                            if (!NodeTagIn(OpenElements_[i], TAG_DD,
                                           TAG_DT, TAG_LI, TAG_OPTGROUP,
                                           TAG_OPTION, TAG_P, TAG_RP,
                                           TAG_RT, TAG_TBODY, TAG_TD,
                                           TAG_TFOOT, TAG_TH, TAG_THEAD,
                                           TAG_TR, TAG_BODY, TAG_HTML))
                            {
                                //add_parse_error(parser, token);
                                success = false;
                                break;
                            }
                        }
                        InsertionMode_ = INSERTION_MODE_AFTER_BODY;
                        if (GetEndTag(token) == TAG_HTML) {
                            ReprocessCurrentToken_ = true;
                        } else {
                            TNode* body = OpenElements_[1];
                            assert(NodeTagIs(body, TAG_BODY));
                            RecordEndOfElement(CurrentToken_, &body->Element);
                        }
                        return success;
                    }

                    case TAG_ADDRESS:
                    case TAG_ARTICLE:
                    case TAG_ASIDE:
                    case TAG_BLOCKQUOTE:
                    case TAG_BUTTON:
                    case TAG_CENTER:
                    case TAG_DETAILS:
                    case TAG_DIR:
                    case TAG_DIV:
                    case TAG_DL:
                    case TAG_FIELDSET:
                    case TAG_FIGCAPTION:
                    case TAG_FIGURE:
                    case TAG_FOOTER:
                    case TAG_HEADER:
                    case TAG_HGROUP:
                    case TAG_LISTING:
                    case TAG_MAIN:
                    case TAG_MENU:
                    case TAG_NAV:
                    case TAG_OL:
                    case TAG_PRE:
                    case TAG_SECTION:
                    case TAG_SUMMARY:
                    case TAG_UL: {
                        const ETag tag = GetEndTag(token);
                        if (!HasAnElementInScope(tag)) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        ImplicitlyCloseTags(tag);
                        return true;
                    }

                    case TAG_FORM: {
                        bool result = true;
                        TNode* node = FormElement_;
                        assert(!node || node->Type == NODE_ELEMENT);
                        FormElement_ = nullptr;
                        if (!node || !HasNodeInScope(node)) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        // This differs from implicitly_close_tags because we remove *only* the
                        // <form> element; other nodes are left in scope.
                        GenerateImpliedEndTags(TAG_LAST);
                        if (GetCurrentNode() != node) {
                            //add_parse_error(parser, token);
                            result = false;
                        }

                        MaybeFlushTextNodeBuffer();

                        int index = OpenElements_.size() - 1;
                        for (; index >= 0 && OpenElements_[index] != node; --index) {
                        }
                        assert(index >= 0);
                        ETag tag = OpenElements_[index]->Element.Tag;
                        OpenElements_.erase(OpenElements_.begin() + index);
                        CheckAndRebuildButtonFlags(tag);
                        RecordEndOfElement(CurrentToken_, &node->Element);

                        return result;
                    }

                    case TAG_P: {
                        if (!HasAnElementInButtonScope(TAG_P)) {
                            //add_parse_error(parser, token);
                            ReconstructActiveFormattingElements();
                            InsertElementOfTagType(TAG_P, INSERTION_CONVERTED_FROM_END_TAG);
                            ReprocessCurrentToken_ = true;
                            return false;
                        }
                        return ImplicitlyCloseTags(TAG_P);
                    }

                    case TAG_LI: {
                        if (!HasAnElementInListScope(TAG_LI)) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        return ImplicitlyCloseTags(TAG_LI);
                    }

                    case TAG_DD:
                    case TAG_DT: {
                        const ETag tag = GetEndTag(token);
                        if (!HasAnElementInScope(tag)) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        return ImplicitlyCloseTags(tag);
                    }

                    case TAG_H1:
                    case TAG_H2:
                    case TAG_H3:
                    case TAG_H4:
                    case TAG_H5:
                    case TAG_H6: {
                        if (!HasAnElementInScopeWithHeadingTags()) {
                            // No heading open; ignore the token entirely.
                            //add_parse_error(parser, token);
                            return false;
                        } else {
                            GenerateImpliedEndTags(TAG_LAST);
                            const TNode* current_node = GetCurrentNode();
                            bool success = NodeTagIs(current_node, token->v.EndTag);
                            if (!success) {
                                // There're children of the heading currently open; close them below and
                                // record a parse error.
                                // TODO(jdtang): Add a way to distinguish this error case from the one
                                // above.

                                //add_parse_error(parser, token);
                            }
                            do {
                                current_node = PopCurrentNode();
                            } while (!NodeTagIn(current_node, TAG_H1, TAG_H2, TAG_H3,
                                                TAG_H4, TAG_H5, TAG_H6));
                            return success;
                        }
                    }

                    case TAG_A:
                    case TAG_B:
                    case TAG_BIG:
                    case TAG_CODE:
                    case TAG_EM:
                    case TAG_FONT:
                    case TAG_I:
                    case TAG_NOBR:
                    case TAG_S:
                    case TAG_SMALL:
                    case TAG_STRIKE:
                    case TAG_STRONG:
                    case TAG_TT:
                    case TAG_U:
                        return AdoptionAgencyAlgorithm(token, token->v.EndTag);

                    case TAG_APPLET:
                    case TAG_MARQUEE:
                    case TAG_OBJECT: {
                        const ETag tag = GetEndTag(token);
                        if (!HasAnElementInTableScope(tag)) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        ImplicitlyCloseTags(tag);
                        ClearActiveFormattingElements();
                        return true;
                    }

                    case TAG_BR: {
                        //add_parse_error(parser, token);
                        ReconstructActiveFormattingElements();
                        InsertElementOfTagType(TAG_BR, INSERTION_CONVERTED_FROM_END_TAG);
                        PopCurrentNode();
                        return false;
                    }

                    default:
                        break;
                }
                break;
        }

        return HandleAnyOtherEndTagInBody(token);
    }

    bool TParser::HandleText(TToken* token) {
        if (token->Type == TOKEN_CHARACTER || token->Type == TOKEN_WHITESPACE) {
            ProcessCharacterToken(token);
        } else {
            // We provide only bare-bones script handling that doesn't involve any of
            // the parser-pause/already-started/script-nesting flags or re-entrant
            // invocations of the tokenizer.  Because the intended usage of this library
            // is mostly for templating, refactoring, and static-analysis libraries, we
            // provide the script body as a text-node child of the <script> element.
            // This behavior doesn't support document.write of partial HTML elements,
            // but should be adequate for almost all other scripting support.
            if (token->Type == TOKEN_EOF) {
                //add_parse_error(parser, token);
                ReprocessCurrentToken_ = true;
            }
            PopCurrentNode();
            InsertionMode_ = OriginalInsertionMode_;
        }
        return true;
    }

    bool TParser::HandleInTable(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                switch (GetStartTag(token)) {
                    case TAG_CAPTION:
                        ClearStackToTableContext();
                        AddFormattingElement((TNode*)&kActiveFormattingScopeMarker);
                        InsertElementFromToken(token);
                        InsertionMode_ = INSERTION_MODE_IN_CAPTION;
                        return true;
                    case TAG_COLGROUP:
                        ClearStackToTableContext();
                        InsertElementFromToken(token);
                        InsertionMode_ = INSERTION_MODE_IN_COLUMN_GROUP;
                        return true;
                    case TAG_COL:
                        ClearStackToTableContext();
                        InsertElementOfTagType(TAG_COLGROUP, INSERTION_IMPLIED);
                        InsertionMode_ = INSERTION_MODE_IN_COLUMN_GROUP;
                        ReprocessCurrentToken_ = true;
                        return true;
                    case TAG_TBODY:
                    case TAG_TFOOT:
                    case TAG_THEAD:
                        ClearStackToTableContext();
                        InsertElementFromToken(token);
                        InsertionMode_ = INSERTION_MODE_IN_TABLE_BODY;
                        return true;
                    case TAG_TD:
                    case TAG_TH:
                    case TAG_TR:
                        ClearStackToTableContext();
                        InsertElementOfTagType(TAG_TBODY, INSERTION_IMPLIED);
                        InsertionMode_ = INSERTION_MODE_IN_TABLE_BODY;
                        ReprocessCurrentToken_ = true;
                        return true;
                    case TAG_TABLE:
                        //add_parse_error(parser, token);
                        if (CloseTable()) {
                            ReprocessCurrentToken_ = true;
                        }
                        return false;
                    case TAG_STYLE:
                    case TAG_SCRIPT:
                        // TODO case TAG_TEMPLATE:
                        return HandleInHead(token);
                    case TAG_INPUT:
                        if (IsAttributeMatches(token->v.StartTag.Attributes, "type", "hidden")) {
                            //add_parse_error(parser, token);
                            InsertElementFromToken(token);
                            PopCurrentNode();
                            return false;
                        }
                        break;
                    case TAG_FORM:
                        //add_parse_error(parser, token);
                        if (FormElement_) {
                            // TODO If there is a template element on the stack of open elements,
                            // or if the form element pointer is not null, ignore the token.
                            return false;
                        }
                        FormElement_ = InsertElementFromToken(token);
                        PopCurrentNode();
                        return false;
                    default:
                        break;
                }
                break;
            case TOKEN_END_TAG:
                switch (GetEndTag(token)) {
                    case TAG_TABLE:
                        if (!CloseTable()) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        return true;
                    case TAG_BODY:
                    case TAG_CAPTION:
                    case TAG_COL:
                    case TAG_COLGROUP:
                    case TAG_HTML:
                    case TAG_TBODY:
                    case TAG_TD:
                    case TAG_TFOOT:
                    case TAG_TH:
                    case TAG_THEAD:
                    case TAG_TR:
                        //add_parse_error(parser, token);
                        return false;
                    // TODO case TAG_TEMPLATE:
                    //    return HandleInHead(token);
                    default:
                        break;
                }
                break;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_WHITESPACE:
            case TOKEN_CHARACTER:
                // The "pending table character tokens" list described in the spec is
                // nothing more than the TextNodeBufferState.  We accumulate text tokens as
                // normal, except that when we go to flush them in the handle_in_table_text,
                // we set _foster_parent_insertions if there're non-whitespace characters in
                // the buffer.

                assert(TextNode_.Empty());
                OriginalInsertionMode_ = InsertionMode_;
                InsertionMode_ = INSERTION_MODE_IN_TABLE_TEXT;
                ReprocessCurrentToken_ = true;
                return true;
            case TOKEN_NULL:
                break;
            case TOKEN_EOF:
                if (!NodeTagIs(GetCurrentNode(), TAG_HTML)) {
                    //add_parse_error(parser, token);
                    return false;
                }
                return true;
        }

        //add_parse_error(parser, token);
        FosterParentInsertions_ = true;
        bool result = HandleInBody(token);
        FosterParentInsertions_ = false;
        return result;
    }

    bool TParser::HandleInTableText(TToken* token) {
        if (token->Type == TOKEN_NULL) {
            //add_parse_error(parser, token);
            return false;
        } else if (token->Type == TOKEN_CHARACTER || token->Type == TOKEN_WHITESPACE) {
            ProcessCharacterToken(token);
            return true;
        } else {
            // TODO Maybe not same as in standard.
            if (TextNode_.Type == NODE_TEXT) {
                //add_parse_error(parser, token);
                FosterParentInsertions_ = true;
                ReconstructActiveFormattingElements();
            }

            MaybeFlushTextNodeBuffer();
            FosterParentInsertions_ = false;
            InsertionMode_ = OriginalInsertionMode_;
            ReprocessCurrentToken_ = true;
            return true;
        }
    }

    bool TParser::HandleInCaption(TToken* token) {
        if (token->Type == TOKEN_START_TAG) {
            switch (GetStartTag(token)) {
                case TAG_CAPTION:
                case TAG_COL:
                case TAG_COLGROUP:
                case TAG_TBODY:
                case TAG_TD:
                case TAG_TFOOT:
                case TAG_TH:
                case TAG_THEAD:
                case TAG_TR:
                    ReprocessCurrentToken_ = true;
                    return CloseCaption();
                default:
                    break;
            }
        } else if (token->Type == TOKEN_END_TAG) {
            switch (GetEndTag(token)) {
                case TAG_CAPTION:
                    return CloseCaption();
                case TAG_TABLE:
                    ReprocessCurrentToken_ = true;
                    return CloseCaption();
                case TAG_BODY:
                case TAG_COL:
                case TAG_COLGROUP:
                case TAG_HTML:
                case TAG_TBODY:
                case TAG_TD:
                case TAG_TFOOT:
                case TAG_TH:
                case TAG_THEAD:
                case TAG_TR:
                    //add_parse_error(parser, token);
                    return false;
                default:
                    break;
            }
        }
        return HandleInBody(token);
    }

    bool TParser::HandleInColumnGroup(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                switch (GetStartTag(token)) {
                    case TAG_HTML:
                        return HandleInBody(token);
                    case TAG_COL:
                        InsertElementFromToken(token);
                        PopCurrentNode();
                        return true;
                    // TODO case TAG_TEMPLATE:
                    //    return HandleInHead(token);
                    default:
                        break;
                }
                break;
            case TOKEN_END_TAG:
                switch (GetEndTag(token)) {
                    case TAG_COLGROUP:
                        if (!NodeTagIs(GetCurrentNode(), TAG_COLGROUP)) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        PopCurrentNode();
                        InsertionMode_ = INSERTION_MODE_IN_TABLE;
                        return true;
                    case TAG_COL:
                        //add_parse_error(parser, token);
                        return false;
                    // TODO case TAG_TEMPLATE:
                    //    return HandleInHead(token);
                    default:
                        break;
                }
                break;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_WHITESPACE:
                ProcessCharacterToken(token);
                return true;
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
                break;
            case TOKEN_EOF:
                return HandleInBody(token);
        }

        if (!NodeTagIs(GetCurrentNode(), TAG_COLGROUP)) {
            //add_parse_error(parser, token);
            return false;
        }
        PopCurrentNode();
        InsertionMode_ = INSERTION_MODE_IN_TABLE;
        ReprocessCurrentToken_ = true;
        return true;
    }

    bool TParser::HandleInTableBody(TToken* token) {
        if (token->Type == TOKEN_START_TAG) {
            switch (GetStartTag(token)) {
                case TAG_TR:
                    ClearStackToTableBodyContext();
                    InsertElementFromToken(token);
                    InsertionMode_ = INSERTION_MODE_IN_ROW;
                    return true;
                case TAG_TD:
                case TAG_TH:
                    //add_parse_error(parser, token);
                    ClearStackToTableBodyContext();
                    InsertElementOfTagType(TAG_TR, INSERTION_IMPLIED);
                    InsertionMode_ = INSERTION_MODE_IN_ROW;
                    ReprocessCurrentToken_ = true;
                    return false;
                case TAG_CAPTION:
                case TAG_COL:
                case TAG_COLGROUP:
                case TAG_TBODY:
                case TAG_TFOOT:
                case TAG_THEAD:
                    if (!(HasAnElementInTableScope(TAG_TBODY) ||
                          HasAnElementInTableScope(TAG_THEAD) ||
                          HasAnElementInTableScope(TAG_TFOOT)))
                    {
                        //add_parse_error(parser, token);
                        return false;
                    }
                    ClearStackToTableBodyContext();
                    PopCurrentNode();
                    InsertionMode_ = INSERTION_MODE_IN_TABLE;
                    ReprocessCurrentToken_ = true;
                    return true;
                default:
                    break;
            }
        } else if (token->Type == TOKEN_END_TAG) {
            switch (GetEndTag(token)) {
                case TAG_TBODY:
                case TAG_TFOOT:
                case TAG_THEAD:
                    if (!HasAnElementInTableScope(GetEndTag(token))) {
                        //add_parse_error(parser, token);
                        return false;
                    }
                    ClearStackToTableBodyContext();
                    PopCurrentNode();
                    InsertionMode_ = INSERTION_MODE_IN_TABLE;
                    return true;
                case TAG_TABLE:
                    if (!(HasAnElementInTableScope(TAG_TBODY) ||
                          HasAnElementInTableScope(TAG_THEAD) ||
                          HasAnElementInTableScope(TAG_TFOOT)))
                    {
                        //add_parse_error(parser, token);
                        return false;
                    }
                    ClearStackToTableBodyContext();
                    PopCurrentNode();
                    InsertionMode_ = INSERTION_MODE_IN_TABLE;
                    ReprocessCurrentToken_ = true;
                    return true;
                case TAG_BODY:
                case TAG_CAPTION:
                case TAG_COL:
                case TAG_COLGROUP:
                case TAG_HTML:
                case TAG_TD:
                case TAG_TH:
                case TAG_TR:
                    //add_parse_error(parser, token);
                    return false;
                default:
                    break;
            }
        }

        return HandleInTable(token);
    }

    bool TParser::HandleInRow(TToken* token) {
        if (TagIn(token, kStartTag, TAG_TH, TAG_TD)) {
            ClearStackToTableRowContext();
            InsertElementFromToken(token);
            InsertionMode_ = INSERTION_MODE_IN_CELL;
            AddFormattingElement((TNode*)&kActiveFormattingScopeMarker);
            return true;
        } else if (TagIn(token, kStartTag, TAG_CAPTION, TAG_COLGROUP,
                         TAG_TBODY, TAG_TFOOT, TAG_THEAD,
                         TAG_TR) ||
                   TagIn(token, kEndTag, TAG_TR, TAG_TABLE,
                         TAG_TBODY, TAG_TFOOT, TAG_THEAD))
 {
            // This case covers 4 clauses of the spec, each of which say "Otherwise, act
            // as if an end tag with the tag name "tr" had been seen."  The differences
            // are in error handling and whether the current token is reprocessed.
            ETag desired_tag =
                TagIn(token, kEndTag, TAG_TBODY, TAG_TFOOT, TAG_THEAD)
                    ? token->v.EndTag
                    : TAG_TR;
            if (!HasAnElementInTableScope(desired_tag)) {
                //add_parse_error(parser, token);
                return false;
            }
            ClearStackToTableRowContext();
            TNode* last_element = PopCurrentNode();
            assert(NodeTagIs(last_element, TAG_TR));
            Y_UNUSED(last_element);
            InsertionMode_ = INSERTION_MODE_IN_TABLE_BODY;
            if (!TagIs(token, kEndTag, TAG_TR)) {
                ReprocessCurrentToken_ = true;
            }
            return true;
        } else if (TagIn(token, kEndTag, TAG_BODY, TAG_CAPTION, TAG_COL, TAG_COLGROUP, TAG_HTML, TAG_TD, TAG_TH)) {
            //add_parse_error(parser, token);
            return false;
        } else {
            return HandleInTable(token);
        }
    }

    bool TParser::HandleInCell(TToken* token) {
        if (token->Type == TOKEN_START_TAG) {
            switch (GetStartTag(token)) {
                case TAG_CAPTION:
                case TAG_COL:
                case TAG_COLGROUP:
                case TAG_TBODY:
                case TAG_TD:
                case TAG_TFOOT:
                case TAG_TH:
                case TAG_THEAD:
                case TAG_TR:
                    if (!HasAnElementInTableScope(TAG_TH) && !HasAnElementInTableScope(TAG_TD)) {
                        //add_parse_error(parser, token);
                        return false;
                    }
                    ReprocessCurrentToken_ = true;
                    return CloseCurrentCell(token);

                default:
                    break;
            }
        } else if (token->Type == TOKEN_END_TAG) {
            switch (ETag tag = GetEndTag(token)) {
                case TAG_TD:
                case TAG_TH:
                    if (!HasAnElementInTableScope(tag)) {
                        //add_parse_error(parser, token);
                        return false;
                    }
                    return CloseTableCell(token, tag);

                case TAG_BODY:
                case TAG_CAPTION:
                case TAG_COL:
                case TAG_COLGROUP:
                case TAG_HTML:
                    //add_parse_error(parser, token);
                    return false;

                case TAG_TABLE:
                case TAG_TBODY:
                case TAG_TFOOT:
                case TAG_THEAD:
                case TAG_TR:
                    if (!HasAnElementInTableScope(tag)) {
                        //add_parse_error(parser, token);
                        return false;
                    }
                    ReprocessCurrentToken_ = true;
                    return CloseCurrentCell(token);

                default:
                    break;
            }
        }

        return HandleInBody(token);
    }

    bool TParser::HandleInSelect(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                switch (GetStartTag(token)) {
                    case TAG_HTML:
                        return HandleInBody(token);

                    case TAG_OPTION:
                        if (NodeTagIs(GetCurrentNode(), TAG_OPTION)) {
                            PopCurrentNode();
                        }
                        InsertElementFromToken(token);
                        return true;

                    case TAG_OPTGROUP:
                        if (NodeTagIs(GetCurrentNode(), TAG_OPTION)) {
                            PopCurrentNode();
                        }
                        if (NodeTagIs(GetCurrentNode(), TAG_OPTGROUP)) {
                            PopCurrentNode();
                        }
                        InsertElementFromToken(token);
                        return true;

                    case TAG_SELECT:
                        //add_parse_error(parser, token);
                        if (!HasAnElementInSelectScope(TAG_SELECT)) {
                            return false;
                        }
                        CloseCurrentSelect();
                        return false;

                    case TAG_INPUT:
                    case TAG_KEYGEN:
                    case TAG_TEXTAREA:
                        //add_parse_error(parser, token);
                        if (HasAnElementInSelectScope(TAG_SELECT)) {
                            CloseCurrentSelect();
                            ReprocessCurrentToken_ = true;
                        }
                        return false;

                    case TAG_SCRIPT:
                        return HandleInHead(token);

                    default:
                        break;
                }
                break;
            case TOKEN_END_TAG:
                switch (GetEndTag(token)) {
                    case TAG_OPTGROUP:
                        if (NodeTagIs(GetCurrentNode(), TAG_OPTION) && NodeTagIs(OpenElements_[OpenElements_.size() - 2], TAG_OPTGROUP)) {
                            PopCurrentNode();
                        }
                        if (NodeTagIs(GetCurrentNode(), TAG_OPTGROUP)) {
                            PopCurrentNode();
                            return true;
                        }
                        //add_parse_error(parser, token);
                        return false;

                    case TAG_OPTION:
                        if (NodeTagIs(GetCurrentNode(), TAG_OPTION)) {
                            PopCurrentNode();
                            return true;
                        }
                        //add_parse_error(parser, token);
                        return false;

                    case TAG_SELECT:
                        if (!HasAnElementInSelectScope(TAG_SELECT)) {
                            //add_parse_error(parser, token);
                            return false;
                        }
                        CloseCurrentSelect();
                        return true;

                    default:
                        break;
                }
                break;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_WHITESPACE:
            case TOKEN_CHARACTER:
                ProcessCharacterToken(token);
                return true;
            case TOKEN_NULL:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_EOF:
                return HandleInBody(token);
        }
        //add_parse_error(parser, token);
        return false;
    }

    bool TParser::HandleInSelectInTable(TToken* token) {
        if (token->Type == TOKEN_START_TAG) {
            switch (GetStartTag(token)) {
                case TAG_CAPTION:
                case TAG_TABLE:
                case TAG_TBODY:
                case TAG_TFOOT:
                case TAG_THEAD:
                case TAG_TR:
                case TAG_TD:
                case TAG_TH:
                    //add_parse_error(parser, token);
                    CloseCurrentSelect();
                    ReprocessCurrentToken_ = true;
                    return false;
                default:
                    break;
            }
        } else if (token->Type == TOKEN_END_TAG) {
            switch (GetEndTag(token)) {
                case TAG_CAPTION:
                case TAG_TABLE:
                case TAG_TBODY:
                case TAG_TFOOT:
                case TAG_THEAD:
                case TAG_TR:
                case TAG_TD:
                case TAG_TH:
                    //add_parse_error(parser, token);
                    if (HasAnElementInTableScope(token->v.EndTag)) {
                        CloseCurrentSelect();
                        ReprocessCurrentToken_ = true;
                    }
                    return false;
                default:
                    break;
            }
        }

        return HandleInSelect(token);
    }

    bool TParser::HandleInTemplate(TToken* token) {
        // TODO(jdtang): Implement this.
        Y_UNUSED(token);
        return true;
    }

    bool TParser::HandleAfterBody(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                if (GetStartTag(token) == TAG_HTML) {
                    return HandleInBody(token);
                }
                break;
            case TOKEN_END_TAG:
                if (GetEndTag(token) == TAG_HTML) {
                    // TODO(jdtang): Handle fragment parsing algorithm case.
                    InsertionMode_ = INSERTION_MODE_AFTER_AFTER_BODY;
                    TNode* html = OpenElements_[0];
                    assert(NodeTagIs(html, TAG_HTML));
                    RecordEndOfElement(CurrentToken_, &html->Element);
                    return true;
                }
                break;
            case TOKEN_COMMENT:
                assert(Output_->Root != nullptr);
                AppendCommentNode(Output_->Root, token);
                return true;
            case TOKEN_WHITESPACE:
                return HandleInBody(token);
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
                break;
            case TOKEN_EOF:
                return true;
        }

        //add_parse_error(parser, token);
        InsertionMode_ = INSERTION_MODE_IN_BODY;
        ReprocessCurrentToken_ = true;
        return false;
    }

    bool TParser::HandleInFrameset(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                switch (GetStartTag(token)) {
                    case TAG_HTML:
                        return HandleInBody(token);

                    case TAG_FRAMESET:
                        InsertElementFromToken(token);
                        return true;

                    case TAG_FRAME:
                        InsertElementFromToken(token);
                        PopCurrentNode();
                        return true;

                    case TAG_NOFRAMES:
                        return HandleInHead(token);

                    default:
                        break;
                }
                break;
            case TOKEN_END_TAG:
                if (GetEndTag(token) == TAG_FRAMESET) {
                    if (NodeTagIs(GetCurrentNode(), TAG_HTML)) {
                        //add_parse_error(parser, token);
                        return false;
                    }
                    PopCurrentNode();
                    // TODO(jdtang): Add a condition to ignore this for the fragment parsing
                    // algorithm.
                    if (!NodeTagIs(GetCurrentNode(), TAG_FRAMESET)) {
                        InsertionMode_ = INSERTION_MODE_AFTER_FRAMESET;
                    }
                    return true;
                }
                break;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_WHITESPACE:
                ProcessCharacterToken(token);
                return true;
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
                break;
            case TOKEN_EOF:
                if (!NodeTagIs(GetCurrentNode(), TAG_HTML)) {
                    //add_parse_error(parser, token);
                    return false;
                }
                return true;
        }

        //add_parse_error(parser, token);
        return false;
    }

    bool TParser::HandleAfterFrameset(TToken* token) {
        switch (token->Type) {
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            case TOKEN_START_TAG:
                switch (GetStartTag(token)) {
                    case TAG_HTML:
                        return HandleInBody(token);

                    case TAG_NOFRAMES:
                        return HandleInHead(token);

                    default:
                        break;
                }
                break;
            case TOKEN_END_TAG:
                if (GetEndTag(token) == TAG_HTML) {
                    TNode* html = OpenElements_[0];
                    assert(NodeTagIs(html, TAG_HTML));
                    RecordEndOfElement(CurrentToken_, &html->Element);
                    InsertionMode_ = INSERTION_MODE_AFTER_AFTER_FRAMESET;
                    return true;
                }
                break;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_WHITESPACE:
                ProcessCharacterToken(token);
                return true;
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
                break;
            case TOKEN_EOF:
                return true;
        }

        //add_parse_error(parser, token);
        return false;
    }

    bool TParser::HandleAfterAfterBody(TToken* token) {
        switch (token->Type) {
            case TOKEN_COMMENT:
                AppendCommentNode(GetDocumentNode(), token);
                return true;
            case TOKEN_DOCTYPE:
            case TOKEN_WHITESPACE:
                return HandleInBody(token);
            case TOKEN_START_TAG:
                if (GetStartTag(token) == TAG_HTML) {
                    return HandleInBody(token);
                }
                break;
            case TOKEN_END_TAG:
            case TOKEN_CHARACTER:
            case TOKEN_NULL:
                break;
            case TOKEN_EOF:
                return true;
        }

        //add_parse_error(parser, token);
        InsertionMode_ = INSERTION_MODE_IN_BODY;
        ReprocessCurrentToken_ = true;
        return false;
    }

    bool TParser::HandleAfterAfterFrameset(TToken* token) {
        if (token->Type == TOKEN_COMMENT) {
            AppendCommentNode(GetDocumentNode(), token);
            return true;
        } else if (token->Type == TOKEN_DOCTYPE || token->Type == TOKEN_WHITESPACE || TagIs(token, kStartTag, TAG_HTML)) {
            return HandleInBody(token);
        } else if (token->Type == TOKEN_EOF) {
            return true;
        } else if (TagIs(token, kStartTag, TAG_NOFRAMES)) {
            return HandleInHead(token);
        } else {
            //add_parse_error(parser, token);
            return false;
        }
    }

    bool TParser::HandleAnyOtherEndTagInBody(const TToken* token) {
        assert(token->Type == TOKEN_END_TAG);
        assert(!OpenElements_.empty());
        assert(NodeTagIs(OpenElements_[0], TAG_HTML));
        const ETag end_tag = token->v.EndTag;

        // Walk up the stack of open elements until we find one that either:
        // a) Matches the tag name we saw
        // b) Is in the "special" category.
        // If we see a), implicitly close everything up to and including it.  If we
        // see b), then record a parse error, don't close anything (except the
        // implied end tags) and ignore the end tag token.
        for (int i = OpenElements_.size(); --i >= 0;) {
            const TNode* node = OpenElements_[i];
            if (node->Element.TagNamespace == NAMESPACE_HTML && NodeTagIs(node, end_tag)) {
                GenerateImpliedEndTags(end_tag);
                // TODO(jdtang): Do I need to add a parse error here?  The condition in
                // the spec seems like it's the inverse of the loop condition above, and
                // so would never fire.
                while (node != PopCurrentNode()) {
                } // Pop everything.
                return true;
            } else if (IsSpecialNode(node)) {
                //add_parse_error(parser, token);
                return false;
            }
        }
        // <html> is in the special category, so we should never get here.
        assert(0);
        return false;
    }

    bool TParser::HandleHtmlContent(TToken* token) {
        switch (InsertionMode_) {
            case INSERTION_MODE_INITIAL:
                return HandleInitial(token);
            case INSERTION_MODE_BEFORE_HTML:
                return HandleBeforeHtml(token);
            case INSERTION_MODE_BEFORE_HEAD:
                return HandleBeforeHead(token);
            case INSERTION_MODE_IN_HEAD:
                return HandleInHead(token);
            case INSERTION_MODE_IN_HEAD_NOSCRIPT:
                return HandleInHeadNoscript(token);
            case INSERTION_MODE_AFTER_HEAD:
                return HandleAfterHead(token);
            case INSERTION_MODE_IN_BODY:
                return HandleInBody(token);
            case INSERTION_MODE_TEXT:
                return HandleText(token);
            case INSERTION_MODE_IN_TABLE:
                return HandleInTable(token);
            case INSERTION_MODE_IN_TABLE_TEXT:
                return HandleInTableText(token);
            case INSERTION_MODE_IN_CAPTION:
                return HandleInCaption(token);
            case INSERTION_MODE_IN_COLUMN_GROUP:
                return HandleInColumnGroup(token);
            case INSERTION_MODE_IN_TABLE_BODY:
                return HandleInTableBody(token);
            case INSERTION_MODE_IN_ROW:
                return HandleInRow(token);
            case INSERTION_MODE_IN_CELL:
                return HandleInCell(token);
            case INSERTION_MODE_IN_SELECT:
                return HandleInSelect(token);
            case INSERTION_MODE_IN_SELECT_IN_TABLE:
                return HandleInSelectInTable(token);
            case INSERTION_MODE_IN_TEMPLATE:
                return HandleInTemplate(token);
            case INSERTION_MODE_AFTER_BODY:
                return HandleAfterBody(token);
            case INSERTION_MODE_IN_FRAMESET:
                return HandleInFrameset(token);
            case INSERTION_MODE_AFTER_FRAMESET:
                return HandleAfterFrameset(token);
            case INSERTION_MODE_AFTER_AFTER_BODY:
                return HandleAfterAfterBody(token);
            case INSERTION_MODE_AFTER_AFTER_FRAMESET:
                return HandleAfterAfterFrameset(token);
        }

        return false;
    }

    bool TParser::HandleInForeignContent(TToken* token) {
        switch (token->Type) {
            case TOKEN_NULL:
                //add_parse_error(parser, token);
                token->Type = TOKEN_CHARACTER;
                token->v.Character = TByteIterator::ReplacementChar();
                ProcessCharacterToken(token);
                return false;
            case TOKEN_WHITESPACE:
                ProcessCharacterToken(token);
                return true;
            case TOKEN_CHARACTER:
                ProcessCharacterToken(token);
                FramesetOk_ = false;
                return true;
            case TOKEN_COMMENT:
                AppendCommentNode(GetCurrentNode(), token);
                return true;
            case TOKEN_DOCTYPE:
                //add_parse_error(parser, token);
                return false;
            default:
                // Fall through to the if-statements below.
                break;
        }
        // Order matters for these clauses.
        if (TagIn(token, kStartTag, TAG_B, TAG_BIG,
                  TAG_BLOCKQUOTE, TAG_BODY, TAG_BR,
                  TAG_CENTER, TAG_CODE, TAG_DD, TAG_DIV,
                  TAG_DL, TAG_DT, TAG_EM, TAG_EMBED,
                  TAG_H1, TAG_H2, TAG_H3, TAG_H4,
                  TAG_H5, TAG_H6, TAG_HEAD, TAG_HR,
                  TAG_I, TAG_IMG, TAG_LI, TAG_LISTING,
                  TAG_MENU, TAG_META, TAG_NOBR, TAG_OL,
                  TAG_P, TAG_PRE, TAG_RUBY, TAG_S,
                  TAG_SMALL, TAG_SPAN, TAG_STRONG,
                  TAG_STRIKE, TAG_SUB, TAG_SUP,
                  TAG_TABLE, TAG_TT, TAG_U, TAG_UL,
                  TAG_VAR) ||
            (TagIs(token, kStartTag, TAG_FONT) && (TokenHasAttribute(token, "color") ||
                                                   TokenHasAttribute(token, "face") ||
                                                   TokenHasAttribute(token, "size")))) {
            //add_parse_error(parser, token);
            do {
                PopCurrentNode();
            } while (!(IsMathmlIntegrationPoint(GetCurrentNode()) ||
                       IsHtmlIntegrationPoint(GetCurrentNode()) ||
                       GetCurrentNode()->Element.TagNamespace == NAMESPACE_HTML));
            ReprocessCurrentToken_ = true;
            return false;
        } else if (token->Type == TOKEN_START_TAG) {
            const ENamespace current_namespace = GetCurrentNode()->Element.TagNamespace;
            if (current_namespace == NAMESPACE_MATHML) {
                //adjust_mathml_attributes(parser, token);
            }
            if (current_namespace == NAMESPACE_SVG) {
                // Tag adjustment is left to the gumbo_normalize_svg_tagname helper
                // function.

                //adjust_svg_attributes(parser, token);
            }
            //adjust_foreign_attributes(parser, token);
            InsertForeignElement(token, current_namespace);
            if (token->v.StartTag.IsSelfClosing) {
                PopCurrentNode();
            }
            return true;
            // </script> tags are handled like any other end tag, putting the script's
            // text into a text node child and closing the current node.
        } else {
            assert(token->Type == TOKEN_END_TAG);
            TNode* node = GetCurrentNode();
            assert(node != nullptr);
            TStringPiece token_tagname = GetTagFromOriginalText(token->OriginalText);
            TStringPiece node_tagname = GetTagFromOriginalText(node->Element.OriginalTag);

            bool is_success = true;
            if (node_tagname.Length != token_tagname.Length || strnicmp(node_tagname.Data, token_tagname.Data, node_tagname.Length) != 0) {
                //add_parse_error(parser, token);
                is_success = false;
            }
            for (int i = OpenElements_.size() - 1; i > 0;) {
                // Here we move up the stack until we find an HTML element (in which
                // case we do nothing) or we find the element that we're about to
                // close (in which case we pop everything we've seen until that
                // point.)
                if (node_tagname.Length == token_tagname.Length && strnicmp(node_tagname.Data, token_tagname.Data, node_tagname.Length) == 0) {
                    while (PopCurrentNode() != node) {
                        // Pop all the nodes below the current one.  Node is guaranteed to
                        // be an element on the stack of open elements (set below), so
                        // this loop is guaranteed to terminate.
                    }
                    return is_success;
                }
                --i;
                node = OpenElements_[i];
                if (node->Element.TagNamespace == NAMESPACE_HTML) {
                    // Must break before gumbo_tag_from_original_text to avoid passing
                    // parser-inserted nodes through.
                    break;
                }
                node_tagname = GetTagFromOriginalText(node->Element.OriginalTag);
            }
            assert(node->Element.TagNamespace == NAMESPACE_HTML);
            // We can't call handle_token directly because the current node is still in
            // the SVG namespace, so it would re-enter this and result in infinite
            // recursion.
            return HandleHtmlContent(token) && is_success;
        }
    }
}
