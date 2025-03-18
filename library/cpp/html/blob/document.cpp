#include "chunks.h"
#include "document.h"
#include "format.h"
#include "internal.h"

#include <library/cpp/packedtypes/longs.h>

#include <util/generic/map.h>
#include <util/generic/queue.h>
#include <util/generic/stack.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>

namespace NHtml {
    namespace NBlob {
        namespace {
            struct TNodeState {
                TString Name;
                //! Задаёт ли текущий узел ключевой
                //! вектор стилей.
                bool KeyStyles = false;
            };

            struct TEnumerateState {
                const char* P;
                const char* End;
                TStack<TNodeState> OpenNodes;

                TStack<TVector<std::pair<i32, i32>>> Updates;
                TStack<TMap<i32, i32>> Styles;

                explicit TEnumerateState(const TStringBuf& data)
                    : P(data.data())
                    , End(data.data() + data.size())
                {
                }
            };

            using TStringPair = std::pair<TString, TString>;

        }

        TDocument::TFrame::TFrame(TDocument* doc, const TDocumentPack::TFrame& meta)
            : Document_(doc)
            , Meta_(meta)
        {
        }

        bool TDocument::TFrame::EnumerateHtmlTree(INodeVisitor* visitor) const {
            return Document_->EnumerateHtmlTree(visitor, GetId());
        }

        TString TDocument::TFrame::GetId() const {
            return Meta_.GetId();
        }

        TString TDocument::TFrame::GetUrl() const {
            return Meta_.GetUrl();
        }

        TDocument::TDocument()
            : Data_()
            , EnableAttributes_(false)
            , EnableStyles_(false)
        {
        }

        TDocument::TDocument(const TBlob& data, const TDocumentOptions& opts)
            : Data_(data)
            , EnableAttributes_(opts.EnableAttributes)
            , EnableStyles_(opts.EnableStyles)
        {
            if (!GetMetadata(TStringBuf(data.AsCharPtr(), data.Length()), &Meta_)) {
                ythrow yexception() << "fail to load metadata";
            }
            if (!LoadFrames()) {
                ythrow yexception() << "fail to load frame data";
            }
            if (!UpdateStrings()) {
                ythrow yexception() << "fail to load string values";
            }
        }

        TDocument::~TDocument() {
        }

        bool TDocument::EnumerateHtmlTree(INodeVisitor* visitor) const {
            return EnumerateHtmlTree(visitor, "main");
        }

        bool TDocument::EnumerateHtmlTree(INodeVisitor* visitor, const TString& frameId) const {
            auto fi = Frames_.find(frameId);
            if (fi != Frames_.end()) {
                return EnumerateHtmlTreeImpl(fi->second.second, visitor);
            }
            return false;
        }

        void TDocument::EnumerateFrames(std::function<void(const TFrame&)> cb) const {
            TQueue<const TDocumentPack::TFrame*> frames;
            // Обходим по структуре метаданных, чтобы обеспечить
            // естественный порядок следования.
            for (frames.push(&Meta_.GetMain()); !frames.empty(); frames.pop()) {
                const auto& frame = frames.front();

                cb(Frames_.find(frame->GetId())->second.first);

                for (const auto& item : frame->children()) {
                    frames.push(&item);
                }
            }
        }

        ui32 TDocument::GetHeight() const {
            if (Meta_.GetContentsSize().HasHeight()) {
                return Meta_.GetContentsSize().GetHeight();
            }
            return 1;
        }

        float TDocument::GetScaleFactor() const {
            if (Meta_.GetContentsSize().HasScaleFactor()) {
                return Meta_.GetContentsSize().GetScaleFactor();
            }
            return 1.0f;
        }

        TString TDocument::GetUrl() const {
            return Meta_.GetMain().GetUrl();
        }

        ui32 TDocument::GetWidth() const {
            if (Meta_.GetContentsSize().HasWidth()) {
                return Meta_.GetContentsSize().GetWidth();
            }
            return 1;
        }

        TDocumentRef TDocument::FromFile(const TString& path, const TDocumentOptions& opt) {
            return FromString(TUnbufferedFileInput(path).ReadAll(), opt);
        }

        TDocumentRef TDocument::FromString(const TString& data, const TDocumentOptions& opt) {
            return FromBlob(TBlob::FromString(data), opt);
        }

