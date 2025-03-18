#include "chunks.h"

#include <library/cpp/html/escape/escape.h>
#include <library/cpp/html/face/noindex.h>
#include <library/cpp/html/spec/tags.h>

#include <util/generic/stack.h>

using namespace NHtml5;

namespace NHtml {
    namespace NBlob {
        namespace {
            class TEnumerator: public INodeVisitor {
            public:
                TEnumerator(IParserResult* handler)
                    : Handler_(handler)
                    , Scripts_(0)
                {
                }

            private:
                void OnDocumentStart() override {
                }

                void OnDocumentEnd() override {
                }

                void OnDocumentType(const TString& doctype) override {
                    THtmlChunk chunk(PARSED_MARKUP);
                    TString text = TString::Join("<!DOCTYPE ", doctype, ">");

                    chunk.flags.apos = (i8)HTLEX_MD;
                    chunk.flags.markup = MARKUP_IGNORED;
                    chunk.text = text.data();
                    chunk.leng = text.size();

                    Handler_->OnHtmlChunk(chunk);
                }

                void OnElementStart(const TElement& elem) override {
                    THtmlChunk chunk(PARSED_MARKUP);
                    TString text = TString::Join("<", elem.Name);

                    // Setup attributes
                    chunk.AttrCount = Min(elem.Attributes.size(), Y_ARRAY_SIZE(Attrs_));
                    chunk.Attrs = Attrs_;

                    for (size_t i = 0; i < chunk.AttrCount; ++i) {
                        const auto& ai = elem.Attributes[i];
                        const int base = text.size();
                        const TString value = NHtml::EscapeAttributeValue(ai.Value);

                        Attrs_[i].Name.Start = base + 1;
                        Attrs_[i].Name.Leng = ai.Name.size();

                        if (ai.Value) {
                            Attrs_[i].Value.Start = 2 + (Attrs_[i].Name.Start + Attrs_[i].Name.Leng);
                            Attrs_[i].Value.Leng = value.size();
                            Attrs_[i].Quot = '"';
                        } else {
                            Attrs_[i].Value = Attrs_[i].Name;
                            Attrs_[i].Quot = '\0';
                        }

                        text += TString::Join(" ", ai.Name, "=\"", value, "\"");
                    }

                    text += ">";

                    // Setup chunk
                    chunk.Tag = &NHtml::FindTag(elem.Name);
                    chunk.text = text.data();
                    chunk.leng = text.size();
                    // Set apos.
                    if (chunk.Tag->is(HT_empty)) {
                        chunk.flags.apos = (i8)HTLEX_EMPTY_TAG;
                    } else {
                        chunk.flags.apos = (i8)HTLEX_START_TAG;
                    }
                    // Set flags
                    chunk.flags.markup = MARKUP_NORMAL;

                    Handler_->OnHtmlChunk(chunk);

                    if (chunk.Tag->id() == HT_SCRIPT || chunk.Tag->id() == HT_STYLE) {
                        ++Scripts_;
                    }
                }

                void OnElementEnd(const TString& name) override {
                    const NHtml::TTag* tag = &NHtml::FindTag(name);

                    if (!tag->is(HT_empty)) {
                        THtmlChunk chunk(PARSED_MARKUP);
                        TString text = TString::Join("</", name, ">");

                        chunk.Tag = tag;
                        chunk.flags.apos = (i8)HTLEX_END_TAG;
                        chunk.flags.markup = MARKUP_NORMAL;
                        chunk.text = text.data();
                        chunk.leng = text.size();

                        Handler_->OnHtmlChunk(chunk);

                        if (chunk.Tag->id() == HT_SCRIPT || chunk.Tag->id() == HT_STYLE) {
                            --Scripts_;
                        }
                    }
                }

