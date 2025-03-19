#include "zonedstring.h"
#include "goodwrds.h"
#include "glue_common.h"

#include <util/charset/unidata.h>
#include <util/generic/deque.h>
#include <util/generic/algorithm.h>
#include <util/generic/list.h>

#include <library/cpp/tokenizer/tokenizer.h>
#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/alpha/abc.h>

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/wordnode.h>

using namespace NSearchQuery;

namespace NHighlighter {
    struct TForm {
        TUtf16String Form;
        TLangMask Lang;
        TForm() {
        }
        TForm(const TUtf16String& form, const TLangMask& lang)
          : Form(form)
          , Lang(lang)
        {
        }
    };
    struct TWordNodeModel {
        TVector<TForm> Forms;
        TLangMask Langs;
        const THiliteMark* Mark = nullptr;
        bool Quoted = false;
        bool QuotedAlone = false;
        EHiliteType HiliteType = HILITE_NONE;
        size_t LeftId = 666;
        size_t RightId = 666;
        bool Hidden = false;
        bool IsAttr = false;
    };
    struct TJoker {
        size_t LeftId = 0;
        size_t RightId = 0;
        size_t Count = 0;
        TJoker() {
        }
        TJoker(size_t leftId, size_t rightId, size_t count)
          : LeftId(leftId)
          , RightId(rightId)
          , Count(count)
        {
        }
    };
    struct TFioInfo {
        TUtf16String F;
        TUtf16String I;
        TUtf16String O;
        size_t Id = 0;
    };
    struct TQueryGraph {
    public:
        TQueryGraph() {
        }
        struct TLookupVal {
            TVector<const TWordNodeModel*> Words;
            bool IsFio = false;
            TLangMask WordLangMask;
        };
        typedef THashMap<TWtringBuf, TLookupVal> TLookup;
        typedef TList<TWordNodeModel> TGraph;
        TGraph Graph;
        TGraph FioGraph;
        TLookup Lookup;
        TLangMask LangMask;
        TVector<TFioInfo> FioInfo;
        TVector<TJoker> Jokers;
        TVector<TFioInfo> FioInfos;
        bool HasFioWord = false;
        bool HasLongFioName = false;
        bool HasShortFioName = false;
        THashSet<size_t> QuoteEdges;
        THashSet<size_t> AbbrevEdges;
        THashSet<size_t> Dom2Edges;
        THashSet<const TWordNodeModel*> StopwordsToHack;
        typedef THashMap<const TWordNodeModel*, TVector<const TWordNodeModel*>> TSimpleSynonyms;
        TSimpleSynonyms SimpleSynonyms;
        TSimpleSynonyms SimpleSynonymsReversed;

        void AddRequest(const TRichRequestNode&, const THiliteMark* marks);
        bool HasNode(const TWtringBuf& tok) const;
        bool Empty() const;
        size_t GetNewEdgeId() {
            return EdgeId++;
        }
        size_t GetFioId() {
            FioId |= 1;
            return FioId / 2;
        }
        void ShiftFioId() {
            if (FioId % 2) {
                ++FioId;
            }
        }
    private:
        size_t EdgeId = 0;
        size_t FioId = 0;
        struct TWalkerState {
            TQueryGraph* Parent = nullptr;
            size_t LeftId = 0;
            size_t RightId = 0;
            const THiliteMark* Marks = nullptr;
            bool InQuotes = false;
            TThesaurusExtensionId InExtension = TThesaurusExtensionId();
            bool SingleExtension = false;
            TVector<TWordNodeModel*> AddedNodes;
            TWalkerState Spawn() const {
                TWalkerState res;
                res.Parent = Parent;
                res.LeftId = LeftId;
                res.RightId = RightId;
                res.Marks = Marks;
                res.InQuotes = InQuotes;
                res.InExtension = InExtension;
                res.SingleExtension = SingleExtension;
                return res;
            }
        };
        typedef TVector<TWalkerState> TWalkerChildrenState;
        void InitChildrenState(TWalkerChildrenState& chState, size_t cnt, const TWalkerState& state, bool sameEdges) {
            chState.clear();
            if (!cnt) {
                return;
            }
            TWalkerState sp = state.Spawn();
            sp.SingleExtension = cnt == 1;
            chState.resize(cnt, sp);
            if (!sameEdges) {
                for (size_t i = 1; i != chState.size(); ++i) {
                    chState[i - 1].RightId = chState[i].LeftId = GetNewEdgeId();
                    if (state.InQuotes) {
                        QuoteEdges.insert(chState[i].LeftId);
                    }
                    if (state.InExtension == TE_ABBREV) {
                        AbbrevEdges.insert(chState[i].LeftId);
                    }
                }
            }
        }
        void WalkRequest(const TRichRequestNode&, TWalkerState& state);
        void AddWordNode(const TWordNode* node, EHiliteType hlType, bool wasQuoted, bool isAttr, TWalkerState& state);
        void AddFIO(const TRichRequestNode& node, const TUtf16String&, TWalkerState& state);
        void ParseWordNode(TWordNodeModel& res, const TWordNode* node, EHiliteType hlType, bool wasQuoted, bool isAttr, TWalkerState& state);
        TWordNodeModel* ConsumeWordNode(const TWordNodeModel& w);
        void ConsumeFioWordNode(const TWordNodeModel& w);
    };

    struct TMarkedZone {
        const THiliteMark* Mark = nullptr;
        size_t BeginOfs = 0;
        size_t EndOfs = 0;
    };

    struct TToken {
        size_t Offset = 0;
        size_t BodyLength = 0;
        size_t SuffixLength = 0;
        size_t SentId = 0;
        size_t FioPosId = 0;
        bool Space = false;
        TWtringBuf Src;
        TQueryGraph::TLookup::const_iterator LookupIt;
        bool SuffixedLookup = false;
        bool FioInitialLookup = false;
        const THiliteMark* Result = nullptr;
        size_t TotalLength() const {
            return BodyLength + SuffixLength;
        }
        TToken(size_t offset, size_t bodyLength, const TWtringBuf& src, size_t suffixLength = 0)
          : Offset(offset)
          , BodyLength(bodyLength)
          , SuffixLength(suffixLength)
          , Src(src)
        {
        }
    };
    struct TZonesTransformer {
    public:
        TZonedString::TZones* SrcZones;
        using TSpan = std::pair<size_t, size_t>;
        using TSpans = TVector<TSpan>;
        struct TZone {
            TSpans Spans;
            const THiliteMark* Mark = nullptr;

            TZone() {
            }

            TZone(TSpans spans, const THiliteMark* mark)
              : Spans(std::move(spans))
              , Mark(mark)
            {
            }

