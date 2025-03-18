#include "parser.h"
#include "output.h"
#include "twitter.h"
#include "text_normalize.h"

#include <library/cpp/html/face/event.h>
#include <library/cpp/html/face/noindex.h>
#include <library/cpp/html/face/onchunk.h>
#include <library/cpp/html/face/parstypes.h>
#include <library/cpp/html/spec/tagstrings.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/buffer.h>
#include <util/stream/output.h>
#include <utility>

namespace NHtml5 {
    namespace {
        const struct {
            ETag Src;
            HT_TAG Dst;
        } remap[] =
            {
                {TAG_UNKNOWN, HT_any},
                {
                    TAG_HTML,
                    HT_HTML,
                },
                {
                    TAG_HEAD,
                    HT_HEAD,
                },
                {TAG_TITLE, HT_TITLE},
                {
                    TAG_BASE,
                    HT_BASE,
                },
                {
                    TAG_LINK,
                    HT_LINK,
                },
                {
                    TAG_META,
                    HT_META,
                },
                {
                    TAG_STYLE,
                    HT_STYLE,
                },
                {
                    TAG_SCRIPT,
                    HT_SCRIPT,
                },
                {
                    TAG_NOSCRIPT,
                    HT_NOSCRIPT,
                },
                {
                    TAG_TEMPLATE,
                    HT_TEMPLATE,
                },
                {
                    TAG_BODY,
                    HT_BODY,
                },
                {
                    TAG_ARTICLE,
                    HT_ARTICLE,
                },
                {
                    TAG_SECTION,
                    HT_SECTION,
                },
                {
                    TAG_NAV,
                    HT_NAV,
                },
                {
                    TAG_ASIDE,
                    HT_ASIDE,
                },
                {
                    TAG_H1,
                    HT_H1,
                },
                {
                    TAG_H2,
                    HT_H2,
                },
                {
                    TAG_H3,
                    HT_H3,
                },
                {
                    TAG_H4,
                    HT_H4,
                },
                {
                    TAG_H5,
                    HT_H5,
                },
                {
                    TAG_H6,
                    HT_H6,
                },
                {
                    TAG_HGROUP,
                    HT_HGROUP,
                },
                {
                    TAG_HEADER,
                    HT_HEADER,
                },
                {
                    TAG_FOOTER,
                    HT_FOOTER,
                },
                {
                    TAG_ADDRESS,
                    HT_ADDRESS,
                },
                {
                    TAG_P,
                    HT_P,
                },
                {
                    TAG_HR,
                    HT_HR,
                },
                {
                    TAG_PRE,
                    HT_PRE,
                },
                {
                    TAG_BLOCKQUOTE,
                    HT_BLOCKQUOTE,
                },
                {
                    TAG_OL,
                    HT_OL,
                },
                {
                    TAG_UL,
                    HT_UL,
                },
                {
                    TAG_LI,
                    HT_LI,
                },
                {
                    TAG_DL,
                    HT_DL,
                },
                {
                    TAG_DT,
                    HT_DT,
                },
                {
                    TAG_DD,
                    HT_DD,
                },
                {
                    TAG_FIGURE,
                    HT_FIGURE,
                },
                {
                    TAG_FIGCAPTION,
                    HT_FIGCAPTION,
                },
                {
                    TAG_MAIN,
                    HT_MAIN,
                },
                {
                    TAG_DIV,
                    HT_DIV,
                },
                {
                    TAG_A,
                    HT_A,
                },
                {
                    TAG_EM,
                    HT_EM,
                },
                {
                    TAG_STRONG,
                    HT_STRONG,
                },
                {
                    TAG_SMALL,
                    HT_SMALL,
                },
                {
                    TAG_S,
                    HT_S,
                },
                {
                    TAG_CITE,
                    HT_CITE,
                },
                {
                    TAG_Q,
                    HT_Q,
                },
                {
                    TAG_DFN,
                    HT_DFN,
                },
                {
                    TAG_ABBR,
                    HT_ABBR,
                },
                {
                    TAG_DATA,
                    HT_DATA,
                },
                {
                    TAG_TIME,
                    HT_TIME,
                },
                {
                    TAG_CODE,
                    HT_CODE,
                },
                {
                    TAG_VAR,
                    HT_VAR,
                },
                {
                    TAG_SAMP,
                    HT_SAMP,
                },
                {
                    TAG_KBD,
                    HT_KBD,
                },
                {
                    TAG_SUB,
                    HT_SUB,
                },
                {
                    TAG_SUP,
                    HT_SUP,
                },
                {
                    TAG_I,
                    HT_I,
                },
                {
                    TAG_B,
                    HT_B,
                },
                {
                    TAG_U,
                    HT_U,
                },
                {
                    TAG_MARK,
                    HT_MARK,
                },
                {
                    TAG_RUBY,
                    HT_RUBY,
                },
                {
                    TAG_RT,
                    HT_RT,
                },
                {
                    TAG_RP,
                    HT_RP,
                },
                {
                    TAG_BDI,
                    HT_BDI,
                },
                {
                    TAG_BDO,
                    HT_BDO,
                },
                {
                    TAG_SPAN,
                    HT_SPAN,
                },
                {
                    TAG_BR,
                    HT_BR,
                },
                {
                    TAG_WBR,
                    HT_WBR,
                },
                {
                    TAG_INS,
                    HT_INS,
                },
                {
                    TAG_DEL,
                    HT_DEL,
                },
                {
                    TAG_IMAGE,
                    HT_IMAGE,
                },
                {
                    TAG_IMG,
                    HT_IMG,
                },
                {
                    TAG_IFRAME,
                    HT_IFRAME,
                },
                {
                    TAG_EMBED,
                    HT_EMBED,
                },
                {
                    TAG_OBJECT,
                    HT_OBJECT,
                },
                {
                    TAG_PARAM,
                    HT_PARAM,
                },
                {
                    TAG_VIDEO,
                    HT_VIDEO,
                },
                {
                    TAG_AUDIO,
                    HT_AUDIO,
                },
                {
                    TAG_SOURCE,
                    HT_SOURCE,
                },
                {
                    TAG_TRACK,
                    HT_TRACK,
                },
                {
                    TAG_CANVAS,
                    HT_CANVAS,
                },
                {
                    TAG_MAP,
                    HT_MAP,
                },
                {
                    TAG_AREA,
                    HT_AREA,
                },
                {
                    TAG_MATH,
                    HT_MATH,
                },
                {
                    TAG_MI,
                    HT_MI,
                },
                {
                    TAG_MO,
                    HT_MO,
                },
                {
                    TAG_MN,
                    HT_MN,
                },
                {
                    TAG_MS,
                    HT_MS,
                },
                {
                    TAG_MTEXT,
                    HT_MTEXT,
                },
                {
                    TAG_MGLYPH,
                    HT_MGLYPH,
                },
                {
                    TAG_MALIGNMARK,
                    HT_MALIGNMARK,
                },
                {
                    TAG_ANNOTATION_XML,
                    HT_any,
                },
                {
                    TAG_SVG,
                    HT_SVG,
                },
                {
                    TAG_FOREIGNOBJECT,
                    HT_FOREIGNOBJECT,
                },
                {
                    TAG_DESC,
                    HT_DESC,
                },
                {
                    TAG_TABLE,
                    HT_TABLE,
                },
                {
                    TAG_CAPTION,
                    HT_CAPTION,
                },
                {
                    TAG_COLGROUP,
                    HT_COLGROUP,
                },
                {
                    TAG_COL,
                    HT_COL,
                },
                {
                    TAG_TBODY,
                    HT_TBODY,
                },
                {
                    TAG_THEAD,
                    HT_THEAD,
                },
                {
                    TAG_TFOOT,
                    HT_TFOOT,
                },
                {
                    TAG_TR,
                    HT_TR,
                },
                {
                    TAG_TD,
                    HT_TD,
                },
                {
                    TAG_TH,
                    HT_TH,
                },
                {
                    TAG_DIALOG,
                    HT_DIALOG,
                },
                {
                    TAG_FORM,
                    HT_FORM,
                },
                {
                    TAG_FIELDSET,
                    HT_FIELDSET,
                },
                {
                    TAG_LEGEND,
                    HT_LEGEND,
                },
                {
                    TAG_LABEL,
                    HT_LABEL,
                },
                {
                    TAG_INPUT,
                    HT_INPUT,
                },
                {
                    TAG_BUTTON,
                    HT_BUTTON,
                },
                {
                    TAG_SELECT,
                    HT_SELECT,
                },
                {
                    TAG_DATALIST,
                    HT_DATALIST,
                },
                {
                    TAG_OPTGROUP,
                    HT_OPTGROUP,
                },
                {
                    TAG_OPTION,
                    HT_OPTION,
                },
                {
                    TAG_TEXTAREA,
                    HT_TEXTAREA,
                },
                {
                    TAG_KEYGEN,
                    HT_KEYGEN,
                },
                {
                    TAG_OUTPUT,
                    HT_OUTPUT,
                },
                {
                    TAG_PROGRESS,
                    HT_PROGRESS,
                },
                {
                    TAG_METER,
                    HT_METER,
                },
                {
                    TAG_DETAILS,
                    HT_DETAILS,
                },
                {
                    TAG_SUMMARY,
                    HT_SUMMARY,
                },
                {
                    TAG_MENU,
                    HT_MENU,
                },
                {
                    TAG_MENUITEM,
                    HT_MENUITEM,
                },
                {
                    TAG_APPLET,
                    HT_APPLET,
                },
                {
                    TAG_ACRONYM,
                    HT_ACRONYM,
                },
                {
                    TAG_BGSOUND,
                    HT_BGSOUND,
                },
                {
                    TAG_DIR,
                    HT_DIR,
                },
                {
                    TAG_FRAME,
                    HT_FRAME,
                },
                {
                    TAG_FRAMESET,
                    HT_FRAMESET,
                },
                {
                    TAG_NOFRAMES,
                    HT_NOFRAMES,
                },
                {
                    TAG_NOINDEX,
                    HT_NOINDEX,
                },
                {
                    TAG_ISINDEX,
                    HT_ISINDEX,
                },
                {
                    TAG_LISTING,
                    HT_LISTING,
                },
                {
                    TAG_XMP,
                    HT_XMP,
                },
                {
                    TAG_NEXTID,
                    HT_NEXTID,
                },
                {
                    TAG_NOEMBED,
                    HT_NOEMBED,
                },
                {
                    TAG_PLAINTEXT,
                    HT_PLAINTEXT,
                },
                {
                    TAG_RB,
                    HT_RB,
                },
                {
                    TAG_STRIKE,
                    HT_STRIKE,
                },
                {
                    TAG_BASEFONT,
                    HT_BASEFONT,
                },
                {
                    TAG_BIG,
                    HT_BIG,
                },
                {
                    TAG_BLINK,
                    HT_BLINK,
                },
                {
                    TAG_CENTER,
                    HT_CENTER,
                },
                {
                    TAG_FONT,
                    HT_FONT,
                },
                {
                    TAG_MARQUEE,
                    HT_MARQUEE,
                },
                {
                    TAG_MULTICOL,
                    HT_MULTICOL,
                },
                {
                    TAG_NOBR,
                    HT_NOBR,
                },
                {
                    TAG_SPACER,
                    HT_SPACER,
                },
                {
                    TAG_TT,
                    HT_TT,
                },
                {
                    TAG_COMMENT,
                    HT_COMMENT,
                },
                {
                    TAG_XML,
                    HT_XML,
                },
                {
                    TAG_AMP_VK,
                    HT_AMP_VK,
                },
                {TAG_LAST, HT_any}};

