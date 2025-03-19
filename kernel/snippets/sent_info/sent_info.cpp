#include "sent_info.h"
#include "sentword.h"
#include "beautify.h"

#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/iface/archive/sent.h>
#include <kernel/snippets/smartcut/char_class.h>
#include <kernel/snippets/smartcut/consts.h>

#include <kernel/tarc/iface/tarcface.h>

#include <library/cpp/token/token_structure.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/singleton.h>
#include <util/string/util.h>
#include <util/charset/wide.h>

namespace NSnippets {
    namespace {
        void FixupParaInTables(TSentsInfo& info) {
            if (info.SentVal.empty()) {
                return;
            }
            const TArchiveMarkupZones& markup = info.GetTextArcMarkup();
            const TVector<TArchiveZoneSpan>& rows = markup.GetZone(AZ_TABLE_ROW).Spans;
            const TVector<TArchiveZoneSpan>& tbls = markup.GetZone(AZ_TABLE).Spans;
            size_t j = 0;
            size_t k = 0;
            for (size_t i = 0; i < info.SentVal.size();) {
                if (info.SentVal[i].ArchiveSent->SourceArc != ARC_TEXT) {
                    ++i;
                    continue;
                }
                int id = info.SentVal[i].ArchiveSent->SentId;
                while (j < rows.size() && rows[j].SentEnd < id) {
                    ++j;
                }
                while (k < tbls.size() && tbls[k].SentEnd < id) {
                    ++k;
                }
                if (j < rows.size() && rows[j].SentBeg <= id && k < tbls.size() && tbls[k].SentBeg <= id) {
                    bool singleRow = true;
                    if (j > 0 && rows[j - 1].SentBeg >= tbls[k].SentBeg || j + 1 < rows.size() && rows[j + 1].SentEnd <= tbls[k].SentEnd) {
                        singleRow = false;
                    }
                    if (singleRow) {
                        ++i;
                        continue;
                    }
                    info.SentVal[i].OffsetInPara = 0;
                    ++i;
                    while (i < info.SentVal.size() && info.SentVal[i].ArchiveSent->SourceArc == ARC_TEXT && info.SentVal[i].ArchiveSent->SentId <= rows[j].SentEnd) {
                        info.SentVal[i].OffsetInPara = -1;
                        ++i;
                    }
                    if (i < info.SentVal.size()) {
                        info.SentVal[i].OffsetInPara = 0;
                    }
                } else {
                    ++i;
                    continue;
                }
            }
            info.SentVal[0].OffsetInPara = 0;
            for (size_t i = 0; i < info.SentVal.size(); ++i) {
                if (info.SentVal[i].OffsetInPara) {
                    info.SentVal[i].OffsetInPara = info.SentVal[i - 1].OffsetInPara + 1;
                }
            }
        }

        void FillParaLen(TSentsInfo& info, bool paraTables) {
            if (!info.SentVal.size())
                return;
            if (paraTables) {
                FixupParaInTables(info);
            }

            int len = info.SentVal.back().OffsetInPara + 1;
            for (int i = int(info.SentVal.size()) - 1; i >= 0; --i) {
                info.SentVal[i].ParaLenInSents = len;
                if (info.SentVal[i].OffsetInPara == 0 && i > 0)
                    len = info.SentVal[i - 1].OffsetInPara + 1;
            }
            for (int i = 0; i < (int)info.SentVal.size(); ++i) {
                if (info.SentVal[i].OffsetInPara == 0) {
                    int sum = 0;
                    for (int j = 0; j < info.SentVal[i].ParaLenInSents; ++j)
                        sum += info.SentVal[i + j].LengthInWords;
                    for (int j = 0; j < info.SentVal[i].ParaLenInSents; ++j)
                        info.SentVal[i + j].ParaLenInWords = sum;
                }
            }
        }