            TZone& operator = (TZone&& src) = default;
        };
        using TZones = THashMap<int, TZone>;
        TZones DstZones;
    private:
        struct TIter {
            const TZonedString::TSpans* SrcSpans = nullptr;
            TSpans* DstSpans = nullptr;
            size_t Done = 0;
            TWtringBuf PrevSrc;
            TIter() {
            }
            TIter(const TZonedString::TSpans* srcSpans, TSpans* dstSpans)
              : SrcSpans(srcSpans)
              , DstSpans(dstSpans)
            {
            }
            bool Intersects(const TWtringBuf& a, const TWtringBuf& b) const {
                return a.data() >= b.data() && a.data() < b.data() + b.size() || b.data() >= a.data() && b.data() < a.data() + a.size();
            }
            void OnTokens(size_t i, size_t j, const TWtringBuf& src) {
                while (Done < SrcSpans->size() && src.data() >= ~(*SrcSpans)[Done] + +(*SrcSpans)[Done]) {
                    ++Done;
                }
                if (Done < SrcSpans->size() && Intersects(src, (*SrcSpans)[Done].Span)) {
                    if (!Intersects(PrevSrc, (*SrcSpans)[Done].Span)) {
                        DstSpans->push_back(TSpan(i, j));
                    } else {
                        DstSpans->back().second = j;
                    }
                }
                PrevSrc = src;
            }
        };
        THashMap<int, TIter> Iters;
   public:
        TZonesTransformer(TZonedString* src)
          : SrcZones(src ? &src->Zones : nullptr)
        {
            if (SrcZones) {
                src->Normalize();
                for (const auto& item : *SrcZones) {
                    DstZones[item.first] = TZone(TSpans(), item.second.Mark);
                    if (item.second.Spans) {
                        Iters[item.first] = TIter(&item.second.Spans, &DstZones[item.first].Spans);
                    }
                }
            }
        }
        void OnTokens(size_t i, size_t j, const TWtringBuf& src) {
            if (!SrcZones) {
                return;
            }
            for (auto& it : Iters) {
                it.second.OnTokens(i, j, src);
            }
        }
    };
    struct TText {
    public:
        TUtf16String TokenPool;
    private:
        TVector<TToken> Tokens;
        bool LastIsMiscSpace = false;
        typedef TVector<size_t> TWordIdx;
        TWordIdx WordIdx;
        TVector<bool> GoRight;
        TVector<bool> ForcedGoRight;
        TZonesTransformer ZonesTransformer;
    public:
        TText(TZonedString* zones = nullptr)
          : ZonesTransformer(zones)
          , WordsInSent(size_t(1), 0)
        {
        }
        const TZonesTransformer& GetZonesTransformer() const {
            return ZonesTransformer;
        }
        size_t TokenCount() const {
            return Tokens.size();
        }
        TToken& GetToken(size_t idx) {
            return Tokens[idx];
        }
        const TToken& GetToken(size_t idx) const {
            return Tokens[idx];
        }
        size_t WordCount() const {
            return WordIdx.size();
        }
        TToken& GetWord(size_t idx) {
            return Tokens[WordIdx[idx]];
        }
        const TToken& GetWord(size_t idx) const {
            return Tokens[WordIdx[idx]];
        }
        bool KindOfApostropheAfterWord(size_t idx) const {
            if (idx + 1 >= WordIdx.size()) {
                return false;
            }
            size_t i = WordIdx[idx];
            size_t j = WordIdx[idx + 1];
            if (j != i + 2) {
                return false;
            }
            const TToken& t = Tokens[i + 1];
            return t.TotalLength() == 1 && TokenPool[t.Offset] == '`';
        }
        bool WordGoesRight(size_t idx, bool allowTokRight) const {
            return ForcedGoRight[idx] || allowTokRight && GoRight[idx];
        }
        void MakeWordGoRight(size_t idx) {
            ForcedGoRight[idx] = true;
        }
        bool MayStickRight(size_t idx) const {
            if (idx + 1 >= WordCount()) {
                return false;
            }
            const TToken& a = GetWord(idx);
            const TToken& b = GetWord(idx + 1);
            if (a.SentId == b.SentId) {
                return true;
            }
            return WordsInSent[a.SentId] == 1 && WordsInSent[b.SentId] == 1;
        }
        void AddToken(const TWideToken& tok, NLP_TYPE type, const TWtringBuf& src);
        void OnEnd();
    private:
        void AddSpace(TToken tok, bool misc = false) {
            if (!tok.TotalLength()) {
                return;
            }
            tok.SentId = SentId;
            tok.Space = true;
            if (misc && LastIsMiscSpace && !Tokens.empty() && !Tokens.back().SuffixLength && Tokens.back().Src.data() + Tokens.back().Src.size() == tok.Src.data()) {
                //glue consecutive misctexts
                Tokens.back().Src = TWtringBuf(Tokens.back().Src.data(), tok.Src.data() + tok.Src.size());
                Tokens.back().BodyLength += tok.BodyLength;
            } else {
                Tokens.push_back(tok);
            }
            LastIsMiscSpace = misc;
        }
        void AddTok(TToken tok, bool goRight = false) {
            if (!tok.BodyLength) {
                return AddSpace(tok);
            }
            tok.SentId = SentId;
            ++WordsInSent[SentId];
            tok.Space = false;
            Tokens.push_back(tok);
            WordIdx.push_back(Tokens.size() - 1);
            GoRight.push_back(goRight);
            ForcedGoRight.push_back(false);
            LastIsMiscSpace = false;
        }
        size_t SentId = 0;
        TVector<size_t> WordsInSent;
    };
}

struct THiliteTokenHandler : ITokenHandler {
    NHighlighter::TText* Text = nullptr;
    TWtringBuf Src;
    THiliteTokenHandler(NHighlighter::TText* text, const TWtringBuf& src)
      : Text(text)
      , Src(src)
    {
    }
    void OnToken(const TWideToken& tok, size_t origLen, NLP_TYPE type) override {
        TWtringBuf cur(Src.data(), Src.data() + origLen);
        Src.Skip(origLen);
        return Text->AddToken(tok, type, cur);
    }
    void OnEnd() {
        Text->OnEnd();
    }
};

void NHighlighter::TText::AddToken(const TWideToken& tok, NLP_TYPE type, const TWtringBuf& src) {
    size_t offset = TokenPool.size();

    TokenPool.append(tok.Token, tok.Leng);

    const size_t beg = Tokens.size();

    switch (type) {
        case NLP_INTEGER:
        case NLP_MARK:
        case NLP_FLOAT:
        case NLP_WORD:
            // This whole stuff is not multitoken-aware. Will redo later.
            Y_ASSERT(!tok.SubTokens.empty());
            AddSpace(TToken(offset, tok.SubTokens[0].Pos, src));
            if (tok.SubTokens.size() <= 1) {
                const TCharSpan& span = tok.SubTokens[0];
                AddTok(TToken(offset + span.Pos, span.Len, src, span.SuffixLen));
            } else {
                const size_t last = tok.SubTokens.size() - 1;
                for (size_t i = 0; i < last; ++i) {
                    const TCharSpan& span = tok.SubTokens[i];
                    const TCharSpan& nextspan = tok.SubTokens[i + 1];
                    AddTok(TToken(offset + span.Pos, span.Len, src, span.SuffixLen), true);
                    const size_t delimPos = span.EndPos() + span.SuffixLen;
                    AddSpace(TToken(offset + delimPos, nextspan.Pos - delimPos, src));
                }
                {
                    const TCharSpan& lastspan = tok.SubTokens.back(); // Add last token with potential suffix
                    AddTok(TToken(offset + lastspan.Pos, lastspan.Len, src, lastspan.SuffixLen));
                }
            }
            break;
        case NLP_SENTBREAK:
        case NLP_PARABREAK:
            AddSpace(TToken(offset, tok.Leng, src));
            ++SentId;
            WordsInSent.push_back(0);
            break;
        default:
            AddSpace(TToken(offset, tok.Leng, src), type == NLP_MISCTEXT);
            break;
    }
    const size_t end = Tokens.size();
    if (beg < end) {
        ZonesTransformer.OnTokens(beg, end - 1, src);
    }
}
void NHighlighter::TText::OnEnd() {
    const auto it = ZonesTransformer.DstZones.find(TZonedString::ZONE_FIO);
    if (it == ZonesTransformer.DstZones.end()) {
        return;
    }
    const TZonesTransformer::TSpans& z = it->second.Spans;
    for (size_t i = 0; i < z.size(); ++i) {
        for (size_t j = z[i].first; j <= z[i].second; ++j) {
            GetToken(j).FioPosId = i + 1;
        }
    }
}