        inline const NHtml::TTag* GetYaTag(const ETag tag) {
            Y_ASSERT(remap[tag].Src == tag);
            return &(NHtml::FindTag(remap[tag].Dst));
        }

        inline ETagNamespace GetYaTagNS(const ENamespace ns) {
            switch (ns) {
                case NAMESPACE_HTML:
                    return ETagNamespace::HTML;
                case NAMESPACE_SVG:
                    return ETagNamespace::SVG;
                case NAMESPACE_MATHML:
                    return ETagNamespace::MATHML;
            }
            Y_ASSERT(0);
            return ETagNamespace::HTML;
        }

        class TParserState {
        public:
            inline TParserState(const TParserOptions& opts)
                : Opts_(opts)
                , LastTag_(HT_any)
                , InBody_(false)
                , InLit_(false)
                , IsTitleMet_(false)
            {
                Format_.IrregTag = IRREG_none;
                Format_.W0Count = 0;
                Format_.PreCount = 0;
                Format_.BCount = 0;
            }

            inline void UpdateOnTagStart(const NHtml::TTag* tag, const TNode* node) {
                if (tag->is(HT_w0)) {
                    if (!OverrideW0(tag, node)) {
                        Format_.W0Count++;
                    }
                }
                if (tag->is(HT_pre)) {
                    Format_.PreCount++;
                }
                if (tag->is(HT_w1)) {
                    Format_.BCount++;
                }
                if (tag->is(HT_lit)) {
                    InLit_ = true;
                }
                if (!tag->is(HT_irreg)) {
                    LastTag_ = tag->id();
                }
                Format_.IrregTag = TIrregTag(Format_.IrregTag | tag->irreg);

                if (tag->id() == HT_BODY) {
                    InBody_ = true;
                }
            }

