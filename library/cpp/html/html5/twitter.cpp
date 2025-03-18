#include "twitter.h"
#include "util.h"

#include <library/cpp/uri/uri.h>

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace NHtml5 {
    namespace {
        constexpr TStringBuf TWITTER = "twitter.com";
        const TStringBuf SINGLE_TWIT_CLASS = TStringBuf("\"js-tweet-text tweet-text\"");
        const TStringBuf TWIT_IN_TIMELINE_CLASS = TStringBuf("\"ProfileTweet-text js-tweet-text u-dir\"");
        constexpr TStringBuf CLASS_ATTR = "class";

        inline bool Compare(const TStringPiece& a, const TStringBuf& b) {
            return a.Length == b.size() && strncmp(a.Data, b.data(), a.Length) == 0;
        }

        inline bool HasPrefix(const TStringPiece& str, const TStringBuf& prefix) {
            return str.Length >= prefix.size() && strncmp(str.Data, prefix.data(), prefix.size()) == 0;
        }

    }

    TTwitterConverter::TTwitterConverter(TOutput* output)
        : ParserOutput_(output)
    {
        Alloc_ = [this](size_t len) -> TNode** {
            return ParserOutput_->CreateVector<TNode*>(len);
        };
        NewIndexes_.reserve(32);
    }

    bool TTwitterConverter::IsExceptionalNode(const TNode* node) const {
        if (node->Type != NODE_ELEMENT || node->Element.Tag != TAG_P) {
            return false;
        }
        const TElement& elem = node->Element;
        for (size_t i = 0; i < elem.Attributes.Length; ++i) {
            const TAttribute& attr = elem.Attributes.Data[i];
            if (Compare(attr.OriginalName, CLASS_ATTR) &&
                (Compare(attr.OriginalValue, SINGLE_TWIT_CLASS) || Compare(attr.OriginalValue, TWIT_IN_TIMELINE_CLASS)))
            {
                return true;
            }
        }
        return false;
    }

    void TTwitterConverter::MaybeChangeSubtree(TNode* tree) {
        if (!IsExceptionalNode(tree))
            return;

        Y_ASSERT(tree->Type == NODE_ELEMENT && tree->Element.Tag == TAG_P);

        auto& pChildren = tree->Element.Children;
        int lastAIndex = -1;
        int lastExpandedAIndex = -1;
        // -1 means that node should be removed.
        int curIndex = 0;

        NewIndexes_.resize(pChildren.Length);

        for (size_t i = 0; i < pChildren.Length; ++i) {
            TNode* const node = pChildren.Data[i];

            if (node->Type == NODE_ELEMENT && node->Element.Tag == TAG_A) {
                TElement& elem = node->Element;
                if (ProcessATag(node)) {
                    // Tag expandable.
                    if (lastAIndex == -1 || lastAIndex != lastExpandedAIndex) {
                        // Add to current link.
                        const size_t from = lastAIndex + 1; //< Zero if have no <a> tag yet.
                        const size_t to = i;

                        TVectorType<TNode*>& aChildren = elem.Children;
                        TRange<TNode*> range = {pChildren.Data + from, to - from};
                        aChildren.InsertRangeAt(range, 0, Alloc_);

                        // Change parent.
                        for (size_t ci = 0; ci < aChildren.Length; ++ci) {
                            aChildren.Data[ci]->Parent = node;
                            aChildren.Data[ci]->IndexWithinParent = ci;
                            if (from + ci < to)
                                NewIndexes_[from + ci] = -1; //< need to remove.
                        }
                        curIndex = (lastAIndex == -1) ? 0 : NewIndexes_[lastAIndex] + 1;
                    }
                    lastExpandedAIndex = i;
                }

                NewIndexes_[i] = curIndex;
                ++curIndex;

                lastAIndex = i;
            } else {
                // Add to previoua <a> if it was expandable.
                if (lastAIndex != -1 && lastAIndex == lastExpandedAIndex) {
                    TNode* lastA = pChildren.Data[lastAIndex];
                    node->IndexWithinParent = lastA->Element.Children.Length;
                    node->Parent = lastA;
                    lastA->Element.Children.PushBack(node, Alloc_);

                    NewIndexes_[i] = -1;
                } else {
                    NewIndexes_[i] = curIndex;
                    ++curIndex;
                }
            }
        }

        // remove all moved nodes.
        size_t newLen = 0;
        for (size_t movedIndex = 0; movedIndex < pChildren.Length; ++movedIndex) {
            if (NewIndexes_[movedIndex] != -1) {
                const int newIdx = NewIndexes_[movedIndex];
                pChildren.Data[newIdx] = pChildren.Data[movedIndex];
                pChildren.Data[newIdx]->IndexWithinParent = newIdx;
                ++newLen;
            }
        }
        pChildren.Length = newLen;
    }

    bool TTwitterConverter::ProcessATag(TNode* aElement) {
        Y_ASSERT(aElement->Type == NODE_ELEMENT && aElement->Element.Tag == TAG_A);

        auto& attrs = aElement->Element.Attributes;
        size_t hrefIndex = 0;
        int control = 0;
        bool expand = false;

        // Now attributes follows as: href="https://t.co/a44WljjX1x" rel="nofollow" dir="ltr" data-expanded-url="https://..." ...
        for (size_t i = 0; i < attrs.Length; ++i) {
            if (control == 3)
                // Have found all we want.
                break;

            TAttribute& attr = attrs.Data[i];
            switch (attr.OriginalName.Data[0]) {
                case 'h':
                    // Suppose that href with http indicates external url
                    if (Compare(attr.OriginalName, TStringBuf("href"))) {
                        hrefIndex = i;

                        if (HasPrefix(attr.OriginalValue, TStringBuf("\"http://")) ||
                            HasPrefix(attr.OriginalValue, TStringBuf("\"https://"))) {
                            expand = true;
                        }

                        ++control;
                    }
                    break;

                case 'd':
                    // Rewrite link destination for external links.
                    if (Compare(attr.OriginalName, TStringBuf("data-expanded-url"))) {
                        TAttribute& href = attrs.Data[hrefIndex];
                        href.OriginalValue = attr.OriginalValue;

                        ++control;
                    }
                    break;

                case 'r':
                    // Kill rel=nofollow
                    if (Compare(attr.OriginalName, TStringBuf("rel"))) {
                        attr.OriginalName = TStringPiece::Empty();
                        attr.OriginalValue = TStringPiece::Empty();

                        ++control;
                    }
                    break;

                default:
                    break;
            }
        }

        return expand;
    }

    THolder<TTwitterConverter> CreateTreeConverter(const TStringBuf& url, TOutput* output) {
        if (url.empty() || !url[0])
            return nullptr;

        ::NUri::TUri uri;
        if (uri.Parse(url, ::NUri::TUri::FeaturesRecommended) == ::NUri::TUri::ParsedOK) {
            TStringBuf host = uri.GetHost();

            if (host.Last(TWITTER.size()) == TWITTER)
                return MakeHolder<TTwitterConverter>(output);
        }
        return nullptr;
    }

}
