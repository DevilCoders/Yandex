#pragma once

#include "ascii.h"
#include "insertion_mode.h"
#include "options.h"
#include "output.h"
#include "tokenizer.h"

#include <util/generic/vector.h>

namespace NHtml5 {
    class TParser {
    public:
        TParser(const TParserOptions& opts, const char* text, size_t text_length);

        void Parse(TOutput* output);

    private:
        // http://www.whatwg.org/specs/web-apps/current-work/complete/parsing.html#reconstruct-the-active-formatting-elements
        void AddFormattingElement(TNode* node);

        // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-end.html#an-introduction-to-error-handling-and-strange-cases-in-the-parser
        // Also described in the "in body" handling for end formatting tags.
        bool AdoptionAgencyAlgorithm(const TToken* token, ETag closing_tag);

        void AppendCommentNode(TNode* node, const TToken* token);

        // Appends a node to the end of its parent, setting the "parent" and
        // "index_within_parent" fields appropriately.
        void AppendNode(TNode* parent, TNode* node);

        void ClearActiveFormattingElements();

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#clear-the-stack-back-to-a-table-context
        void ClearStackToTableContext();

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#clear-the-stack-back-to-a-table-body-context
        void ClearStackToTableBodyContext();

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#clear-the-stack-back-to-a-table-row-context
        void ClearStackToTableRowContext();

        // Clones attributes, tags, etc. of a node, but does not copy the content.  The
        // clone shares no structure with the original node: all owned strings and
        // values are fresh copies.
        TNode* CloneElementNode(const TNode* node, EParseFlags reason);

        // Common steps from "in caption" insertion mode.
        bool CloseCaption();

        // This factors out the clauses relating to "act as if an end tag token with tag
        // name "table" had been seen.  Returns true if there's a table element in table
        // scope which was successfully closed, false if not and the token should be
        // ignored.  Does not add parse errors; callers should handle that.
        bool CloseTable();

        // This factors out the clauses relating to "act as if an end tag token with tag
        // name `cell_tag` had been seen".
        bool CloseTableCell(const TToken* token, ETag cell_tag);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#close-the-cell
        // This holds the logic to determine whether we should close a <td> or a <th>.
        bool CloseCurrentCell(const TToken* token);

        // This factors out the "act as if an end tag of tag name 'select' had been
        // seen" clause of the spec, since it's referenced in several places.  It pops
        // all nodes from the stack until the current <select> has been closed, then
        // resets the insertion mode appropriately.
        void CloseCurrentSelect();

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#the-initial-insertion-mode
        EQuirksMode ComputeQuirksMode(const TTokenDocType* doctype);

        // Counts the number of open formatting elements in the list of active
        // formatting elements (after the last active scope marker) that have a specific
        // tag.  If this is > 0, then earliest_matching_index will be filled in with the
        // index of the first such element.
        int CountFormattingElementsOfTag(const TNode* desired_node, int* earliest_matching_index);

        // Creates a parser-inserted element in the HTML namespace and returns it.
        TNode* CreateElement(ETag tag);

        // Constructs an element from the given start tag token.
        TNode* CreateElementFromToken(const TToken* token, ENamespace tag_namespace);

        TNode* CreateNode(ENodeType type);

        // Returns true if there's an anchor tag in the list of active formatting
        // elements, and fills in its index if so.
        bool FindLastAnchorIndex(int* anchor_index) const;

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#foster-parenting
        void FosterParentElement(TNode* node);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#generate-implied-end-tags
        // "exception" is the "element to exclude from the process" listed in the spec.
        // Pass GUMBO_TAG_LAST to not exclude any of them.
        void GenerateImpliedEndTags(ETag exception);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/parsing.html#reset-the-insertion-mode-appropriately
        // This is a helper function that returns the appropriate insertion mode instead
        // of setting it.  Returns GUMBO_INSERTION_MODE_INITIAL as a sentinel value to
        // indicate that there is no appropriate insertion mode, and the loop should
        // continue.
        EInsertionMode GetAppropriateInsertionMode(const TNode* node, bool is_last) const;

        TNode* GetDocumentNode();

        // Returns the node at the bottom of the stack of open elements, or NULL if no
        // elements have been added yet.
        TNode* GetCurrentNode() const;

        // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#has-an-element-in-scope
        bool HasAnElementInScope(ETag tag) const;

        bool HasAnElementInScopeWithHeadingTags() const;

        // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#has-an-element-in-button-scope
        bool HasAnElementInButtonScope(ETag tag) const;

        // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#has-an-element-in-list-item-scope
        bool HasAnElementInListScope(ETag tag) const;

        // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#has-an-element-in-select-scope
        bool HasAnElementInSelectScope(ETag tag) const;

        // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#has-an-element-in-table-scope
        bool HasAnElementInTableScope(ETag tag) const;

        // Like "has an element in scope", but for the specific case of looking for a
        // unique target node, not for any node with a given tag name.  This duplicates
        // much of the algorithm from has_an_element_in_specific_scope because the
        // predicate is different when checking for an exact node, and it's easier &
        // faster just to duplicate the code for this one case than to try and
        // parameterize it.
        bool HasNodeInScope(const TNode* node) const;

        // Implicitly closes currently open tags until it reaches an element with the
        // specified tag name.  If the elements closed are in the set handled by
        // generate_implied_end_tags, this is normal operation and this function returns
        // true.  Otherwise, a parse error is recorded and this function returns false.
        bool ImplicitlyCloseTags(ETag target);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#insert-an-html-element
        void InsertElement(TNode* node, bool is_reconstructing_formatting_elements);

        // Convenience method that combines create_element_from_token and
        // insert_element, inserting the generated element directly into the current
        // node.  Returns the node inserted.
        TNode* InsertElementFromToken(const TToken* token);

        // Convenience method that combines create_element and insert_element, inserting
        // a parser-generated element of a specific tag type.  Returns the node
        // inserted.
        TNode* InsertElementOfTagType(ETag tag, EParseFlags reason);

        // Convenience method for creating foreign namespaced element.  Returns the node
        // inserted.
        TNode* InsertForeignElement(const TToken* token, ENamespace tag_namespace);

        // Inserts a node at the specified index within its parent, updating the
        // "parent" and "index_within_parent" fields of it and all its siblings.
        void InsertNode(TNode* parent, size_t index, TNode* node);

        // http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#html-integration-point
        bool IsHtmlIntegrationPoint(const TNode* node) const;

        // http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#mathml-text-integration-point
        bool IsMathmlIntegrationPoint(const TNode* node) const;

        bool IsOpenElement(const TNode* node) const;

        // The list of nodes in the "special" category:
        // http://www.whatwg.org/specs/web-apps/current-work/complete/parsing.html#special
        bool IsSpecialNode(const TNode* node) const;

        bool MaybeAddDoctypeError(const TToken* token);

        void MaybeFlushTextNodeBuffer();

        // Merge text from source node with text in destination node
        // and destroy source.
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#insert-a-character
        bool MaybeMergeNodes(TNode* destination, TNode* source) const;

        // Convenience function to encapsulate the logic for closing <li> or <dd>/<dt>
        // tags.  Pass true to is_li for handling <li> tags, false for <dd> and <dt>.
        void MaybeImplicitlyCloseListTag(bool is_li);

        // If the stack of open elements has a <p> tag in button scope, this acts as if
        // a </p> tag was encountered, implicitly closing tags.  Returns false if a
        // parse error occurs.  This is a convenience function because this particular
        // clause appears several times in the spec.
        bool MaybeImplicitlyClosePTag();

        void MergeAttributes(TToken* token, TNode* node) const;

        TNode* PopCurrentNode();

        // Try insert character or text token to current text node buffer.
        // If it breaks text entirety flushs current buffer state and initialize new buffer.
        void ProcessCharacterToken(const TToken* token);

        // "Reconstruct active formatting elements" part of the spec.
        // This implementation is based on the html5lib translation from the mess of
        // GOTOs in the spec to reasonably structured programming.
        // http://code.google.com/p/html5lib/source/browse/python/html5lib/treebuilders/_base.py
        void ReconstructActiveFormattingElements();

        void RecordEndOfElement(const TToken* current_token, TElement* element) const;

        void RemoveFromParent(TNode* node);

        // This performs the actual "reset the insertion mode" loop.
        void ResetInsertionModeAppropriately();

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#generic-rcdata-element-parsing-algorithm
        void RunGenericParsingAlgorithm(TToken* token, ETokenizerState lexer_state);