            inline void UpdateOnTagEnd(const NHtml::TTag* tag, const TNode* node) {
                if (tag->id() == HT_TITLE) {
                    IsTitleMet_ = true;
                }
                if (tag->id() == HT_BODY) {
                    InBody_ = false;
                }
                if (tag->is(HT_lit)) {
                    InLit_ = false;
                }

                // End format
                if (tag->is(HT_w0)) {
                    if (!OverrideW0(tag, node)) {
                        Y_ASSERT(Format_.W0Count != 0);
                        Format_.W0Count--;
                    }
                }
                if (tag->is(HT_pre)) {
                    Y_ASSERT(Format_.PreCount != 0);
                    Format_.PreCount--;
                }
                if (tag->is(HT_w1)) {
                    Y_ASSERT(Format_.BCount != 0);
                    Format_.BCount--;
                }
                Format_.IrregTag = TIrregTag(Format_.IrregTag & ~tag->irreg);
            }

            inline void UpdateOnText() {
                LastTag_ = HT_any;
            }

            // Used for comments.
            inline void ApplyNoindex(const TNoindexType& type) {
                Y_ASSERT(type.IsNoindex());

                if (type.IsClose()) {
                    Format_.IrregTag = TIrregTag(Format_.IrregTag & ~IRREG_NOINDEX);
                } else {
                    Format_.IrregTag = TIrregTag(Format_.IrregTag | IRREG_NOINDEX);
                }
            }