class TInlineHighlighter::TImpl {
private:
    friend struct TPainterImpl;
    NHighlighter::TQueryGraph Graph;
public:
    TImpl();
    TIPainterPtr GetPainter() const;
    void AddRequest(const TRichRequestNode& richtree, const THiliteMark* marks = nullptr);
    bool IsGoodWord(const TWideToken& tok) const;
    bool IsGoodWord(const wchar16* word, size_t leng) const;
    bool Empty() const;
};

TInlineHighlighter::TImpl::TImpl()
  : Graph()
{
}

void TInlineHighlighter::AddRequest(const TRichRequestNode& richtree, const THiliteMark* marks, bool additional) {
    if (!additional && !Impl->Empty()) {
        return;
    }
    if (!marks) {
        marks = &DEFAULT_MARKS;
    }
    Impl->AddRequest(richtree, marks);
}
void TInlineHighlighter::TImpl::AddRequest(const TRichRequestNode& richtree, const THiliteMark* marks) {
    Graph.AddRequest(richtree, marks);
}
void NHighlighter::TQueryGraph::AddRequest(const TRichRequestNode& richtree, const THiliteMark* marks) {
    TWalkerState state;
    state.Parent = this;
    state.Marks = marks;
    state.LeftId = GetNewEdgeId();
    state.RightId = GetNewEdgeId();
    state.InQuotes = false;
    state.InExtension = TE_NONE;
    state.SingleExtension = false;
    WalkRequest(richtree, state);
}
void NHighlighter::TQueryGraph::AddWordNode(const TWordNode* node, EHiliteType hlType, bool wasQuoted, bool isAttr, TWalkerState& state) {
    TWordNodeModel w;
    ParseWordNode(w, node, hlType, wasQuoted, isAttr, state);
    state.AddedNodes.push_back(ConsumeWordNode(w));
    if (state.InExtension != TE_NONE && !state.SingleExtension && node->IsStopWord() && hlType == HILITE_SELF) {
        StopwordsToHack.insert(state.AddedNodes.back());
    }
}
void NHighlighter::TQueryGraph::ParseWordNode(TWordNodeModel& w, const TWordNode* node, EHiliteType hlType, bool wasQuoted, bool isAttr, TWalkerState& state) {
    w.HiliteType = hlType;
    w.Quoted = state.InQuotes;
    w.QuotedAlone = w.Quoted && !wasQuoted;
    w.Mark = state.Marks;
    w.LeftId = state.LeftId;
    w.RightId = state.RightId;
    w.IsAttr = isAttr;
    if (node->IsLemmerWord()) {
        for (const TLemmaForms& lemma : node->GetLemmas()) {
            for (const auto& lemmaForm : lemma.GetForms()) {
                w.Forms.push_back(TForm(lemmaForm.first, lemma.GetLanguage()));
            }
        }
    }
    {
        TUtf16String token(node->GetNormalizedForm());
        token.to_lower();
        w.Forms.push_back(TForm(token, TLangMask()));
    }
    for (const TForm& form : w.Forms) {
        w.Langs |= form.Lang;
    }
}
NHighlighter::TWordNodeModel* NHighlighter::TQueryGraph::ConsumeWordNode(const TWordNodeModel& w) {
    Graph.push_back(w);
    for (const TForm& form : Graph.back().Forms) {
        LangMask |= form.Lang;
        auto& lookupVal = Lookup[form.Form];
        lookupVal.Words.push_back(&Graph.back());
        lookupVal.WordLangMask |= w.Langs;
    }
    return &Graph.back();
}
void NHighlighter::TQueryGraph::ConsumeFioWordNode(const TWordNodeModel& w) {
    if (w.Forms.size() != 1) {
        return;
    }
    const TForm& form = w.Forms[0];
    TLookup::insert_ctx insertCtx;
    if (auto it = Lookup.find(form.Form, insertCtx); it == Lookup.end() || !it->second.IsFio) {
        FioGraph.push_back(w);
        //TLookup uses TWtringBuf as key so we have to use the copy we own as a source for the key
        const TWordNodeModel& localW = FioGraph.back();
        if (it == Lookup.end()) {
            it = Lookup.emplace_direct(insertCtx, localW.Forms[0].Form, TLookupVal());
        }
        auto& lookupVal = it->second;

        LangMask |= form.Lang;
        lookupVal.Words.push_back(&localW);
        lookupVal.WordLangMask |= form.Lang;
        lookupVal.IsFio = true;
        HasFioWord = true;
    }
}
inline constexpr TWtringBuf fioname{u"fioname"};
inline constexpr TWtringBuf fiinoinname{u"fiinoinname"};
inline constexpr TWtringBuf finame{u"finame"};
inline constexpr TWtringBuf fiinname = {u"fiinname"};

static inline bool IsFioname(const TUtf16String& s) {
    return s == fioname;
}

static inline bool IsFiinoinname(const TUtf16String& s) {
    return s == fiinoinname;
}

static inline bool IsFiname(const TUtf16String& s) {
    return s == finame;
}

static inline bool IsFiinname(const TUtf16String& s) {
    return s == fiinname;
}