        class TCtxControl {
        private:
            struct TPendingWord {
                unsigned int OffsetInSent = 0;
                unsigned int Length = 0;
                bool IsSuffix = false;
                TWtringBuf WordOrigin;
                NLP_TYPE Type;
            };
            struct TPendingSent {
                TSentsInfo& Ctx;
                TVector<wchar16> Text;
                TVector<TPendingWord> Words;
                TWtringBuf Origin;
                TWtringBuf SentOrigin;
                bool IsParaBeg = false;
                ESentsSourceType SourceType = SST_TEXT;
                TPendingSent(TSentsInfo& ctx)
                    : Ctx(ctx)
                {
                }
                void DropData() {
                    Text.clear();
                    Words.clear();
                    Origin = TWtringBuf();
                    SentOrigin = TWtringBuf();
                }
                void JumpOver(const TWtringBuf& tok) {
                    if (!Origin || !tok) {
                        return;
                    }
                    Y_ASSERT(tok.data() >= Origin.data() && tok.data() + tok.size() <= Origin.data() + Origin.size());
                    SentOrigin = TWtringBuf(Origin.data(), tok.data() + tok.size());
                }
                void BestHash(TSentsInfo::TWordPodStroka& s) {
                    auto nit = Ctx.W2H.find(s.Hash);
                    if (nit == Ctx.W2H.end()) {
                        s.N = Ctx.MaxN++;
                        Ctx.W2H[s.Hash] = s.N;
                    } else {
                        s.N = nit->second;
                    }
                }
                void Accept() {
                    Y_ASSERT(!Words.empty());
                    const int sentId = Ctx.SentVal.ysize();
                    const size_t sentOffset = Ctx.Text.size();
                    Ctx.SentVal.emplace_back();
                    Ctx.SentVal.back().StartInWords = Ctx.WordVal.size();
                    Ctx.SentVal.back().LengthInWords = Words.size();
                    Ctx.SentVal.back().SourceType = SourceType;
                    Ctx.SentVal.back().Sent.Ofs = sentOffset;
                    Ctx.SentVal.back().Sent.Len = Text.size();
                    Ctx.Text.append(Text.begin(), Text.end());
                    for (const TPendingWord& word : Words) {
                        const unsigned int wordOffset = sentOffset + word.OffsetInSent;
                        const unsigned int wordLength = word.Length;
                        const wchar16* wordPtr = Ctx.Text.data() + wordOffset;
                        Ctx.WordVal.emplace_back();
                        Ctx.WordVal.back().SentId = sentId;
                        Ctx.WordVal.back().IsSuffix = word.IsSuffix;
                        Ctx.WordVal.back().Origin = word.WordOrigin;
                        Ctx.WordVal.back().Word.Ofs = wordOffset;
                        Ctx.WordVal.back().Word.Len = wordLength;
                        Ctx.WordVal.back().Word.Hash = ComputeHash(TWtringBuf{wordPtr, wordLength});
                        Ctx.WordVal.back().Type = word.Type;
                        BestHash(Ctx.WordVal.back().Word);
                    }
                    Ctx.SentVal.back().ArchiveOrigin = SentOrigin;

                    TWtringBuf newOrigin = !Origin ? TWtringBuf() : !SentOrigin ? Origin : TWtringBuf(SentOrigin.data() + SentOrigin.size(), Origin.data() + Origin.size());
                    DropData();
                    Origin = newOrigin;
                    if (IsParaBeg || Ctx.SentVal.size() == 1) {
                        Ctx.SentVal.back().OffsetInPara = 0;
                        IsParaBeg = false;
                    } else {
                        Ctx.SentVal.back().OffsetInPara = Ctx.SentVal[Ctx.SentVal.size() - 2].OffsetInPara + 1;
                    }
                }
                void MaybeAccept() {
                    if (!Words.empty())
                        Accept();
                }
            };

        private:
            TPendingSent Sent;
            bool RespectSentEnd;

        public:
            TCtxControl(TSentsInfo& ctx, bool respectSentEnd)
                : Sent(ctx)
                , RespectSentEnd(respectSentEnd)
            {
            }
            void SetOrigin(const TWtringBuf& origin) {
                Y_ASSERT(Sent.Text.empty());
                Sent.Origin = origin;
                Sent.SentOrigin = TWtringBuf();
            }
            void JumpOver(const TWtringBuf& tok) {
                Sent.JumpOver(tok);
            }
            void NewPara() {
                Sent.IsParaBeg = true;
            }
            void SetSentSourceType(ESentsSourceType type) {
                Sent.SourceType = type;
            }
            void AddText(const wchar16* text, int len) {
                Sent.Text.insert(Sent.Text.end(), text, text + len);
            }
            void OnWord(const wchar16* ptr, int len, bool isSuffix, const TWtringBuf& origin, NLP_TYPE type) {
                size_t offsetInSent = Sent.Text.size();
                AddText(ptr, len);
                Sent.Words.emplace_back();
                Sent.Words.back().OffsetInSent = offsetInSent;
                Sent.Words.back().Length = len;
                Sent.Words.back().IsSuffix = isSuffix;
                Sent.Words.back().WordOrigin = origin;
                Sent.Words.back().Type = type;
            }
            void OnSentBreak(const wchar16* brea, int len, bool force = false) {
                AddText(brea, len);
                if (RespectSentEnd || force)
                    Sent.MaybeAccept();
            }
            void CleanupTail() {
                Sent.DropData();
            }
        };