            inline void SetNodeOptions(const NHtml::TTag* tag, const TNode* node, THtmlChunk* chunk) const {
                if (!tag->id()) {
                    chunk->flags.markup = MARKUP_IGNORED;
                }

                chunk->flags.brk = GetBreakType(tag, node, chunk->GetLexType());
                chunk->flags.weight = GetMarkupWeight();
            }

            inline void SetTextOptions(const NHtml::TTag* parent, THtmlChunk* chunk) const {
                chunk->flags.weight = GetTextWeight(parent);
                chunk->flags.space = GetSpaceMode(parent);
                chunk->Format = Format_.IrregTag;

                // private hack to distinguish noindex text used for language detection
                if ((Format_.IrregTag & IRREG_NOINDEX) && Format_.W0Count == 0) {
                    chunk->flags.atype = 1;
                }
            }

            inline void SetWhitespaceOptions(const NHtml::TTag* parent, THtmlChunk* chunk) const {
                chunk->flags.space = GetSpaceMode(parent);
            }

            inline bool IsInLit() const {
                return InLit_;
            }

        private:
            inline bool CheckASequenceCondition(const TNode* parent, size_t index) const {
                Y_ASSERT(parent->Type == NODE_ELEMENT);
                Y_ASSERT(index < parent->Element.Children.Length);

                if (parent->Element.Children.Data[index]->Type == NODE_ELEMENT && parent->Element.Children.Data[index]->Element.Tag == TAG_A) {
                    return true;
                }

                return false;
            }