        TDocumentRef TDocument::FromBlob(
            const TBlob& data,
            const TDocumentOptions& opt) {
            return TDocumentRef(new TDocument(data, opt));
        }

        bool TDocument::EnumerateHtmlTreeImpl(const TStringBuf& data, INodeVisitor* visitor) const {
            TEnumerateState s(data);

            if (s.P == s.End) {
                return true;
            } else {
                visitor->OnDocumentStart();
            }

            while (s.P < s.End) {
                i32 flags;
                i32 id = 0;

                s.P += in_long(flags, s.P);

                switch (ui8 type = flags & NODE_TYPE_MASK) {
                    case NODE_CLOSE: {
                        if (s.OpenNodes.empty()) {
                            if (s.P != s.End) {
                                return false;
                            } else {
                                goto done;
                            }
                        } else {
                            visitor->OnElementEnd(s.OpenNodes.top().Name);

                            if (s.Styles) {
                                if (s.OpenNodes.top().KeyStyles) {
                                    s.Styles.pop();
                                } else if (s.Updates) {
                                    const auto& prev = s.Updates.top();
                                    auto& base = s.Styles.top();
                                    for (size_t i = 0; i < prev.size(); ++i) {
                                        base[prev[i].first] = prev[i].second;
                                    }
                                    s.Updates.pop();
                                }
                            }

                            s.OpenNodes.pop();
                        }
                        break;
                    }
                    case NODE_ELEMENT: {
                        s.P += in_long(id, s.P);
                        TElement elem;
                        TNodeState node;

                        elem.Name = Tags_.at(id);
                        elem.Tag = &FindTag(elem.Name);
                        node.Name = elem.Name;

                        if (flags & FIELD_VIEW) {
                            s.P += in_long(elem.Viewbound.X, s.P);
                            s.P += in_long(elem.Viewbound.Y, s.P);
                            s.P += in_long(elem.Viewbound.Width, s.P);
                            s.P += in_long(elem.Viewbound.Height, s.P);
                        }
                        if (flags & FIELD_ATTRIBUTES) {
                            if (EnableAttributes_) {
                                s.P += LoadIndexMap(s.P, AttrNames_, AttrValues_, &elem.Attributes);
                            } else {
                                s.P += SkipIndexMap(s.P);
                            }
                        }
                        if (flags & FIELD_MATCHED) {
                            if (EnableStyles_) {
                                s.P += LoadIndexMap(s.P, StyleNames_, StyleValues_, &elem.MatchedStyle);
                            } else {
                                s.P += SkipIndexMap(s.P);
                            }
                        }
                        if (flags & FIELD_COMPUTED) {
                            i32 key;
                            s.P += in_long(key, s.P);
                            if (EnableStyles_) {
                                TVector<std::pair<i32, i32>> map;
                                s.P += LoadIndexMap(s.P, &map);

                                if (key) {
                                    node.KeyStyles = true;

                                    s.Styles.push(
                                        TMap<i32, i32>(map.begin(), map.end()));
                                } else {
                                    auto& base = s.Styles.top();
                                    TVector<std::pair<i32, i32>> prev;

                                    for (size_t i = 0; i < map.size(); ++i) {
                                        prev.push_back(std::make_pair(
                                            map[i].first, base[map[i].first]));

                                        base[map[i].first] = map[i].second;
                                    }

                                    s.Updates.push(prev);
                                }

                                const auto& base = s.Styles.top();
                                for (auto pi = base.begin(); pi != base.end(); ++pi) {
                                    elem.ComputedStyle.push_back(
                                        TElement::TPair{
                                            StyleNames_.at(pi->first),
                                            StyleValues_.at(pi->second)});
                                }
                            } else {
                                s.P += SkipIndexMap(s.P);
                            }
                        }
                        if (flags & FIELD_EXTENSIONS) {
                            i32 len;
                            s.P += in_long(len, s.P);
                            s.P += len;
                        }

                        visitor->OnElementStart(elem);
                        s.OpenNodes.push(node);
                        break;
                    }
                    case NODE_TEXT: {
                        s.P += in_long(id, s.P);
                        visitor->OnText(Texts_.at(id));
                        break;
                    }
                    case NODE_COMMENT: {
                        s.P += in_long(id, s.P);
                        visitor->OnComment(Texts_.at(id));
                        break;
                    }
                    case NODE_DOCUMENT: {
                        break;
                    }
                    case NODE_DOCUMENT_TYPE: {
                        s.P += in_long(id, s.P);
                        visitor->OnDocumentType(Texts_.at(id));
                        break;
                    }
                    default: {
                        break;
                    }
                }

                if (s.P >= s.End) {
                    return false;
                }
            }
        done:
            visitor->OnDocumentEnd();

            return true;
        }