                void OnText(const TString& text) override {
                    THtmlChunk chunk(PARSED_TEXT);
                    TString data = Scripts_ > 0 ? text : NHtml::EscapeText(text);

                    chunk.text = data.data();
                    chunk.leng = data.size();
                    chunk.flags.apos = (i8)HTLEX_TEXT;
                    chunk.IsWhitespace = IsWhitespaceText(data);

                    Handler_->OnHtmlChunk(chunk);
                }

                void OnComment(const TString& text) override {
                    THtmlChunk chunk(PARSED_MARKUP);
                    TString data = TString::Join("<!--", text, "-->");

                    chunk.text = data.data();
                    chunk.leng = data.size();
                    chunk.flags.apos = (i8)HTLEX_COMMENT;
                    chunk.flags.markup = MARKUP_IGNORED;

                    Handler_->OnHtmlChunk(chunk);
                }

            private:
                static bool IsWhitespaceText(const TStringBuf& text) {
                    for (auto ci = text.begin(); ci != text.end(); ++ci) {
                        if (!isspace(*ci)) {
                            return false;
                        }
                    }
                    return true;
                }

            private:
                IParserResult* Handler_;
                NHtml::TAttribute Attrs_[512];
                int Scripts_;
            };

            class TIndexerFlags: public IParserResult {
            public:
                TIndexerFlags(IParserResult* slave)
                    : Slave_(slave)
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

            private:
                const TTag* GetParentTag() const {
                    Y_ASSERT(!OpenNodes_.empty());
                    return OpenNodes_.top();
                }

                THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override {
                    THtmlChunk ev(chunk);

                    switch (chunk.GetLexType()) {
                        case HTLEX_START_TAG:
                        case HTLEX_EMPTY_TAG: {
                            if (!ev.Tag->id()) {
                                ev.flags.markup = MARKUP_IGNORED;
                            }
                            ev.flags.brk = GetBreakType(ev.Tag, chunk.GetLexType());
                            ev.flags.weight = GetMarkupWeight();

                            Slave_->OnHtmlChunk(ev);
                            UpdateOnTagStart(ev.Tag);

                            if (chunk.GetLexType() == HTLEX_START_TAG) {
                                OpenNodes_.push(ev.Tag);
                            }
                            break;
                        }

                        case HTLEX_END_TAG: {
                            OpenNodes_.pop();
                            UpdateOnTagEnd(ev.Tag);

                            if (!ev.Tag->id()) {
                                ev.flags.markup = MARKUP_IGNORED;
                            }
                            ev.flags.brk = GetBreakType(ev.Tag, HTLEX_END_TAG);
                            ev.flags.weight = GetMarkupWeight();

                            Slave_->OnHtmlChunk(ev);
                            break;
                        }

                        case HTLEX_TEXT: {
                            if (ev.IsWhitespace) {
                                ev.flags.space = GetSpaceMode(GetParentTag());
                                ev.IsCDATA = InLit_;
                            } else {
                                ev.flags.weight = GetTextWeight(GetParentTag());
                                ev.flags.space = GetSpaceMode(GetParentTag());
                                ev.Format = Format_.IrregTag;
                                ev.IsCDATA = InLit_;

                                // private hack to distinguish noindex text used for language detection
                                if ((Format_.IrregTag & IRREG_NOINDEX) && Format_.W0Count == 0) {
                                    ev.flags.atype = 1;
                                }

                                UpdateOnText();
                            }

                            Slave_->OnHtmlChunk(ev);
                            break;
                        }

                        case HTLEX_COMMENT: {
                            // Check for noindex
                            const TNoindexType type = DetectNoindex(ev.text, ev.leng);
                            if (type.IsNoindex()) {
                                ev.Tag = &(FindTag(HT_NOINDEX));
                                ApplyNoindex(type);
                            }

                            ev.flags.markup = MARKUP_IGNORED;
                            ev.IsCDATA = InLit_;

                            Slave_->OnHtmlChunk(ev);
                            break;
                        }

                        case HTLEX_EOF:
                        case HTLEX_MD:
                        case HTLEX_PI:
                        case HTLEX_ASP:
                            Slave_->OnHtmlChunk(ev);
                            break;
                    }
                    return nullptr;
                }