            inline BREAK_TYPE GetBreakType(const NHtml::TTag* tag, const TNode* node, HTLEX_TYPE lexType) const {
                switch (tag->id()) {
                    case HT_A:
                        if (lexType == HTLEX_END_TAG) {
                            return BREAK_NONE;
                        }
                        if (node->IndexWithinParent > 0 && CheckASequenceCondition(node->Parent, node->IndexWithinParent - 1)) {
                            return BREAK_WORD;
                        }
                        break;
                    case HT_BODY:
                        if (lexType == HTLEX_START_TAG) {
                            return BREAK_BODY;
                        }
                        break;
                    case HT_BR:
                        // <BR>space*<BR> breaks paragraph, single <BR> breaks word
                        if (LastTag_ == HT_BR)
                            return BREAK_PARAGRAPH;
                        else
                            return BREAK_WORD;
                    case HT_CITE:
                        return BREAK_WORD;
                    case HT_META:
                        if (InBody_) {
                            return BREAK_WORD;
                        }
                        break;
                    default:
                        break;
                }

                if (tag->is(HT_br)) {
                    return BREAK_PARAGRAPH;
                }
                if (tag->is(HT_wbr)) {
                    return BREAK_WORD;
                }
                return BREAK_NONE;
            }

            inline SPACE_MODE GetSpaceMode(const NHtml::TTag* parent) const {
                if (parent->is(HT_lit) || Format_.PreCount)
                    return SPACE_PRESERVE;

                return SPACE_DEFAULT;
            }

            inline TEXT_WEIGHT GetTextWeight(const NHtml::TTag* parent) const {
                if (Opts_.EnableNoindex && (Format_.IrregTag & IRREG_NOINDEX))
                    return WEIGHT_ZERO;

                if (parent->id() == HT_TITLE) {
                    if (IsTitleMet_)
                        return WEIGHT_ZERO; // Kill all additional titles
                    else
                        return WEIGHT_BEST;
                }

                if (Format_.W0Count)
                    return WEIGHT_ZERO;

                if (Format_.BCount)
                    return WEIGHT_HIGH;

                return WEIGHT_NORMAL;
            }

            inline TEXT_WEIGHT GetMarkupWeight() const {
                if (Opts_.EnableNoindex && (Format_.IrregTag & IRREG_NOINDEX))
                    return WEIGHT_ZERO;

                if (Format_.W0Count)
                    return WEIGHT_ZERO;

                return WEIGHT_NORMAL;
            }

            inline bool OverrideW0(const NHtml::TTag* tag, const TNode* node) const {
                switch (tag->id()) {
                    // Content of tag <comment> is visible in all modern browsers.
                    case HT_COMMENT:
                        return true;
                    // See https://st.yandex-team.ru/INDEX-138
                    case HT_OPTION:
                        return true;
                    case HT_OBJECT:
                        // We can lost some part of document content if <object> was not closed explicitly.
                        if (node->ParseFlags & INSERTION_IMPLICIT_END_TAG) {
                            return true;
                        }
                        break;
                    case HT_NOINDEX:
                        if (!Opts_.EnableNoindex) {
                            return true;
                        }
                        break;
                    default:
                        break;
                }
                return false;
            }

        private:
            struct TFormat {
                unsigned W0Count;
                unsigned PreCount;
                unsigned BCount;
                TIrregTag IrregTag;
            };

            TParserOptions Opts_;
            TFormat Format_;
            HT_TAG LastTag_;
            bool InBody_;
            bool InLit_;
            bool IsTitleMet_;
        };

        class TDocumentConstructor {
            template <typename T>
            class TVectorIterator {
            public:
                inline TVectorIterator(const TVectorType<T>& v)
                    : Vec_(v)
                    , Idx_(0)
                {
                }

                inline const T* Item() const {
                    return &Vec_.Data[Idx_];
                }

                inline bool IsValid() const {
                    return Idx_ < Vec_.Length;
                }

                inline void Next() {
                    ++Idx_;
                }

            private:
                const TVectorType<T>& Vec_;
                size_t Idx_;
            };

        public:
            TDocumentConstructor(TOutput* parserOutput, const TStringBuf& url)
                : ParserOutput_(parserOutput)
                , TwitterConverter_(CreateTreeConverter(url, ParserOutput_))
            {
            }