static inline bool IsFioZone(const TUtf16String& s) {
    return IsFioname(s) || IsFiinoinname(s) || IsFiname(s) || IsFiinname(s);
}
static inline bool CutSuffix(TUtf16String& s, TWtringBuf suf) {
    if (!s.EndsWith(suf.data(), suf.size())) {
        return false;
    }
    s.resize(s.size() - suf.size());
    return true;
}
void NHighlighter::TQueryGraph::AddFIO(const TRichRequestNode& node, const TUtf16String& name, TWalkerState& state) {
    for (size_t i = 0; i < node.Children.size(); ++i) {
        if (!node.Children[i]->Children.empty() || !node.Children[i]->WordInfo.Get()) {
            return;
        }
    }
    TVector<TWtringBuf> origin;
    const size_t ch = node.Children.size();
    constexpr wchar16 fi[] = { 'f', 'i', 0 };
    constexpr wchar16 fo[] = { 'f', 'o', 0 };
    if ((IsFioname(name) || IsFiinoinname(name)) && ch == 3) {
        TWordNodeModel i;
        TWordNodeModel o;
        TWordNodeModel f;
        TWalkerChildrenState chState;
        InitChildrenState(chState, 3, state, false);
        ParseWordNode(i, node.Children[0]->WordInfo.Get(), HILITE_LEFT_OR_RIGHT, false, false, chState[0]);
        ParseWordNode(o, node.Children[1]->WordInfo.Get(), HILITE_LEFT_OR_RIGHT, false, false, chState[1]);
        ParseWordNode(f, node.Children[2]->WordInfo.Get(), HILITE_LEFT_OR_RIGHT, false, false, chState[2]);
        if (i.Forms.size() < 1 || !CutSuffix(i.Forms[0].Form, fi)) {
            return;
        }
        if (o.Forms.size() < 1 || !CutSuffix(o.Forms[0].Form, fo)) {
            return;
        }
        if (f.Forms.empty()) {
            return;
        }
        i.Hidden = o.Hidden = f.Hidden = true;
        i.Forms.resize(1);
        o.Forms.resize(1);
        f.Forms.resize(1);
        ConsumeFioWordNode(i);
        ConsumeFioWordNode(o);
        ConsumeFioWordNode(f);
        TFioInfo fio;
        fio.I = i.Forms[0].Form;
        fio.O = o.Forms[0].Form;
        fio.F = f.Forms[0].Form;
        fio.Id = GetFioId();
        if (fio.I.size() > 1 || fio.O.size() > 1) {
            HasLongFioName = true;
        }
        if ((fio.I.size() <= 1 || fio.O.size() <= 1) && IsFioname(name)) {
            HasShortFioName = true;
        }
        FioInfos.push_back(fio);
    } else if (ch == 2) {
        TWordNodeModel i;
        TWordNodeModel f;
        i.Hidden = f.Hidden = true;
        TWalkerChildrenState chState;
        InitChildrenState(chState, 2, state, false);
        ParseWordNode(i, node.Children[0]->WordInfo.Get(), HILITE_LEFT_OR_RIGHT, false, false, chState[0]);
        ParseWordNode(f, node.Children[1]->WordInfo.Get(), HILITE_LEFT_OR_RIGHT, false, false, chState[1]);
        if (i.Forms.size() < 1 || !CutSuffix(i.Forms[0].Form, fi)) {
            return;
        }
        if (f.Forms.empty()) {
            return;
        }
        i.Forms.resize(1);
        f.Forms.resize(1);
        ConsumeFioWordNode(i);
        ConsumeFioWordNode(f);
        TFioInfo fio;
        fio.I = i.Forms[0].Form;
        fio.F = f.Forms[0].Form;
        fio.Id = GetFioId();
        if (fio.I.size() > 1) {
            HasLongFioName = true;
        }
        if (fio.I.size() <= 1 && (IsFioname(name) || IsFiname(name))) {
            HasShortFioName = true;
        }
        FioInfos.push_back(fio);
    }
}
void NHighlighter::TQueryGraph::WalkRequest(const TRichRequestNode& node, TWalkerState& state) {
    if (node.Necessity == nMUSTNOT) {
        return;
    }
    for (const auto& child : node.MiscOps) {
        if (oZone == child->OpInfo.Op && IsFioZone(child->GetTextName())) {
            if (node.Children.size() == 1) {
                AddFIO(*node.Children[0].Get(), child->GetTextName(), state);
            } else {
                AddFIO(node, child->GetTextName(), state);
            }
            return;
        }
    }

    const bool sameEdges = !node.Children.empty() && (node.Op() == oOr || node.Op() == oWeakOr);
    const bool wasQuoted = state.InQuotes;
    state.InQuotes = state.InQuotes || node.IsQuoted();
    const bool isAttr = IsAttribute(node);

    TWalkerChildrenState chState;
    InitChildrenState(chState, node.Children.size(), state, sameEdges);

    if (node.Children.empty()) {
        if (node.WordInfo.Get()) {
            AddWordNode(node.WordInfo.Get(), node.GetHiliteType(), wasQuoted, isAttr, state);
        }
    } else {
        int i = 0;
        const int n = (int)node.Children.size();
        for (auto& child : node.Children) {
            WalkRequest(*child, chState[i]);
            state.AddedNodes.insert(state.AddedNodes.end(), chState[i].AddedNodes.begin(), chState[i].AddedNodes.end());
            i++;
        }
        if (state.InQuotes) {
            for (i = 0; i + 1 < n; ++i) {
                const TProximity& proxAfter = node.Children.ProxAfter(i);
                if (proxAfter.Beg == proxAfter.End && proxAfter.Level == BREAK_LEVEL) {
                    Jokers.push_back(TJoker(chState[i].RightId, chState[i + 1].LeftId, proxAfter.Beg));
                }
            }
        }
        if (!sameEdges) {
            for (i = 1; i < n; ++i) {
                const TProximity& proxBefore = node.Children.ProxBefore(i);
                if (proxBefore.Level == BREAK_LEVEL && proxBefore.Beg >= 1 && proxBefore.End <= 1 && (proxBefore.Beg || proxBefore.End)) {
                    Dom2Edges.insert(chState[i].LeftId);
                }
            }
        }
    }
    for (const auto& child : node.MiscOps) {
        if (oRefine == child->Op() || oRestrDoc == child->Op()) {//hilite <- and <<
            TWalkerState rState = state.Spawn();
            rState.InQuotes = false;
            rState.LeftId = GetNewEdgeId();
            rState.RightId = GetNewEdgeId();
            WalkRequest(*child, rState);
        }
    }

    for (TForwardMarkupIterator<TSynonym, true> j(node.Markup()); !j.AtEnd(); ++j) {
        const size_t l = j->Range.Beg;
        const size_t r = j->Range.End;
        TWalkerState sState = state.Spawn();
        sState.InQuotes = false;
        sState.InExtension |= j.GetData().GetType();
        sState.SingleExtension = l == r;
        TWordNodeModel* dst = nullptr;
        if (chState.empty()) {
            if (l || r) {
                continue;
            }
            if (!state.AddedNodes.empty()) {
                dst = state.AddedNodes[0];
            }
        } else {
            if (l >= chState.size() || r >= chState.size()) {
                continue;
            }
            sState.LeftId = chState[l].LeftId;
            sState.RightId = chState[r].RightId;
            if (l == r && chState[l].AddedNodes.size() == 1) {
                dst = chState[l].AddedNodes[0];
            }
        }
        WalkRequest(*j.GetData().SubTree, sState);
        if (dst && sState.AddedNodes.size() == 1) {
            SimpleSynonyms[dst].push_back(sState.AddedNodes[0]);
            SimpleSynonymsReversed[sState.AddedNodes[0]].push_back(dst);
        }
    }
    ShiftFioId();
}
bool NHighlighter::TQueryGraph::HasNode(const TWtringBuf& tok) const {
    auto it = Lookup.find(tok);
    if (it == Lookup.end()) {
        return false;
    }
    for (const auto& word : it->second.Words) {
        if (word->HiliteType != HILITE_NONE) {
            return true;
        }
    }
    return false;
}
bool NHighlighter::TQueryGraph::Empty() const {
    return Graph.empty();
}
bool TInlineHighlighter::TImpl::Empty() const {
    return Graph.Empty();
}