            private:
                // Used for comments.
                void ApplyNoindex(const TNoindexType& type) {
                    Y_ASSERT(type.IsNoindex());

                    if (type.IsClose()) {
                        Format_.IrregTag = TIrregTag(Format_.IrregTag & ~IRREG_NOINDEX);
                    } else {
                        Format_.IrregTag = TIrregTag(Format_.IrregTag | IRREG_NOINDEX);
                    }
                }

                BREAK_TYPE GetBreakType(const NHtml::TTag* tag, HTLEX_TYPE lexType) const {
                    switch (tag->id()) {
                        case HT_A:
                            if (lexType == HTLEX_END_TAG) {
                                return BREAK_NONE;
                            }
                            break;
                        case HT_BODY:
                            if (lexType == HTLEX_START_TAG) {
                                return BREAK_BODY;
                            }
                            break;
                        case HT_BR:
                            // <BR>space*<BR> breaks paragraph, single <BR> breaks word
                            if (LastTag_ == HT_BR) {
                                return BREAK_PARAGRAPH;
                            } else {
                                return BREAK_WORD;
                            }
                            break;
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

                TEXT_WEIGHT GetMarkupWeight() const {
                    if (/*Opts_.EnableNoindex && */ (Format_.IrregTag & IRREG_NOINDEX)) {
                        return WEIGHT_ZERO;
                    }
                    if (Format_.W0Count) {
                        return WEIGHT_ZERO;
                    }
                    return WEIGHT_NORMAL;
                }

                SPACE_MODE GetSpaceMode(const TTag* parent) const {
                    if (parent->is(HT_lit) || Format_.PreCount) {
                        return SPACE_PRESERVE;
                    }
                    return SPACE_DEFAULT;
                }

                TEXT_WEIGHT GetTextWeight(const TTag* parent) const {
                    if (/*Opts_.EnableNoindex && */ (Format_.IrregTag & IRREG_NOINDEX)) {
                        return WEIGHT_ZERO;
                    }
                    if (parent->id() == HT_TITLE) {
                        if (IsTitleMet_)
                            return WEIGHT_ZERO; // Kill all additional titles
                        else
                            return WEIGHT_BEST;
                    }
                    if (Format_.W0Count) {
                        return WEIGHT_ZERO;
                    }
                    if (Format_.BCount) {
                        return WEIGHT_HIGH;
                    }
                    return WEIGHT_NORMAL;
                }

                bool OverrideW0(const TTag* tag) const {
                    switch (tag->id()) {
                        // Content of tag <comment> is visible in all modern browsers.
                        case HT_COMMENT:
                            return true;
                        // See https://st.yandex-team.ru/INDEX-138
                        case HT_OPTION:
                            return true;
                        case HT_NOINDEX:
                            //if (!Opts_.EnableNoindex)
                            {
                                return true;
                            }
                            break;
                        default:
                            break;
                    }
                    return false;
                }

                inline void UpdateOnTagStart(const TTag* tag) {
                    if (tag->is(HT_w0)) {
                        if (!OverrideW0(tag)) {
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

                void UpdateOnTagEnd(const TTag* tag) {
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
                        if (!OverrideW0(tag)) {
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

                void UpdateOnText() {
                    LastTag_ = HT_any;
                }

            private:
                struct TFormat {
                    unsigned W0Count;
                    unsigned PreCount;
                    unsigned BCount;
                    TIrregTag IrregTag;
                };

                IParserResult* Slave_;
                TStack<const TTag*> OpenNodes_;
                TFormat Format_;
                HT_TAG LastTag_;
                bool InBody_;
                bool InLit_;
                bool IsTitleMet_;
            };

        }

        bool NumerateHtmlChunks(const TDocumentRef& doc, IParserResult* result) {
            TIndexerFlags flags(result);
            TEnumerator e(&flags);

            return doc->EnumerateHtmlTree(&e);
        }

    }
}