    private:
        bool HandleInitial(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#the-before-html-insertion-mode
        bool HandleBeforeHtml(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#the-before-head-insertion-mode
        bool HandleBeforeHead(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-inhead
        bool HandleInHead(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-inheadnoscript
        bool HandleInHeadNoscript(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#the-after-head-insertion-mode
        bool HandleAfterHead(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-inbody
        bool HandleInBody(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-incdata
        bool HandleText(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-intable
        bool HandleInTable(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-intabletext
        bool HandleInTableText(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-incaption
        bool HandleInCaption(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-incolgroup
        bool HandleInColumnGroup(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-intbody
        bool HandleInTableBody(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-intr
        bool HandleInRow(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-intd
        bool HandleInCell(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-inselect
        bool HandleInSelect(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-inselectintable
        bool HandleInSelectInTable(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#parsing-main-intemplate
        bool HandleInTemplate(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-afterbody
        bool HandleAfterBody(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-inframeset
        bool HandleInFrameset(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-afterframeset
        bool HandleAfterFrameset(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#the-after-after-body-insertion-mode
        bool HandleAfterAfterBody(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#the-after-after-frameset-insertion-mode
        bool HandleAfterAfterFrameset(TToken* token);

        // Any other end tag described at
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#parsing-main-inbody
        bool HandleAnyOtherEndTagInBody(const TToken* token);

        bool HandleHtmlContent(TToken* token);

        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#parsing-main-inforeign
        bool HandleInForeignContent(TToken* token);

        void PushButtonFlags(const TNode* node);
        void PopButtonFlags();
        void RebuildButtonFlags();
        void CheckAndRebuildButtonFlags(ETag tag);

    private:
        // http://www.whatwg.org/specs/web-apps/current-work/complete/tokenization.html#insert-a-character
        struct TTextNodeBufferState {
            // The accumulated text to be inserted into the current text node.
            TCodepointBuffer<TByteIterator> Buffer;

            // A pointer to the original text represented by this text node.  Note that
            // because of foster parenting and other strange DOM manipulations, this may
            // include other non-text HTML tags in it; it is defined as the span of
            // original text from the first character in this text node to the last
            // character in this text node.
            const char* StartOriginalText;

            // Contains the poiter to the end of current text block.
            // Consistency condition: text blocks be unparted.
            const char* EndOriginalText;

            // The type of node that will be inserted (TEXT or WHITESPACE).
            ENodeType Type;

            // Reset buffer state to default.
            inline void Clear();

            inline bool Empty() const;
        };

        const TParserOptions Options_;

        TTokenizer<TByteIterator> Tokenizer_;

        // Output for the parse.
        TOutput* Output_;

        // http://www.whatwg.org/specs/web-apps/current-work/complete/parsing.html#insertion-mode
        EInsertionMode InsertionMode_;

        // Used for run_generic_parsing_algorithm, which needs to switch back to the
        // original insertion mode at its conclusion.
        EInsertionMode OriginalInsertionMode_;

        // http://www.whatwg.org/specs/web-apps/current-work/complete/parsing.html#the-stack-of-open-elements
        TVector<TNode*> OpenElements_;
        TVector<bool> ClosePTagImplicitlyFlags_;

        // http://www.whatwg.org/specs/web-apps/current-work/complete/parsing.html#the-list-of-active-formatting-elements
        TVector<TNode*> ActiveFormattingElements_;

        // http://www.whatwg.org/specs/web-apps/current-work/complete/parsing.html#the-element-pointers
        TNode* HeadElement_;
        TNode* FormElement_;

        // The flag for when the spec says "Reprocess the current token in..."
        bool ReprocessCurrentToken_;

        // The "frameset-ok" flag from the spec.
        bool FramesetOk_;

        // The flag for "If the next token is a LINE FEED, ignore that token...".
        bool IgnoreNextLinefeed_;

        // The flag for "whenever a node would be inserted into the current node, it
        // must instead be foster parented".  This is used for misnested table
        // content, which needs to be handled according to "in body" rules yet foster
        // parented outside of the table.
        // It would perhaps be more explicit to have this as a parameter to
        // handle_in_body and insert_element, but given how special-purpose this is
        // and the number of call-sites that would need to take the extra parameter,
        // it's easier just to have a state flag.
        bool FosterParentInsertions_;

        // The accumulated text node buffer state.
        TTextNodeBufferState TextNode_;

        // The current token.
        TToken* CurrentToken_;

        // The way that the spec is written, the </body> and </html> tags are *always*
        // implicit, because encountering one of those tokens merely switches the
        // insertion mode out of "in body".  So we have individual state flags for
        // those end tags that are then inspected by pop_current_node when the <body>
        // and <html> nodes are popped to set the GUMBO_INSERTION_IMPLICIT_END_TAG
        // flag appropriately.
        bool ClosedBodyTag_;
        bool ClosedHtmlTag_;

        std::function<TNode**(size_t)> CreateNodeVector_;
        std::function<TAttribute*(size_t)> CreateAttributeVector_;
    };

}