bool TInlineHighlighter::IsGoodWord(const TWideToken& tok) const {
    return Impl->IsGoodWord(tok);
}
bool TInlineHighlighter::TImpl::IsGoodWord(const TWideToken& tok) const {
    TUtf16String low(tok.Token, tok.Leng);
    low.to_lower();
    if (IsGoodWord(low.data(), tok.Leng))
        return true;

    if (!tok.SubTokens.empty() && tok.SubTokens.back().EndPos() < tok.Leng) {
        // Retry without suffix
        return IsGoodWord(low.data(), tok.SubTokens.back().EndPos());
    }
    return false;
}

bool TInlineHighlighter::TImpl::IsGoodWord(const wchar16* word, size_t leng) const {
    return Graph.HasNode(TWtringBuf(word, leng));
}

struct TPainterImpl : TInlineHighlighter::IPainter {
    const TInlineHighlighter::TImpl* IH;
    struct TJob : TIntrusiveListItem<TJob> {
        TUtf16String& SourceText;
        TZonedString* SourceZonedString = nullptr;
        NHighlighter::TText TokenizedText;
    public:
        TJob(TUtf16String& text)
            : SourceText(text)
        {
        }
        TJob(TZonedString& text)
            : SourceText(text.String)
            , SourceZonedString(&text)
            , TokenizedText(&text)
        {
        }
    };
    TIntrusiveListWithAutoDelete<TJob, TDelete> Jobs;