            void Construct(const TParserOptions& opts, IParserResult* result) {
                TParserState state(opts);
                TVector<std::pair<const TNode*, TVectorIterator<TNode*>>>
                    openElements;

                HandleDocument(ParserOutput_->Document, result);
                openElements.push_back(
                    std::make_pair(ParserOutput_->Document, TVectorIterator<TNode*>(ParserOutput_->Document->Document.Children)));

                do {
                    while (true) {
                        const TNode* current = openElements.back().first;
                        TVectorIterator<TNode*>& ci = openElements.back().second;

                        if (!ci.IsValid()) {
                            if (current->Type == NODE_ELEMENT) {
                                HandleEndTag(current, &state, result);
                            }
                            openElements.pop_back();
                            break;
                        }

                        TNode* node = *ci.Item();

                        // Skip all nodes inserted from <isindex>
                        // cause we don't have enough original text pointers
                        // for it.
                        if (node->ParseFlags & INSERTION_FROM_ISINDEX) {
                            ci.Next();
                            continue;
                        }

                        switch (node->Type) {
                            case NODE_DOCUMENT:
                                ci.Next();
                                break;
                            case NODE_ELEMENT:
                                if (TwitterConverter_.Get()) {
                                    TwitterConverter_->MaybeChangeSubtree(node);
                                }

                                HandleStartTag(node, &state, result);
                                ci.Next();
                                openElements.push_back(
                                    std::make_pair(node, TVectorIterator<TNode*>(node->Element.Children)));
                                break;
                            case NODE_TEXT:
                                HandleText(node, &state, result);
                                ci.Next();
                                break;
                            case NODE_COMMENT:
                                HandleComment(node, &state, result);
                                ci.Next();
                                break;
                            case NODE_WHITESPACE:
                                HandleWhitespace(node, &state, result);
                                ci.Next();
                                break;
                        }
                    }
                } while (!openElements.empty());
            }

        private:
            void HandleDocument(const TNode* node, IParserResult* result) {
                if (node->Document.HasDoctype) {
                    THtmlChunk chunk(PARSED_MARKUP);

                    chunk.flags.apos = (i8)HTLEX_MD;
                    chunk.flags.markup = MARKUP_IGNORED;
                    chunk.text = node->Document.OriginalText.Data;
                    chunk.leng = node->Document.OriginalText.Length;

                    result->OnHtmlChunk(chunk);
                }
            }