        class TokenHandler: private TNonCopyable {
        public:
            TCtxControl& CtxControl;

        public:
            TokenHandler(TCtxControl& ctxControl)
                : CtxControl(ctxControl)
            {
            }

            void SetOrigin(const TWtringBuf& origin) {
                CtxControl.SetOrigin(origin);
            }

            void OnToken(const TWideToken& multiToken, const TWtringBuf& origin, NLP_TYPE type) {
                CtxControl.JumpOver(origin);
                const int leng = (int)multiToken.Leng;
                const wchar16* const token = multiToken.Token;
                if (type == NLP_SENTBREAK || type == NLP_PARABREAK || type == NLP_END) {
                    CtxControl.OnSentBreak(token, leng);
                } else if (!leng) {
                    return;
                } else if (type == NLP_WORD || type == NLP_MARK || type == NLP_INTEGER || type == NLP_FLOAT) {
                    const TTokenStructure& subTokens = multiToken.SubTokens;
                    if (subTokens.size() == 0) {
                        CtxControl.OnWord(token, leng, false, origin, type);
                    } else if (subTokens.size() == 1) {
                        const TCharSpan& first = subTokens[0];
                        if (first.Pos > 0) {
                            CtxControl.AddText(token, first.Pos); //TODO: maybe remember that this text chunk is a prefix
                        }
                        CtxControl.OnWord(token + first.Pos, leng - first.Pos, false, origin, type);
                    } else {
                        const TCharSpan& first = subTokens[0];
                        if (first.Pos > 0) {
                            CtxControl.AddText(token, first.Pos); //TODO: maybe remember that this text chunk is a prefix
                        }
                        CtxControl.OnWord(token + first.Pos, first.Len, false, origin, type);
                        for (int i = 1; i < (int)subTokens.size(); ++i) {
                            const TCharSpan& prev = subTokens[i - 1];
                            const TCharSpan& cur = subTokens[i];
                            if (prev.Pos + prev.Len < cur.Pos) {
                                CtxControl.AddText(token + prev.Pos + prev.Len, cur.Pos - prev.Pos - prev.Len);
                            }
                            const int len = (i + 1 == (int)subTokens.size()) ? leng - cur.Pos : cur.Len;
                            CtxControl.OnWord(token + cur.Pos, len, true, origin, type);
                        }
                    }
                } else {
                    CtxControl.AddText(token, leng);
                }
            }
        };

        struct TSentEndFSM {
            bool NeedSentEnd = false;
            void OnToken(const TWideToken& multiToken, NLP_TYPE type) {
                if (type == NLP_SENTBREAK || type == NLP_PARABREAK || type == NLP_END) {
                    NeedSentEnd = false;
                } else if (type == NLP_WORD || type == NLP_INTEGER || type == NLP_FLOAT || type == NLP_MARK) {
                    NeedSentEnd = true;
                } else if (type == NLP_MISCTEXT) {
                    TWtringBuf token = multiToken.Text();
                    for (wchar16 c : token) {
                        if (IsTerminal(c)) {
                            NeedSentEnd = false;
                            break;
                        }
                    }
                } else {
                    NeedSentEnd = false;
                }
            }
        };

        const wchar16 SentEnd[] = {'.', ' '};
        const wchar16 Space1[] = {' '};

        template <class T>
        class TProxyFixSentDelimetersTokenHandler: public ITokenHandler, private TNonCopyable {
        private:
            T& Handler;
            const bool PutDot;
            TSentEndFSM SentEndFSM;
            TUtf16String PendingSpace;
            TWideToken SpaceToken;
            TWtringBuf Origin;
            TWtringBuf TokOrigin;

        private:
            void FlushSpace() {
                if (!PendingSpace)
                    return;
                TWideToken& token = SpaceToken;
                token.Token = PendingSpace.data();
                token.Leng = PendingSpace.size();
                Handler.OnToken(token, TWtringBuf(), NLP_MISCTEXT);
                PendingSpace.clear();
            }
            bool CatchSpace(const TWideToken& multiToken, NLP_TYPE type) {
                if (type == NLP_MISCTEXT && IsSpace(multiToken.Token, multiToken.Leng)) {
                    PendingSpace.append(multiToken.Token, multiToken.Leng);
                    return true;
                }
                return false;
            }