    TPainterImpl(const TInlineHighlighter::TImpl* ih)
      : IH(ih)
      , Jobs()
    {
    }
    void AddJob(TUtf16String& text) override {
        Jobs.PushBack(new TJob(text));
    }
    void AddJob(TZonedString& text) override {
        Jobs.PushBack(new TJob(text));
    }
    void DropJobs() override {
        Jobs.Clear();
        ResetState();
    }
    void Paint() override {
        PrepareFio();
        for (TJob& job : Jobs) {
            THiliteTokenHandler t(&job.TokenizedText, job.SourceText);
            TNlpTokenizer toker(t, !Options.TokHl);
            toker.Tokenize(job.SourceText.data(), job.SourceText.size());
            t.OnEnd();
        }
        for (TJob& job : Jobs) {
            GetForms(job.TokenizedText);
            GetEdges(job.TokenizedText);
            GetFio(job.TokenizedText, Options.UseFioZones, Options.UseFioZones2);
        }
        for (TJob& job : Jobs) {
            Paint(job.TokenizedText);
        }
        for (TJob& job : Jobs) {
            TVector<NHighlighter::TMarkedZone> markZones;
            TZonedString localResult(job.SourceText);
            TZonedString& result = job.SourceZonedString ? *job.SourceZonedString : localResult;
            if (Options.SrcOutput) {
                OutputSrcs(job.TokenizedText, markZones);
            } else {
                Output(job.TokenizedText, result.String, markZones);
            }
            SaveZonedOutput(result, markZones);
            if (!job.SourceZonedString) {
                job.SourceText = NSnippets::MergedGlue(result);
            }
        }
        DropJobs();
    }
private:
    THashMap<const NHighlighter::TWordNodeModel*, THashMap<const NHighlighter::TFioInfo*, size_t>> Mm;
    THashSet<size_t> FioIdsSeen;
    THashSet<const NHighlighter::TWordNodeModel*> ShadowedByFio;
    THashSet<size_t> PowerEdgesSeen;
    THashSet<size_t> EdgesSeen;
    void ResetState() {
        Mm.clear();
        FioIdsSeen.clear();
        ShadowedByFio.clear();
        PowerEdgesSeen.clear();
        EdgesSeen.clear();
    }
    void PrepareFio() {
        for (const NHighlighter::TFioInfo& fioInfo : IH->Graph.FioInfos) {
            const TUtf16String* const parts[] = {&fioInfo.F, &fioInfo.I, &fioInfo.O};
            for (size_t i = 0; i < 3; ++i) {
                const auto fio = IH->Graph.Lookup.find(*parts[i]);
                if (fio == IH->Graph.Lookup.end()) {
                    continue;
                }
                for (const NHighlighter::TWordNodeModel* word : fio->second.Words) {
                    Mm[word][&fioInfo] |= (size_t(1) << i);
                    if (IH->Graph.SimpleSynonyms.contains(word)) {
                        for (const NHighlighter::TWordNodeModel* synWord : IH->Graph.SimpleSynonyms.find(word)->second) {
                            Mm[synWord][&fioInfo] |= (size_t(1) << i);
                        }
                    }
                }
            }
        }
    }
    size_t FioMatch(const NHighlighter::TText& text, const NHighlighter::TToken& tok, const NHighlighter::TWordNodeModel* word, const NHighlighter::TFioInfo* fio) const {
        bool wantUpper = tok.TotalLength() == 1;
        auto mm = Mm.find(word);
        if (mm == Mm.end()) {
            return 0;
        }
        auto mms = mm->second.find(fio);
        if (mms == mm->second.end()) {
            return 0;
        }
        size_t all = mms->second;
        if (wantUpper) {
            wchar16 c = text.TokenPool[tok.Offset];
            if (ToUpper(c) != c) {
                all &= 1;
            }
        }
        return all;
    }
    struct TFioOccurence {
        NHighlighter::TText& Text;
        size_t FioId;
        struct TPart {
            size_t Idx;
            const NHighlighter::TWordNodeModel* Word;
            TPart()
              : Idx()
              , Word()
            {
            }
            TPart(size_t idx, const NHighlighter::TWordNodeModel* word)
              : Idx(idx)
              , Word(word)
            {
            }
        };
        TVector<TPart> Parts;
        TFioOccurence(NHighlighter::TText& text)
          : Text(text)
          , FioId()
          , Parts()
        {
        }
    };
    void OnFioOccurence(const TFioOccurence& fio) {
        if (!Options.PaintFios) {
            return;
        }
        bool newId = !FioIdsSeen.contains(fio.FioId);
        FioIdsSeen.insert(fio.FioId);
        for (const auto& part : fio.Parts) {
            const THiliteMark*& mark = fio.Text.GetWord(part.Idx).Result;
            if (!mark) {
                mark = part.Word->Mark;
            }
        }
        if (newId) {
            for (const NHighlighter::TFioInfo& fioInfo : IH->Graph.FioInfos) {
                if (fioInfo.Id != fio.FioId) {
                    continue;
                }
                const TUtf16String* const parts[] = {&fioInfo.F, &fioInfo.I, &fioInfo.O};
                for (size_t i = 0; i < 3; ++i) {
                    if (parts[i]->empty()) {
                        continue;
                    }
                    for (TWordIterator it(IH->Graph, fio.Text, *parts[i], !Options.SkipAttrs, true, true); !it.Empty(); ++it) {
                        ShadowedByFio.insert(*it);
                    }
                }
            }
        }
    }
    bool IsFioPair(const NHighlighter::TText& text, size_t i1, size_t i2, bool useFioZones, bool useFioZones2) {
        if (i1 >= text.WordCount() || i2 >= text.WordCount()) {
            return false;
        }
        const NHighlighter::TToken& t1 = text.GetWord(i1);
        const NHighlighter::TToken& t2 = text.GetWord(i2);
        if (t1.SentId != t2.SentId) {
            return false;
        }
        if (useFioZones) {
            return t1.FioPosId == t2.FioPosId;
        }
        if (!useFioZones2) {
            return true;
        }
        if (t1.FioPosId && t2.FioPosId) {
            return t1.FioPosId == t2.FioPosId;
        }
        if (!t1.FioPosId && !t2.FioPosId) {
            return true;
        }
        if (!t1.FioPosId) {
            return t1.TotalLength() > 1;
        } else {
            return t2.TotalLength() > 1;
        }
    }
    void GetFio(NHighlighter::TText& text, bool useFioZones, bool useFioZones2) {
        TFioOccurence fio(text);
        for (size_t idx = 0; idx != text.WordCount(); ++idx) {
            for (TWordIterator jt(IH->Graph, text, idx, !Options.SkipAttrs, true, true); !jt.Empty(); ++jt) {
                auto mmj = Mm.find(*jt);
                if (mmj == Mm.end()) {
                    continue;
                }
                for (const auto& item : mmj->second) {
                    const NHighlighter::TFioInfo* fioInfo = item.first;
                    size_t all = FioMatch(text, text.GetWord(idx), *jt, fioInfo);
                    if (!all) {
                        continue;
                    }
                    fio.Parts.push_back(TFioOccurence::TPart(idx, *jt));
                    for (size_t d = 1; d <= 2; ++d) {
                        if (!IsFioPair(text, idx, idx + d, useFioZones, useFioZones2)) {
                            continue;
                        }
                        for (TWordIterator njt(IH->Graph, text, idx + d, !Options.SkipAttrs, true, true); !njt.Empty(); ++njt) {
                            auto mm = Mm.find(*njt);
                            if (mm == Mm.end()) {
                                continue;
                            }
                            auto mms = mm->second.find(fioInfo);
                            if (mms == mm->second.end()) {
                                continue;
                            }
                            size_t cur = FioMatch(text, text.GetWord(idx + d), *njt, mms->first);
                            if (!cur) {
                                continue;
                            }
                            fio.Parts.push_back(TFioOccurence::TPart(idx + d, *njt));
                            all |= cur;
                        }
                    }
                    if ((all & 3) == 3 && fio.Parts.size() > 1) {
                        fio.FioId = fioInfo->Id;
                        OnFioOccurence(fio);
                    }
                    fio.Parts.clear();
                }
            }
        }
    }
    class TWordIterator {
    private:
        const NHighlighter::TQueryGraph& Graph;
        bool Void = true;
        using TGraphLookup = TVector<const NHighlighter::TWordNodeModel*>::const_iterator;
        TGraphLookup Cur = TGraphLookup();
        TGraphLookup End = TGraphLookup();
        using TWordLookup = TVector<const NHighlighter::TWordNodeModel*>::const_iterator;
        TWordLookup SynCur = TWordLookup();
        TWordLookup SynEnd = TWordLookup();
        TWordLookup RevSynCur = TWordLookup();
        TWordLookup RevSynEnd = TWordLookup();
        bool SeeAttrs = false;
        bool SeeHidden = false;
        bool SeeSynonyms = false;
        bool OnSynonym = false;
        bool OnRevSynonym = false;
    public:
        TWordIterator(const NHighlighter::TQueryGraph& graph, const NHighlighter::TText& text, size_t idx, bool seeAttr, bool seeHidden, bool seeSynonyms)
          : Graph(graph)
          , SeeAttrs(seeAttr)
          , SeeHidden(seeHidden)
          , SeeSynonyms(seeSynonyms)
        {
            if (idx >= +text.WordCount()) {
                return;
            }
            const NHighlighter::TToken& tok = text.GetWord(idx);
            const auto& it = tok.LookupIt;
            if (it == Graph.Lookup.end()) {
                return;
            }
            Void = false;
            Cur = it->second.Words.begin();
            End = it->second.Words.end();
            Shift();
        }
        TWordIterator(const NHighlighter::TQueryGraph& graph, const NHighlighter::TText& /*text*/, const TUtf16String& word, bool seeAttr, bool seeHidden, bool seeSynonyms)
          : Graph(graph)
          , SeeAttrs(seeAttr)
          , SeeHidden(seeHidden)
          , SeeSynonyms(seeSynonyms)
        {
            const auto it = graph.Lookup.find(word);
            if (it == Graph.Lookup.end()) {
                return;
            }
            Void = false;
            Cur = it->second.Words.begin();
            End = it->second.Words.end();
            Shift();
        }
        bool Empty() {
            return Void || Cur == End;
        }
        bool Hidden() const {
            return operator*()->Hidden;
        }
        bool Synonym() const {
            return OnSynonym;
        }
        bool RevSynonym() const {
            return OnRevSynonym;
        }
        void operator++() {
            if (OnRevSynonym) {
                ++RevSynCur;
            } else if (OnSynonym) {
                ++SynCur;
            } else {
                OnSynonym = true;
            }
            while (true) {
                if (OnRevSynonym) {
                    ShiftRevSyn();
                    if (RevSynCur == RevSynEnd) {
                        OnRevSynonym = false;
                        ++Cur;
                        Shift();
                    }
                } else if (OnSynonym) {
                    ShiftSyn();
                    if (SynCur == SynEnd) {
                        OnSynonym = false;
                        OnRevSynonym = true;
                        continue;
                    }
                } else {
                    if (Cur < End) {
                        OnSynonym = true;
                        continue;
                    }
                }
                break;
            }
        }
        const NHighlighter::TWordNodeModel* operator*() const {
            return OnRevSynonym ? *RevSynCur : OnSynonym ? *SynCur : *Cur;
        }
        void Shift() {
            while (Cur < End && ((*Cur)->Hidden && !SeeHidden || (*Cur)->IsAttr && !SeeAttrs)) {
                ++Cur;
            }
            if (SeeSynonyms && Cur < End) {
                auto syn = Graph.SimpleSynonyms.find(*Cur);
                if (syn != Graph.SimpleSynonyms.end()) {
                    SynCur = syn->second.begin();
                    SynEnd = syn->second.end();
                }
                auto revSyn = Graph.SimpleSynonymsReversed.find(*Cur);
                if (revSyn != Graph.SimpleSynonymsReversed.end()) {
                    RevSynCur = revSyn->second.begin();
                    RevSynEnd = revSyn->second.end();
                }
            }
        }
        void ShiftSyn() {
            while (SynCur < SynEnd && ((*SynCur)->Hidden && !SeeHidden || (*SynCur)->IsAttr && !SeeAttrs)) {
                ++SynCur;
            }
        }
        void ShiftRevSyn() {
            while (RevSynCur < RevSynEnd && ((*RevSynCur)->Hidden && !SeeHidden || (*RevSynCur)->IsAttr && !SeeAttrs)) {
                ++RevSynCur;
            }
        }
    };
    bool HasEdge(const NHighlighter::TText& text, size_t idx, size_t id) const {
        for (TWordIterator it(IH->Graph, text, idx, !Options.SkipAttrs, false, false); !it.Empty(); ++it) {
            if ((*it)->LeftId == id) {
                return true;
            }
        }
        return false;
    }
    bool HasEdgeRev(const NHighlighter::TText& text, size_t idx, size_t id) const {
        for (TWordIterator it(IH->Graph, text, idx, !Options.SkipAttrs, false, false); !it.Empty(); ++it) {
            if ((*it)->RightId == id) {
                return true;
            }
        }
        return false;
    }
    void LookEdge(const NHighlighter::TText& text, size_t idx, size_t id) {
        if (HasEdge(text, idx, id)) {
            EdgesSeen.insert(id);
            if (Options.PaintClosePositionsSmart && IH->Graph.QuoteEdges.contains(id)) {
                PowerEdgesSeen.insert(id);
            }
            if (Options.PaintAbbrevSmart && IH->Graph.AbbrevEdges.contains(id)) {
                PowerEdgesSeen.insert(id);
            }
            if (Options.PaintDom2Smart && IH->Graph.Dom2Edges.contains(id)) {
                PowerEdgesSeen.insert(id);
            }
        }
    }