            void HandleStartTag(const TNode* node, TParserState* state, IParserResult* result) {
                THtmlChunk chunk(PARSED_MARKUP);
                const NHtml::TTag* tag = GetYaTag(node->Element.Tag);

                chunk.Tag = tag;
                chunk.Namespace = GetYaTagNS(node->Element.TagNamespace);

                if (node->ParseFlags & INSERTION_IMPLIED) {
                    // Implied markup.
                    chunk.flags.markup = MARKUP_IMPLIED;
                    chunk.text = TagStrings[tag->id()].Open;
                    chunk.leng = TagStrings[tag->id()].OpenLen;
                } else {
                    // Normal markup or cloned or moved nodes.
                    if (node->ParseFlags & ~(INSERTION_IMPLICIT_END_TAG | INSERTION_FOSTER_PARENTED | INSERTION_ADOPTION_AGENCY_MOVED)) {
                        chunk.flags.markup = MARKUP_IMPLIED;
                    } else {
                        chunk.flags.markup = MARKUP_NORMAL;
                    }
                    chunk.text = node->Element.OriginalTag.Data;
                    chunk.leng = node->Element.OriginalTag.Length;
                }

                // Set apos.
                if (chunk.Tag->is(HT_empty))
                    chunk.flags.apos = (i8)HTLEX_EMPTY_TAG;
                else
                    chunk.flags.apos = (i8)HTLEX_START_TAG;

                if (node->Element.Attributes.Length != 0) {
                    const TVectorType<TAttribute>& attrs = node->Element.Attributes;

                    chunk.AttrCount = 0;
                    chunk.Attrs = Attrs_;

                    for (size_t i = 0, len = Min(size_t(attrs.Length), Y_ARRAY_SIZE(Attrs_)); i < len; ++i) {
                        const TAttribute& attr = attrs.Data[i];
                        NHtml::TAttribute* current = &Attrs_[chunk.AttrCount];

                        // Resulting attribute offset can't be negative
                        // because it's will be stored as unsigned integer
                        if (attr.OriginalName.Data < node->Element.OriginalTag.Data ||
                            // Need to ignore merged attributes because it's not deal with serialization
                            attr.OriginalName.Data >= node->Element.OriginalTag.Data + node->Element.OriginalTag.Length) {
                            continue;
                        }
                        if (attr.OriginalName.Length == 0) {
                            continue;
                        }

                        ui8 nsShift = 0;
                        switch (attr.AttrNamespace) {
                            case ATTR_NAMESPACE_NONE:
                                current->Namespace = EAttrNS::NONE;
                                break;
                            case ATTR_NAMESPACE_XLINK:
                                current->Namespace = EAttrNS::XLINK;
                                nsShift = 6;
                                break;
                            case ATTR_NAMESPACE_XML:
                                current->Namespace = EAttrNS::XML;
                                nsShift = 4;
                                break;
                            case ATTR_NAMESPACE_XMLNS:
                                current->Namespace = EAttrNS::XMLNS;
                                nsShift = 6;
                                break;
                        }

                        current->Name.Start = attr.OriginalName.Data - node->Element.OriginalTag.Data + nsShift;
                        current->Name.Leng = attr.OriginalName.Length - nsShift;

                        if (attr.OriginalName.Data == attr.OriginalValue.Data) {
                            current->Value = current->Name;
                            current->Quot = '\0';
                        } else if (attr.OriginalValue.Data[0] == '\'' || attr.OriginalValue.Data[0] == '\"') {
                            current->Value.Start = attr.OriginalValue.Data - node->Element.OriginalTag.Data + 1;
                            current->Value.Leng = attr.OriginalValue.Length - 2;
                            current->Quot = attr.OriginalValue.Data[0];
                        } else {
                            current->Value.Start = attr.OriginalValue.Data - node->Element.OriginalTag.Data;
                            current->Value.Leng = attr.OriginalValue.Length;
                            current->Quot = '\0';
                        }

                        chunk.AttrCount++;
                    }
                }

                state->SetNodeOptions(tag, node, &chunk);
                result->OnHtmlChunk(chunk);
                state->UpdateOnTagStart(tag, node);
            }

            void HandleEndTag(const TNode* node, TParserState* state, IParserResult* result) {
                const NHtml::TTag* tag = GetYaTag(node->Element.Tag);

                state->UpdateOnTagEnd(tag, node);

                if (!tag->is(HT_empty)) {
                    THtmlChunk chunk(PARSED_MARKUP);

                    chunk.Tag = tag;
                    chunk.Namespace = GetYaTagNS(node->Element.TagNamespace);

                    if (node->ParseFlags & (INSERTION_IMPLICIT_END_TAG | INSERTION_RECONSTRUCTED_FORMATTING_ELEMENT)) {
                        // Implied markup.
                        chunk.flags.markup = MARKUP_IMPLIED;
                        chunk.flags.apos = (i8)HTLEX_END_TAG;
                        chunk.text = TagStrings[tag->id()].Close;
                        chunk.leng = TagStrings[tag->id()].CloseLen;
                    } else {
                        // Normal markup.
                        chunk.flags.markup = MARKUP_NORMAL;
                        chunk.flags.apos = (i8)HTLEX_END_TAG;
                        chunk.text = node->Element.OriginalEndTag.Data;
                        chunk.leng = node->Element.OriginalEndTag.Length;
                    }

                    if (!chunk.text || node->ParseFlags & INSERTION_ADOPTION_AGENCY_MOVED)
                        chunk.flags.markup = MARKUP_IMPLIED;

                    state->SetNodeOptions(tag, node, &chunk);
                    result->OnHtmlChunk(chunk);
                }
            }

            void HandleText(const TNode* node, TParserState* state, IParserResult* result) {
                THtmlChunk chunk(PARSED_TEXT);

                chunk.text = node->Text.OriginalText.Data;
                chunk.leng = node->Text.OriginalText.Length;
                chunk.flags.apos = (i8)HTLEX_TEXT;
                chunk.IsCDATA = state->IsInLit();

                state->SetTextOptions(GetYaTag(node->Parent->Element.Tag), &chunk);
                state->UpdateOnText();

                result->OnHtmlChunk(chunk);
            }