        public:
            TProxyFixSentDelimetersTokenHandler(T& handler, bool putDot)
                : Handler(handler)
                , PutDot(putDot)
                , SpaceToken()
            {
            }

            void SetOrigin(const TWtringBuf& origin) {
                Handler.SetOrigin(origin);
                Origin = origin;
                TokOrigin = origin;
            }

            void OnToken(const TWideToken& multiToken, size_t origleng, NLP_TYPE type) override {
                if (!CatchSpace(multiToken, type)) {
                    FlushSpace();
                    Handler.OnToken(multiToken, TokOrigin.Head(origleng), type);
                    SentEndFSM.OnToken(multiToken, type);
                }
                TokOrigin.Skip(origleng);
            }

            void EnsureDelimited() {
                if (PutDot && SentEndFSM.NeedSentEnd) {
                    Handler.CtxControl.OnSentBreak(SentEnd, 2, true);
                    FlushSpace();
                } else {
                    FlushSpace();
                    Handler.CtxControl.OnSentBreak(Space1, 1, true);
                }
            }
        };
    }

    TSentsInfo::TSentsInfo(const TArchiveMarkup* markup,
                           const TArchiveView& vText,
                           const TArchiveView* metaDescrAdd,
                           bool putDot,
                           bool paraTables)
        : MetaDescrAdd(metaDescrAdd ? *metaDescrAdd : TArchiveView())
        , Segments(markup ? markup->GetSegments() : nullptr)
        , TextArcMarkupZones(markup ? markup->GetArcMarkup(ARC_TEXT) : Default<TArchiveMarkupZones>())
    {
        Init(vText, putDot, paraTables);
    }

    void TSentsInfo::Init(const TArchiveView& vText,
                          bool putDot,
                          bool paraTables) {
        TCtxControl ctxControl(*this, true);
        TokenHandler tokenHandler(ctxControl);
        typedef TProxyFixSentDelimetersTokenHandler<TokenHandler> TProxyHandler;

        TProxyHandler proxyDelimTokenHandler(tokenHandler, putDot);
        TNlpTokenizer tokenizer(proxyDelimTokenHandler, false);

        if (MetaDescrAdd.Size()) {
            ctxControl.SetSentSourceType(SST_META_DESCR);
            ctxControl.NewPara();

            for (size_t i = 0; i < MetaDescrAdd.Size(); ++i) {
                size_t j = SentVal.size();

                TWtringBuf origin = MetaDescrAdd.Get(i)->Sent;
                proxyDelimTokenHandler.SetOrigin(origin);
                tokenizer.Tokenize(origin.data(), origin.size());
                proxyDelimTokenHandler.EnsureDelimited();
                ctxControl.CleanupTail();

                for (; j < SentVal.size(); ++j) {
                    SentVal[j].ArchiveSent = MetaDescrAdd.Get(i);
                }
            }
        }

        ctxControl.SetSentSourceType(SST_TEXT);

        for (int ni = 0; ni != (int)vText.Size(); ++ni) {
            size_t j = SentVal.size();

            if (vText.Get(ni)->IsParaStart) {
                ctxControl.NewPara();
            }

            TWtringBuf origin = vText.Get(ni)->Sent;
            if (vText.Get(ni)->SourceArc == ARC_LINK) {
                origin = CutFirstUrls(origin);
            }

            if (origin.size()) {
                proxyDelimTokenHandler.SetOrigin(origin);
                tokenizer.Tokenize(origin.data(), origin.size());
            }

            proxyDelimTokenHandler.EnsureDelimited();
            ctxControl.CleanupTail();

            for (; j < SentVal.size(); ++j) {
                SentVal[j].ArchiveSent = vText.Get(ni);
            }
        }

        FillParaLen(*this, paraTables);
        for (int i = 0; i < SentencesCount(); ++i) {
            if (SentVal[i].ArchiveSent->SourceArc != ARC_TEXT) {
                continue;
            }
            const int arch = SentVal[i].ArchiveSent->SentId;
            if (TextArchIdToSents.find(arch) == TextArchIdToSents.end()) {
                TextArchIdToSents[arch] = std::pair<int, int>(i, i);
            } else {
                TextArchIdToSents[arch].second = i;
            }
        }

        for (int i = 0; i < WordCount(); ++i) {
            const int sentId = WordId2SentId(i);
            const bool beginSent = FirstWordIdInSent(sentId) == i;
            unsigned int beg = beginSent ? SentVal[sentId].Sent.Ofs : WordVal[i].Word.Ofs;
            if (!beginSent && beg > 0 && IsLeftQuoteOrBracket(Text[beg - 1])) {
                --beg;
            }
            WordVal[i].TextBufBegin = beg;

            const bool endSent = LastWordIdInSent(sentId) == i;
            if (endSent) {
                unsigned int end = SentVal[sentId].Sent.EndOfs();
                if (end > 0 && IsSpace(Text[end - 1])) {
                    --end;
                }
                WordVal[i].TextBufEnd = end;
            } else {
                unsigned int end = WordVal[i].Word.EndOfs();
                if (end + 1 < Text.size() && IsRightQuoteOrBracket(Text[end])) {
                    ++end;
                }
                WordVal[i].TextBufEnd = end;
            }
        }
    }