    void OnJokerOccurence(NHighlighter::TText& text, size_t l, size_t r, size_t id, const NHighlighter::TWordNodeModel* leftWord) {
        LookEdge(text, r, id);
        for (size_t d = l; d <= r; ++d) {
            if (!text.GetWord(d).Result) {
                text.GetWord(d).Result = leftWord->Mark;
            }
        }
    }
    void GetEdges(NHighlighter::TText& text) {
        for (size_t idx = 0; idx != text.WordCount(); ++idx) {
            for (TWordIterator jt(IH->Graph, text, idx, !Options.SkipAttrs, false, false); !jt.Empty(); ++jt) {
                if (idx + 1 < text.WordCount() && text.MayStickRight(idx)) {
                    LookEdge(text, idx + 1, (*jt)->RightId);
                }
                for (const NHighlighter::TJoker& joker : IH->Graph.Jokers) {
                    if (joker.LeftId == (*jt)->RightId) {
                        if (HasEdge(text, idx + joker.Count, joker.RightId)) {
                            OnJokerOccurence(text, idx, idx + joker.Count, joker.RightId, *jt);
                        }
                    }
                }
            }
        }
    }
    void OutputSpaces(TUtf16String& res, const wchar16* s, size_t len) {
        if (Options.TrustPunct) {
            res.append(s, len);
            return;
        }
        size_t beg = res.size();
        for (size_t i = 0; i < len; ) {
            size_t cnt = 1;
            for (; i + cnt < len; ++cnt) {
                if (s[i + cnt] != s[i]) {
                    break;
                }
            }
            const wchar16 c = s[i];
            i += cnt;
            if (cnt >= 6) {
                cnt = 1;
            }
            for (; cnt; --cnt) {
                res += c;
            }
        }
        if (res.size() - beg > 11) {
            Copy(res.begin() + res.size() - 3, res.begin() + res.size(), res.begin() + beg + 8);
            res[beg + 3] = ' '; res[beg + 7] = ' ';
            res[beg + 4] = '.'; res[beg + 5] = '.'; res[beg + 6] = '.';
            res.resize(beg + 11);
        }
    }
    void OutputMarkBegin(size_t pos, const THiliteMark* mark, TVector<NHighlighter::TMarkedZone>& markZones) {
        if (mark) {
            markZones.emplace_back();
            markZones.back().Mark = mark;
            markZones.back().BeginOfs = pos;
        }
    }
    void OutputMarkEnd(size_t pos, const THiliteMark* mark, TVector<NHighlighter::TMarkedZone>& markZones) {
        if (mark && markZones) {
            markZones.back().EndOfs = pos;
        }
    }
    void Output(const NHighlighter::TText& text, TUtf16String& res, TVector<NHighlighter::TMarkedZone>& markZones) {
        res.clear();
        for (size_t i = 0; i != text.TokenCount(); ++i) {
            const NHighlighter::TToken& tok = text.GetToken(i);
            const THiliteMark* const mark = tok.Result;
            OutputMarkBegin(res.length(), mark, markZones);
            if (!tok.Space) {
                if (!tok.SuffixedLookup) {
                    res.append(text.TokenPool.data() + tok.Offset, tok.TotalLength());
                } else {
                    res.append(text.TokenPool.data() + tok.Offset, tok.BodyLength);
                    OutputMarkEnd(res.length(), mark, markZones);
                    res.append(text.TokenPool.data() + tok.Offset + tok.BodyLength, tok.TotalLength() - tok.BodyLength);
                    continue;
                }
            } else {
                OutputSpaces(res, text.TokenPool.data() + tok.Offset, tok.TotalLength());
            }
            OutputMarkEnd(res.length(), mark, markZones);
        }
    }
    void OutputSrcs(const NHighlighter::TText& text, TVector<NHighlighter::TMarkedZone>& markZones) {
        size_t pos = 0;
        for (size_t i = 0; i != text.TokenCount();) {
            TWtringBuf src = text.GetToken(i).Src;
            const THiliteMark* anyMark = text.GetToken(i).Result;
            size_t j = i;
            size_t offset = 0;
            bool mismatch = false;
            for (; j != text.TokenCount(); ++j) {
                const NHighlighter::TToken& tok = text.GetToken(j);
                if (tok.Src.data() != src.data()) {
                    break;
                }
                if (!anyMark && tok.Result) {
                    anyMark = tok.Result;
                }
                if (!mismatch) {
                    if (tok.TotalLength() + offset > src.size() || TWtringBuf(src.data() + offset, tok.TotalLength()) != TWtringBuf(text.TokenPool.data() + tok.Offset, tok.TotalLength())) {
                        mismatch = true;
                    } else {
                        offset += tok.TotalLength();
                    }
                }
            }
            if (offset != src.size()) {
                mismatch = true;
            }
            if (mismatch) {
                OutputMarkBegin(pos, anyMark, markZones);
                pos += src.size();
                OutputMarkEnd(pos, anyMark, markZones);
                i = j;
            } else {
                for (; i != j; ++i) {
                    const NHighlighter::TToken& tok = text.GetToken(i);
                    const THiliteMark* const mark = tok.Result;
                    OutputMarkBegin(pos, mark, markZones);
                    const bool nosuffix = !tok.Space && tok.SuffixedLookup;
                    pos += tok.BodyLength;
                    if (nosuffix) {
                        OutputMarkEnd(pos, mark, markZones);
                    }
                    pos += tok.TotalLength() - tok.BodyLength;
                    if (!nosuffix) {
                        OutputMarkEnd(pos, mark, markZones);
                    }
                }
            }
        }
    }
    void SaveZonedOutput(TZonedString& result, const TVector<NHighlighter::TMarkedZone>& markZones) {
        result.Zones.clear();
        const int baseZoneIndex = +TZonedString::ZONE_UNKNOWN;
        for (const NHighlighter::TMarkedZone& z : markZones) {
            TZonedString::TZone* zone = nullptr;
            for (int zoneIndex = baseZoneIndex; ; ++zoneIndex) {
                zone = &result.Zones[zoneIndex]; // find or add
                if (!zone->Mark) { // just added
                    zone->Mark = z.Mark;
                    break;
                } else if (zone->Mark == z.Mark) {
                    break;
                }
            }
            TWtringBuf span(result.String.data() + z.BeginOfs, result.String.data() + z.EndOfs);
            zone->Spans.push_back(TZonedString::TSpan(span));
        }
    }
    void Paint(NHighlighter::TText& text) {
        for (const auto& dstZone : text.GetZonesTransformer().DstZones) {
            if (!dstZone.second.Mark) {
                continue;
            }
            for (const auto& span : dstZone.second.Spans) {
                for (size_t i = span.first; i <= span.second; ++i) {
                    NHighlighter::TToken& token = text.GetToken(i);
                    if (!token.Space) {
                        token.Result = dstZone.second.Mark;
                    }
                }
            }
        }
        for (size_t idx = 0; idx != text.WordCount(); ++idx) {
            for (TWordIterator word(IH->Graph, text, idx, !Options.SkipAttrs, false, false); !word.Empty(); ++word) {
                EHiliteType hl = (*word)->HiliteType;
                if (Options.HackStopwords && IH->Graph.StopwordsToHack.contains(*word)) {
                    hl = HILITE_LEFT_OR_RIGHT;
                }
                //XXX: single letter bug
                bool fio = ShadowedByFio.contains(*word) || text.GetWord(idx).FioInitialLookup;
                bool done = false;
                if (hl == HILITE_SELF && !PowerEdgesSeen.contains((*word)->LeftId) && !PowerEdgesSeen.contains((*word)->RightId)) {
                    if (!fio || !Options.SmartUnpaintFios) {
                        text.GetWord(idx).Result = (*word)->Mark;
                        done = true;
                    }
                }
                if (!fio && idx + 1 < text.WordCount() && text.MayStickRight(idx) && HasEdge(text, idx + 1, (*word)->RightId)) {
                    text.GetWord(idx + 1).Result = text.GetWord(idx).Result = (*word)->Mark;
                    done = true;
                }
                if (!fio && idx > 0 && text.MayStickRight(idx - 1) && HasEdgeRev(text, idx - 1, (*word)->LeftId)) {
                    text.GetWord(idx - 1).Result = text.GetWord(idx).Result = (*word)->Mark;
                    done = true;
                }
                if (done) {
                    for (size_t i = idx; text.WordGoesRight(i, Options.WholeTokens) && !text.GetWord(i + 1).Result; ++i) {
                        text.GetWord(i + 1).Result = (*word)->Mark;
                    }
                    for (size_t i = idx; i && text.WordGoesRight(i - 1, Options.WholeTokens) && !text.GetWord(i - 1).Result; --i) {
                        text.GetWord(i - 1).Result = (*word)->Mark;
                    }
                }
            }
        }
    }
    void GetForms(NHighlighter::TText& text) {
        TUtf16String lowcase;
        TUtf16String conv;
        for (size_t i = 0; i != text.WordCount(); ++i) {
            text.GetWord(i).LookupIt = IH->Graph.Lookup.end();
        }
        for (size_t i = 0; i != text.WordCount(); ++i) {
            GetForm(text, i, lowcase, conv);
        }
    }
    bool NormalizeLowerWord(ELanguage langId, const TUtf16String& lowcase, TUtf16String& conv) {
        size_t bufSize = Max<size_t>(lowcase.size() * 2, 64);
        conv.resize(bufSize);
        NLemmer::TConvertRet r = NLemmer::GetAlphaRules(langId)->Normalize(lowcase.data(), lowcase.size(), conv.begin(), bufSize);
        if (!r.Valid) {
            return false;
        }
        conv.resize(r.Length);
        return true;
    }
    bool CheckForm(TUtf16String& lowcase, TUtf16String& conv, NHighlighter::TQueryGraph::TLookup::const_iterator& foundIt) {
        lowcase.to_lower();
        auto it = IH->Graph.Lookup.find(lowcase);
        if (it != IH->Graph.Lookup.end()) {
            foundIt = it;
            return true;
        }
        if (NormalizeLowerWord(LANG_UNK, lowcase, conv) && conv != lowcase) {
            it = IH->Graph.Lookup.find(conv);
            if (it != IH->Graph.Lookup.end()) {
                foundIt = it;
                return true;
            }
        }
        for (ELanguage langId : IH->Graph.LangMask) {
            if (NormalizeLowerWord(langId, lowcase, conv) && conv != lowcase) {
                it = IH->Graph.Lookup.find(conv);
                if (it != IH->Graph.Lookup.end()) {
                    if (it->second.WordLangMask.Test(langId)) {
                        foundIt = it;
                        return true;
                    }
                }
            }
        }
        return false;
    }
    void GetForm(NHighlighter::TText& text, size_t idx, TUtf16String& lowcase, TUtf16String& conv) {
        NHighlighter::TToken& tok = text.GetWord(idx);
        // check main token
        lowcase.AssignNoAlias(text.TokenPool.data() + tok.Offset, tok.TotalLength());
        if (CheckForm(lowcase, conv, tok.LookupIt)) {
            return;
        }
        // check token with it's suffix (C++, Google+)
        if (tok.SuffixLength) {
            lowcase.AssignNoAlias(text.TokenPool.data() + tok.Offset, tok.BodyLength);
            if (CheckForm(lowcase, conv, tok.LookupIt)) {
                tok.SuffixedLookup = true;
                return;
            }
        }
        // check joined tokens
        if (Options.GlueTok && (text.WordGoesRight(idx, true) || (Options.GlueTokHack && text.KindOfApostropheAfterWord(idx)))) {
            NHighlighter::TToken& nextTok = text.GetWord(idx + 1);
            lowcase.AssignNoAlias(text.TokenPool.data() + tok.Offset, tok.TotalLength());
            lowcase.append(text.TokenPool.data() + nextTok.Offset, nextTok.TotalLength());
            if (CheckForm(lowcase, conv, tok.LookupIt)) {
                text.MakeWordGoRight(idx);
                nextTok.LookupIt = tok.LookupIt;
                return;
            }
        }
        // check fio initials (first letter)
        if (tok.FioPosId && IH->Graph.HasFioWord && !IH->Graph.HasLongFioName && IH->Graph.HasShortFioName) {
            wchar16 lowInitial = ToLower(text.TokenPool[tok.Offset]);
            auto it = IH->Graph.Lookup.find(TWtringBuf(&lowInitial, 1));
            if (it != IH->Graph.Lookup.end() && it->second.IsFio) {
                tok.LookupIt = it;
                tok.FioInitialLookup = true;
                return;
            }
        }
    }
};

TInlineHighlighter::TInlineHighlighter()
{
    Impl.Reset(new TImpl());
}

TInlineHighlighter::~TInlineHighlighter()
{
}

TInlineHighlighter::TIPainterPtr TInlineHighlighter::GetPainter() const {
    return Impl->GetPainter();
}
TInlineHighlighter::TIPainterPtr TInlineHighlighter::TImpl::GetPainter() const {
    return TIPainterPtr(new TPainterImpl(this));
}

void TInlineHighlighter::PaintPassages(TUtf16String& passage, const TPaintingOptions& options) const {
    TIPainterPtr p = GetPainter();
    p->Options = options;
    p->AddJob(passage);
    p->Paint();
}
void TInlineHighlighter::PaintPassages(TZonedString& passage, const TPaintingOptions& options) const {
    TIPainterPtr p = GetPainter();
    p->Options = options;
    p->AddJob(passage);
    p->Paint();
}
void TInlineHighlighter::PaintPassages(TVector<TZonedString>& passages, const TPaintingOptions& options) const {
    TIPainterPtr p = GetPainter();
    p->Options = options;
    for (TZonedString& passage : passages) {
        p->AddJob(passage);
    }
    p->Paint();
}