        bool TDocument::LoadFrames() {
            TQueue<const TDocumentPack::TFrame*> frames;

            for (frames.push(&Meta_.GetMain()); !frames.empty(); frames.pop()) {
                auto frame = frames.front();

                Frames_.insert(
                    std::make_pair(frame->GetId(),
                                   std::make_pair(TFrame(this, *frame), Rebase(frame->GetData()))));

                for (auto fi = frame->children().begin(); fi != frame->children().end(); ++fi) {
                    frames.push(&(*fi));
                }
            }

            return true;
        }

        ptrdiff_t TDocument::LoadIndexMap(
            const char* p,
            const TVector<TString>& names,
            const TVector<TString>& values,
            TVector<TElement::TPair>* map) const {
            const char* const s = p;
            i32 count;

            p += in_long(count, p);
            map->reserve(count);

            for (i32 i = 0; i < count; ++i) {
                i32 name;
                i32 value;

                p += in_long(name, p);
                p += in_long(value, p);

                map->push_back(TElement::TPair{names.at(name), values.at(value)});
            }

            return p - s;
        }

        ptrdiff_t TDocument::LoadIndexMap(
            const char* p,
            TVector<std::pair<i32, i32>>* map) const {
            const char* const s = p;
            i32 count;

            p += in_long(count, p);
            map->reserve(count);

            for (i32 i = 0; i < count; ++i) {
                i32 name;
                i32 value;

                p += in_long(name, p);
                p += in_long(value, p);

                map->push_back(std::make_pair(name, value));
            }

            return p - s;
        }

        TStringBuf TDocument::Rebase(const TRange& range) const {
            const char* const begin = Data_.AsCharPtr();
            const char* const end = Data_.AsCharPtr() + Data_.Length();

            if (begin + range.GetBegin() > end || begin + range.GetEnd() > end || range.GetBegin() > range.GetEnd()) {
                ythrow yexception() << "range out of bounds";
            }

            return TStringBuf(
                begin + range.GetBegin(),
                begin + range.GetEnd());
        }

        ptrdiff_t TDocument::SkipIndexMap(const char* p) const {
            const char* const s = p;
            i32 count;

            p += in_long(count, p);

            for (i32 i = 0; i < count; ++i) {
                i32 idx;

                p += in_long(idx, p);
                p += in_long(idx, p);
            }

            return p - s;
        }

        bool TDocument::UpdateStrings() {
            auto strings = Meta_.GetStrings();

            if (strings.HasTags() && !Tags_) {
                if (!GetStrings(Rebase(strings.GetTags()), &Tags_)) {
                    return false;
                }
            }
            if (strings.HasTexts() && !Texts_) {
                if (!GetStrings(Rebase(strings.GetTexts()), &Texts_)) {
                    return false;
                }
            }
            if (EnableAttributes_) {
                if (strings.HasAttributeNames() && !AttrNames_) {
                    if (!GetStrings(Rebase(strings.GetAttributeNames()), &AttrNames_)) {
                        return false;
                    }
                }
                if (strings.HasAttributeValues() && !AttrValues_) {
                    if (!GetStrings(Rebase(strings.GetAttributeValues()), &AttrValues_)) {
                        return false;
                    }
                }
            }
            if (EnableStyles_) {
                if (strings.HasStyleNames() && !StyleNames_) {
                    if (!GetStrings(Rebase(strings.GetStyleNames()), &StyleNames_)) {
                        return false;
                    }
                }
                if (strings.HasStyleValues() && !StyleValues_) {
                    if (!GetStrings(Rebase(strings.GetStyleValues()), &StyleValues_)) {
                        return false;
                    }
                }
            }

            return true;
        }

    }
}