    template <>
    TSentWord TSentsInfo::Begin() const {
        return TSentWord(this, 0, 0);
    }

    template <>
    TSentMultiword TSentsInfo::Begin() const {
        return TSentMultiword(Begin<TSentWord>());
    }

    template <>
    TSentWord TSentsInfo::End() const {
        return TSentWord(this, SentencesCount(), 0);
    }

    template <>
    TSentMultiword TSentsInfo::End() const {
        return TSentMultiword(End<TSentWord>());
    }

    int TSentsInfo::WordCount() const {
        return WordVal.ysize();
    }

    int TSentsInfo::SentencesCount() const {
        return SentVal.ysize();
    }

    std::pair<int, int> TSentsInfo::TextArchIdToSentIds(int archId) const {
        if (TextArchIdToSents.find(archId) == TextArchIdToSents.end()) {
            return std::pair<int, int>(-1, -1);
        }
        return TextArchIdToSents.find(archId)->second;
    }

    TSentWord TSentsInfo::WordId2SentWord(int wordId) const {
        const int sentId = WordId2SentId(wordId);
        const int ofs = wordId - FirstWordIdInSent(sentId);
        return TSentWord(this, sentId, ofs);
    }

    int TSentsInfo::GetOrigSentId(int i) const {
        switch (SentVal[i].SourceType) {
            case SST_META_DESCR:
                return META_ORIGIN_SENT_ID;
            default:
                Y_ASSERT(SentVal[i].SourceType == SST_TEXT);
                return SentVal[i].ArchiveSent->SentId;
        }
    }

    int TSentsInfo::WordId2SentId(int wordId) const {
        return WordVal[wordId].SentId;
    }

    int TSentsInfo::FirstWordIdInSent(int sentId) const {
        return SentVal[sentId].StartInWords;
    }

    int TSentsInfo::LastWordIdInSent(int sentId) const {
        return SentVal[sentId].StartInWords + SentVal[sentId].LengthInWords - 1;
    }

    int TSentsInfo::GetSentLengthInWords(int sentId) const {
        return SentVal[sentId].LengthInWords;
    }

    const TArchiveSent& TSentsInfo::GetArchiveSent(int sentId) const {
        return *SentVal[sentId].ArchiveSent;
    }

    bool TSentsInfo::IsSentIdFirstInArchiveSent(int sentId) const {
        return sentId == 0 || SentVal[sentId].ArchiveSent != SentVal[sentId - 1].ArchiveSent;
    }

    bool TSentsInfo::IsSentIdFirstInPara(int sentId) const {
        return SentVal[sentId].OffsetInPara == 0;
    }

    bool TSentsInfo::IsSentIdLastInPara(int sentId) const {
        return SentVal[sentId].OffsetInPara == SentVal[sentId].ParaLenInSents - 1;
    }

    bool TSentsInfo::IsWordIdFirstInSent(int wordId) const {
        return wordId == FirstWordIdInSent(WordId2SentId(wordId));
    }

    bool TSentsInfo::IsWordIdLastInSent(int wordId) const {
        return wordId == LastWordIdInSent(WordId2SentId(wordId));
    }

    bool TSentsInfo::IsCharIdFirstInWord(size_t charId, int wordId) const {
        return charId == WordVal[wordId].Word.Ofs;
    }

    void TSentsInfo::OutWords(int i, int j) const {
        for (int a = i; a <= j; a++) {
            Cerr << GetWordBuf(a) << " ";
        }
        Cerr << Endl;
    }