            void HandleComment(const TNode* node, TParserState* state, IParserResult* result) {
                THtmlChunk chunk(PARSED_MARKUP);

                chunk.text = node->Text.OriginalText.Data;
                chunk.leng = node->Text.OriginalText.Length;
                chunk.flags.apos = (i8)HTLEX_COMMENT;
                chunk.flags.markup = MARKUP_IGNORED;

                // Check for noindex
                const TNoindexType type = DetectNoindex(TStringBuf(node->Text.Text.Data, node->Text.Text.Length));

                if (type.IsNoindex()) {
                    chunk.Tag = &(NHtml::FindTag(HT_NOINDEX));

                    state->ApplyNoindex(type);
                }

                result->OnHtmlChunk(chunk);
            }

            void HandleWhitespace(const TNode* node, TParserState* state, IParserResult* result) {
                THtmlChunk chunk(PARSED_TEXT);

                chunk.text = node->Text.OriginalText.Data;
                chunk.leng = node->Text.OriginalText.Length;
                chunk.flags.apos = (i8)HTLEX_TEXT;
                chunk.IsWhitespace = true;
                chunk.IsCDATA = state->IsInLit();

                state->SetWhitespaceOptions(GetYaTag(node->Parent->Element.Tag), &chunk);
                result->OnHtmlChunk(chunk);
            }

        private:
            TOutput* ParserOutput_;
            NHtml::TAttribute Attrs_[512];

            THolder<TTwitterConverter> TwitterConverter_;
        };

        void ParseHtmlImpl(const TParserOptions& opts, TStringBuf html, const TStringBuf& url, IParserResult* result) {
            class TImpl {
            public:
                TImpl(const TParserOptions& theOpts, const TStringBuf& theHtml, TOutput* output)
                    : Output_(output)
                {
                    TParser(theOpts, theHtml.data(), theHtml.size()).Parse(Output_);
                }

                TOutput* Get() const {
                    return Output_;
                }

            private:
                TOutput* Output_;
            };

            //
            // Skip UTF-8 BOM
            //
            if (html.size() >= 3) {
                if (memcmp(html.data(), "\xEF\xBB\xBF", 3) == 0) {
                    html = TStringBuf(html.data() + 3, html.size() - 3);
                }
            }

            //
            // Parse document
            //
            TOutput output;
            TDocumentConstructor(TImpl(opts, html, &output).Get(), url)
                .Construct(opts, result);
        }

    }

    void ParseHtml(TStringBuf html, IParserResult* result, TStringBuf url) {
        TParserOptions opts;
        opts.CompatiblePlainText = true;
        opts.NoindexToComment = true;
        opts.EnableNoindex = true;
        opts.EnableScripting = true;
        opts.PointersToOriginal = true;

        ParseHtmlImpl(opts, html, url, result);
    }

    void ParseHtml(TStringBuf html, IParserResult* result) {
        ParseHtml(html, result, TStringBuf());
    }

    void ParseHtml(TStringBuf html, IParserResult* result, const TParserOptions& opts) {
        ParseHtmlImpl(opts, html, TStringBuf(), result);
    }

    void ParseHtml(TBuffer& doc, IParserResult* result, TStringBuf url) {
        if (doc.Empty())
            return;
        NHtml::NormalizeUtfInput(&doc, false);
        ParseHtml(TStringBuf(doc.Data(), doc.Size()), result, url);
    }

    void ParseHtml(TBuffer& doc, IParserResult* result) {
        ParseHtml(doc, result, TStringBuf());
    }

    template <typename TInput>
    static void ParseHtmlImpl(TInput* input, IParserResult* result) {
        TBuffer doc;
        {
            TBufferOutput out(doc);
            TransferData(input, &out);
        }
        ParseHtml(doc, result, TStringBuf());
    }

    void ParseHtml(IInputStream* input, IParserResult* parserResult) {
        ParseHtmlImpl(input, parserResult);
    }

    void ParseHtml(IZeroCopyInput* input, IParserResult* parserResult) {
        ParseHtmlImpl(input, parserResult);
    }

}
