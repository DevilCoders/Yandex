#include "print.h"

#include <library/cpp/html/storage/storage.h>

#include <util/generic/stack.h>
#include <util/stream/str.h>
#include <utility>

namespace NHtml {
    class THtmlPrinter::TImpl {
    public:
        TImpl(IOutputStream& out, const TPrintConfig& config)
            : Output_(out)
            , Config_(config)
        {
        }

        ~TImpl() {
            Y_ASSERT(Parents_.empty());
        }

        bool IsBlock(const NHtml::TTag* tag) const {
            if (tag) {
                if (tag->is(HT_inline)) {
                    return false;
                }
                switch (tag->id()) {
                    case HT_A:
                    case HT_B:
                    case HT_EM:
                    case HT_FONT:
                    case HT_H1:
                    case HT_H2:
                    case HT_H3:
                    case HT_H4:
                    case HT_H5:
                    case HT_H6:
                    case HT_I:
                    case HT_NOBR:
                    case HT_SPAN:
                    case HT_STRIKE:
                    case HT_STRONG:
                    case HT_SUB:
                    case HT_SUP:
                    case HT_TITLE:
                    case HT_TT:
                    case HT_U:
                        return false;
                    default:
                        break;
                }
                return true;
            }
            return false;
        }

        void OnHtmlChunk(const THtmlChunk& chunk) {
            switch (chunk.GetLexType()) {
                case HTLEX_START_TAG:
                    if (Config_.Format) {
                        if (Parents_.empty()) {
                            EmitIndet();
                        } else {
                            if (IsBlock(Parents_.top().first)) {
                                EmitIndet();
                            }
                            Parents_.top().second += 1;
                        }
                        Parents_.push(std::make_pair(chunk.Tag, 0));
                    }
                    EmitStartTag(chunk);
                    break;
                case HTLEX_EMPTY_TAG:
                    if (Config_.Format) {
                        if (Parents_) {
                            Parents_.top().second += 1;
                        }
                        EmitIndet();
                    }
                    EmitStartTag(chunk);
                    break;
                case HTLEX_END_TAG:
                    if (Config_.Format) {
                        auto value = Parents_.top();
                        Parents_.pop();
                        if (IsBlock(value.first) && value.second) {
                            EmitIndet();
                        }
                    }
                    EmitEndTag(chunk);
                    break;
                case HTLEX_TEXT:
                    if (Config_.Format) {
                        if (chunk.IsWhitespace) {
                            return;
                        }

                        if (!Parents_.empty()) {
                            Parents_.top().second += 1;

                            if (IsBlock(Parents_.top().first)) {
                                EmitIndet();
                            }
                        }
                    } else if (Config_.Strip) {
                        if (chunk.IsWhitespace) {
                            return;
                        }
                    }
                    EmitText(chunk);
                    break;
                case HTLEX_COMMENT:
                    if (Config_.Format) {
                        if (!Parents_.empty()) {
                            Parents_.top().second += 1;
                        }
                        EmitIndet();
                    }
                    if (Config_.Comments) {
                        EmitText(chunk);
                    }
                    break;
                case HTLEX_MD:
                    EmitText(chunk);
                    break;
                default:
                    break;
            }
        }

    private:
        inline void EmitIndet() {
            Output_ << '\n'
                    << TString(Config_.Indent * Parents_.size(), Config_.Char);
        }

        inline void EmitText(const THtmlChunk& chunk) {
            Output_ << TStringBuf(chunk.text, chunk.leng);
        }

        inline void EmitStartTag(const THtmlChunk& chunk) {
            Output_ << "<" << TagName(chunk);

            for (size_t i = 0; i < chunk.AttrCount; ++i) {
                const NHtml::TAttribute& attr = chunk.Attrs[i];

                Output_ << " ";
                Output_ << TStringBuf(chunk.text + attr.Name.Start, attr.Name.Leng);

                if (!attr.IsBoolean()) {
                    Output_ << "=";
                    if (attr.Quot) {
                        Output_ << attr.Quot;
                    }
                    Output_ << TStringBuf(chunk.text + attr.Value.Start, attr.Value.Leng);
                    if (attr.Quot) {
                        Output_ << attr.Quot;
                    }
                }
            }

            Output_ << ">";
        }

        inline void EmitEndTag(const THtmlChunk& chunk) {
            Output_ << "</" << TagName(chunk) << ">";
        }

        inline TStringBuf TagName(const THtmlChunk& chunk) const {
            if (chunk.Tag->id() == HT_any)
                return GetTagName(chunk);
            else
                return TStringBuf(chunk.Tag->lowerName);
        }

    private:
        IOutputStream& Output_;
        const TPrintConfig Config_;
        TStack<std::pair<const NHtml::TTag*, int>> Parents_;
    };

    THtmlPrinter::THtmlPrinter(IOutputStream& out, const TPrintConfig& config)
        : Impl_(new TImpl(out, config))
    {
    }

    THtmlPrinter::~THtmlPrinter() {
    }

    THtmlChunk* THtmlPrinter::OnHtmlChunk(const THtmlChunk& chunk) {
        Impl_->OnHtmlChunk(chunk);
        return nullptr;
    }

    TString PrintHtml(const TStorage& html, const TPrintConfig& config) {
        TStringStream output;
        THtmlPrinter printer(output, config);

        for (auto ci = html.Begin(); ci != html.End(); ++ci) {
            printer.OnHtmlChunk(*ci);
        }

        return output.Str();
    }

}