    const NSegments::TSegmentsInfo* TSentsInfo::GetSegments() const {
        return Segments;
    }

    const TArchiveMarkupZones& TSentsInfo::GetTextArcMarkup() const {
        return TextArcMarkupZones;
    }

    TWtringBuf TSentsInfo::GetTextBuf(int i, int j) const {
        return TWtringBuf(Text.data() + WordVal[i].TextBufBegin, Text.data() + WordVal[j].TextBufEnd);
    }

    TUtf16String TSentsInfo::GetTextWithEllipsis(int i, int j) const {
        TWtringBuf prefix, text, suffix;
        GetTextWithEllipsis(i, j, prefix, text, suffix);
        return TUtf16String::Join(prefix, text, suffix);
    }

    void TSentsInfo::GetTextWithEllipsis(int i, int j, TWtringBuf& prefix, TWtringBuf& text, TWtringBuf& suffix) const {
        if (!IsWordIdFirstInSent(i)) {
            prefix = BOUNDARY_ELLIPSIS;
        } else {
            prefix.Clear();
        }
        text = GetTextBuf(i, j);
        if (!IsWordIdLastInSent(j)) {
            suffix = BOUNDARY_ELLIPSIS;
        } else {
            suffix.Clear();
        }
    }

    NLP_TYPE TSentsInfo::GetWordType(int wordId) const {
        return WordVal[wordId].Type;
    }

    TWtringBuf TSentsInfo::GetWordBuf(int wordId) const {
        return GetWordSpanBuf(wordId, wordId);
    }

    TWtringBuf TSentsInfo::GetBlanksBefore(int wordId) const {
        size_t beginOfs = wordId > 0 ? WordVal[wordId - 1].Word.EndOfs() : 0;
        size_t endOfs = WordVal[wordId].Word.Ofs;
        return TWtringBuf(Text.data() + beginOfs, Text.data() + endOfs);
    }

    TWtringBuf TSentsInfo::GetBlanksAfter(int wordId) const {
        size_t beginOfs = WordVal[wordId].Word.EndOfs();
        size_t endOfs = wordId + 1 < WordVal.ysize() ? WordVal[wordId + 1].Word.Ofs : Text.size();
        return TWtringBuf(Text.data() + beginOfs, Text.data() + endOfs);
    }

    TWtringBuf TSentsInfo::GetSentBeginBlanks(int sentId) const {
        size_t sentBeg = SentVal[sentId].Sent.Ofs;
        size_t firstWordBeg = WordVal[FirstWordIdInSent(sentId)].Word.Ofs;
        return TWtringBuf(Text.data() + sentBeg, Text.data() + firstWordBeg);
    }

    TWtringBuf TSentsInfo::GetSentEndBlanks(int sentId) const {
        size_t lastWordEnd = WordVal[LastWordIdInSent(sentId)].Word.EndOfs();
        size_t sentEnd = SentVal[sentId].Sent.EndOfs();
        return TWtringBuf(Text.data() + lastWordEnd, Text.data() + sentEnd);
    }

    TWtringBuf TSentsInfo::GetWordSpanBuf(int w0, int w1) const {
        size_t beginOfs = WordVal[w0].Word.Ofs;
        size_t endOfs = WordVal[w1].Word.EndOfs();
        return TWtringBuf(Text.data() + beginOfs, Text.data() + endOfs);
    }

    TWtringBuf TSentsInfo::GetWordSentSpanBuf(int w0, int w1) const {
        const int s0 = WordId2SentId(w0);
        const int s1 = WordId2SentId(w1);
        size_t beginOfs = w0 == FirstWordIdInSent(s0) ? SentVal[s0].Sent.Ofs : WordVal[w0].Word.Ofs;
        size_t endOfs = w1 == LastWordIdInSent(s1) ? SentVal[s1].Sent.EndOfs() : WordVal[w1].Word.EndOfs();
        return TWtringBuf(Text.data() + beginOfs, Text.data() + endOfs);
    }

    TWtringBuf TSentsInfo::GetSentBuf(int sentId) const {
        return GetSentSpanBuf(sentId, sentId);
    }

    TWtringBuf TSentsInfo::GetSentSpanBuf(int s0, int s1) const {
        size_t beginOfs = SentVal[s0].Sent.Ofs;
        size_t endOfs = SentVal[s1].Sent.EndOfs();
        return TWtringBuf(Text.data() + beginOfs, Text.data() + endOfs);
    }
}
