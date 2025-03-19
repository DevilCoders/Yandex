#include "redump.h"

#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/factors/factor_storage.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/titles/make_title/make_title.h>
#include <kernel/snippets/titles/make_title/util_title.h>
#include <kernel/snippets/weight/weighter.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/string/split.h>
#include <util/string/cast.h>

namespace NSnippets
{
    static TString SaveArcFragment(const TArcFragment& a) {
        TString res;
        const TStringBuf arc = a.SourceType == SST_META_DESCR ? "Meta" : (a.LinkArc ? "Link" : "Text");
        res += arc;
        res += "_";
        res += ToString(a.SentBeg) + ":" + ToString(a.OffsetBeg);
        res += "~";
        res += ToString(a.SentEnd) + ":" + ToString(a.OffsetEnd);
        return res;
    }

    TString TArcFragments::Save() const {
        TString res;
        for (size_t i = 0; i < Fragments.size(); ++i) {
            if (res)
                res += ' ';
            res += SaveArcFragment(Fragments[i]);
        }
        return res;
     }

     static bool LoadColonPair(int& a, int& b, TStringBuf s) {
         size_t d = s.find(':');
         if (d == TString::npos) {
             return false;
         }
         return TryFromString(TStringBuf(s.data(), s.data() + d), a) && TryFromString(TStringBuf(s.data() + d + 1, s.data() + s.size()), b);
     }
     static bool LoadArcFragment(TArcFragment& res, TStringBuf s) {
         size_t d = s.find('_');
         if (d == TString::npos) {
             return false;
         }
         TStringBuf arc(s.data(), s.data() + d);
         if (arc == "Link") {
             res.LinkArc = true;
         } else if (arc == "Text") {
             res.LinkArc = false;
         } else if (arc == "Meta") {
             res.SourceType = SST_META_DESCR;
         } else {
             return false;
         }
         s = TStringBuf(s.data() + d + 1, s.data() + s.size());
         d = s.find('~');
         if (d == TString::npos) {
             return false;
         }
         if (!LoadColonPair(res.SentBeg, res.OffsetBeg, TStringBuf(s.data(), s.data() + d))) {
             return false;
         }
         if (!LoadColonPair(res.SentEnd, res.OffsetEnd, TStringBuf(s.data() + d + 1, s.data() + s.size()))) {
             return false;
         }
         return true;
     }

    bool TArcFragments::Load(TStringBuf s) {
         Fragments.clear();
         while (s.size()) {
             size_t d = s.find(' ');
             if (d == TString::npos) {
                 d = s.size();
             }
             TArcFragment x;
             if (!LoadArcFragment(x, TStringBuf(s.data(), s.data() + d))) {
                 return false;
             }
             Fragments.push_back(x);
             s = TStringBuf(s.data() + d + (d < s.size()), s.data() + s.size());
         }
         return true;
    }

    namespace NSnipRedump
    {

        static THolder<TSnipTitle> DeserializeTitle(TStringBuf serialized, const TConfig& cfg, const TQueryy& query) {
            TRetainedSentsMatchInfo matchInfo;
            TSnip snip;
            if (!DeserializeCustomSource(serialized, matchInfo, snip, cfg, query)) {
                return {};
            }
            return MakeHolder<TSnipTitle>(matchInfo.GetSentsMatchInfo() ? matchInfo.GetSentsMatchInfo()->SentsInfo.Text : TUtf16String(), matchInfo, snip, TTitleFactors(), TDefinition());
        }

        void GetSnippet(const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSentsMatchInfo* textInfo, const TSentsMatchInfo* linkInfo, TStringBuf coordsDump, ISnippetCandidateDebugHandler* cb, const char* aname, const TSnipTitle* title, float maxSnipLenForLPFactors, const TString& url, TCustomSnippetsStorage& customSnippetsStorage) {
            if (!cb || !coordsDump.size()) {
                return;
            }
            IAlgoTop* cbtext = cb->AddTop(aname, CS_TEXT_ARC);
            IAlgoTop* cblink = cb->AddTop(aname, CS_LINK_ARC);
            IAlgoTop* cbmeta = cb->AddTop(aname, CS_METADATA);
            if (!cbtext && !cblink && !cbmeta) {
                return;
            }
            TFactorStorage fstore(&cfg.GetTotalFormula().GetFactorDomain());
            THolder<TFactorsCalcer> fcalcText;
            THolder<TFactorsCalcer> fcalcLink;
            TVector<TStringBuf> snips;
            {
                TCharDelimiter<const char> c('\n');
                TContainerConsumer< TVector<TStringBuf> > v(&snips);
                SplitString(coordsDump.data(), coordsDump.data() + coordsDump.size(), c, v);
            }

            TMaybe<TString> currentTitleString;
            THolder<TSnipTitle> currentTitle;
            for (size_t i = 0; i < snips.size(); ++i) {
                TStringBuf serialized = snips[i];
                TStringBuf s = serialized.NextTok('|');
                TMaybe<TString> requestedTitleString;
                if (serialized.IsInited())
                    requestedTitleString = ToString(serialized);
                if (requestedTitleString != currentTitleString) {
                    currentTitleString = requestedTitleString;
                    currentTitle.Destroy();
                    if (currentTitleString)
                        currentTitle = DeserializeTitle(currentTitleString.GetRef(), cfg, textInfo ? textInfo->Query : linkInfo->Query);
                    fcalcText.Destroy();
                    fcalcLink.Destroy();
                }

                if (s.StartsWith("String_")) {
                    if (!cbmeta)
                        continue;
                    TRetainedSentsMatchInfo& matchInfo = customSnippetsStorage.CreateRetainedInfo();
                    TSnip snip;
                    if (!DeserializeCustomSource(s, matchInfo, snip, cfg, textInfo ? textInfo->Query : linkInfo->Query)) {
                        continue;
                    }
                    if (!snip.Snips.empty()) {
                        TFactorsCalcer calcer(*matchInfo.GetSentsMatchInfo(), cfg, url, wordSpanLen, currentTitle ? currentTitle.Get() : title, maxSnipLenForLPFactors);
                        fstore.Clear();
                        DoCalc(fstore, calcer, snip);
                        snip = TSnip(snip.Snips, INVALID_SNIP_WEIGHT, fstore);
                        cbmeta->Push(snip);
                    }
                    continue;
                }

                TArcFragments f;
                if (f.Load(s)) {
                    if (f.AllLink() && cblink && linkInfo) {
                        TSnip snip = SnipFromArc(linkInfo, f);
                        if (!fcalcLink.Get()) {
                            fcalcLink.Reset(new TFactorsCalcer(*linkInfo, cfg, url, wordSpanLen, currentTitle ? currentTitle.Get() : title, maxSnipLenForLPFactors));
                        }
                        if (!snip.Snips.empty()) {
                            fstore.Clear();
                            DoCalc(fstore, *fcalcLink.Get(), snip);
                            snip = TSnip(snip.Snips, INVALID_SNIP_WEIGHT, fstore);
                            cblink->Push(snip);
                        }
                    } else if (f.AllText() && cbtext && textInfo) {
                        TSnip snip = SnipFromArc(textInfo, f);
                        if (!fcalcText.Get()) {
                            fcalcText.Reset(new TFactorsCalcer(*textInfo, cfg, url, wordSpanLen, currentTitle ? currentTitle.Get() : title, maxSnipLenForLPFactors));
                        }
                        if (!snip.Snips.empty()) {
                            fstore.Clear();
                            DoCalc(fstore, *fcalcText.Get(), snip);
                            snip = TSnip(snip.Snips, INVALID_SNIP_WEIGHT, fstore);
                            cbtext->Push(snip);
                        }
                    }
                }
            }
        }

        void FindSnippet(const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSentsMatchInfo* textInfo, const TUtf16String& snipText, ISnippetCandidateDebugHandler* cb, const char* aname, const TSnipTitle* title, float maxLenForLPFactors) {
            if (!cb || !snipText.size() || !textInfo) {
                return;
            }
            IAlgoTop* top = cb->AddTop(aname, CS_TEXT_ARC);
            if (!top) {
                return;
            }
            TSnip snip = SnipFromText(cfg, textInfo, snipText);
            if (!snip.Snips.empty()) {
                TFactorStorage fstore(&cfg.GetTotalFormula().GetFactorDomain());
                TFactorsCalcer fcalc(*textInfo, cfg, TString(), wordSpanLen, title, maxLenForLPFactors);
                DoCalc(fstore, fcalc, snip);
                snip = TSnip(snip.Snips, INVALID_SNIP_WEIGHT, std::move(fstore));
                top->Push(snip);
            }
        }

        void DoCalc(TFactorStorage& factors, TFactorsCalcer& fcalc, const TSnip& snip) {
            TSpans ss, ws;
            for (TSnip::TSnips::const_iterator it = snip.Snips.begin(); it != snip.Snips.end(); ++it) {
                ws.push_back(TSpan(it->GetFirstWord(), it->GetLastWord()));
                int si = it->GetSentsMatchInfo()->SentsInfo.WordId2SentId(it->GetFirstWord());
                int sj = it->GetSentsMatchInfo()->SentsInfo.WordId2SentId(it->GetLastWord());
                ss.push_back(TSpan(si, sj));
            }
            fcalc.CalcAll(factors, ws, ss);
        }

        static int SearchWordAtCoords(const TSentsInfo& s, const int sent, const int offset, const ESentsSourceType sst)
        {
            int l = 0, r = s.WordCount() - 1;

            if (sst != SST_TEXT) {
                bool foundSent = false;
                for (int i = 0; i < s.SentencesCount(); ++i) {
                    if (s.SentVal[i].SourceType == SST_TEXT) {
                        break;
                    } else if (s.SentVal[i].SourceType == sst) {
                        if (!foundSent) {
                            foundSent = true;
                            l = s.FirstWordIdInSent(i);
                        }
                        r = s.LastWordIdInSent(i);
                    }
                }
                if (!foundSent) {
                    return -1;
                }
            }

            while (l < r) {
                const int m = (l + r) / 2;
                int joinedSentId = s.WordId2SentId(m);
                const TArchiveSent& sa = s.GetArchiveSent(joinedSentId);
                if (s.SentVal[joinedSentId].SourceType != SST_TEXT && sst == SST_TEXT) {
                    l = m + 1;
                } else if (sa.SentId < sent) {
                    l = m + 1;
                } else if (sa.SentId > sent) {
                    r = m;
                } else {
                    const TWtringBuf w = s.WordVal[m].Origin;
                    Y_ASSERT(w.size());
                    if (w.data() + w.size() <= sa.Sent.data() + offset) {
                        l = m + 1;
                    } else {
                        r = m;
                    }
                }
            }

            // for group of words with the same Origin we are positioned on the first one
            int next = l + 1;
            int delta = s.GetArchiveSent(s.WordId2SentId(l)).Sent.data() + offset - s.WordVal[l].Origin.data();
            while (next < s.WordCount() && s.WordVal[next].Origin.data() == s.WordVal[l].Origin.data() && (int)(s.WordVal[next].Word.Ofs - s.WordVal[l].Word.Ofs) <= delta)
                ++next;

            return next - 1;
        }

        static bool CheckFoundValidWord(const int word, const TSentsInfo& s, const int sent, const int offset, ESentsSourceType sst) {
            int joinedSentId = s.WordId2SentId(word);
            const TArchiveSent& sa = s.GetArchiveSent(joinedSentId);
            const TWtringBuf w = s.WordVal[word].Origin;
            Y_ASSERT(w.size());
            if (s.SentVal[joinedSentId].SourceType != SST_TEXT && sst == SST_TEXT) {
                return false;
            }
            if (sa.SentId != sent) {
                return false;
            }
            if (w.data() + w.size() < sa.Sent.data() + offset) {
                return false;
            }
            if (w.data() > sa.Sent.data() + offset) {
                return false;
            }
            return true;
        }

        int SeekMultitokenAtOffset(int word, const TSentsInfo& s, const int subOffset) {
            if (subOffset) {
                while (s.WordVal[word].IsSuffix && word > 0) {
                    --word;
                }
                unsigned int multiTokStart = s.WordVal[word].Word.Ofs;
                while (s.WordVal[word].Word.Ofs < subOffset + multiTokStart && word < s.WordCount()) {
                    ++word;
                }
            }
            return word;
        }

        static int FindWordWithArcChar(const TSentsInfo& s, int sent, int offset, int subOffset, ESentsSourceType sst) {
            int word = SearchWordAtCoords(s, sent, offset, sst);
            if (word < 0) {
                return -1;
            }
            if (!CheckFoundValidWord(word, s, sent, offset, sst)) {
                return -1;
            }
            word = SeekMultitokenAtOffset(word, s, subOffset);
            return word;
        }

        TSnip SnipFromArc(const TSentsMatchInfo* info, const TArcFragments& snip) {
            TSnip s;
            if (!info || snip.Fragments.empty() || !info->SentsInfo.WordCount()) {
                return TSnip();
            }
            const TSentsInfo& sinfo = info->SentsInfo;
            for (size_t i = 0; i < snip.Fragments.size(); ++i) {
                const TArcFragment& f = snip.Fragments[i];
                int l = FindWordWithArcChar(sinfo, f.SentBeg, f.OffsetBeg, f.SubOffsetBeg, f.SourceType);
                int r = FindWordWithArcChar(sinfo, f.SentEnd, f.OffsetEnd, f.SubOffsetEnd, f.SourceType);
                if (l < 0 || r < 0) {
                    return TSnip();
                }
                s.Snips.push_back(TSingleSnip(l, r, *info));
            }
            return s;
        }

        TSnip SnipFromText(const TConfig& cfg, const TSentsMatchInfo* info, const TUtf16String& text) {
            TSnip res;
            if (!info) {
                return res;
            }
            const int m = info->SentsInfo.WordCount();
            if (!text.size() || !m) {
                return res;
            }
            TSkippedRestr skip(false, info->SentsInfo, cfg);
            TArchiveStorage tempStorage;
            TArchiveView view;
            view.PushBack(&*tempStorage.Add(ARC_MISC, 0, text, 0));
            TSentsInfo sinfo(nullptr, view, nullptr, false, false);

            const int n = sinfo.WordCount();
            if (!n) {
                return res;
            }
            //lame greedy exact word matching in res.size()*m*cmpwords time
            int shift = 0;
            TVector<int> pf;
            int i0 = 0;
            while (shift < n) {
                pf.clear();
                pf.push_back(0);
                int cur = 0;
                int best = 0;
                int ibest = 0;
                for (int i = i0; i < m; ++i) {
                    TWtringBuf t = info->SentsInfo.GetWordBuf(i);
                    while (cur && t != sinfo.GetWordBuf(shift + cur)) {
                        cur = pf[cur - 1];
                    }
                    if (t == sinfo.GetWordBuf(shift + cur)) {
                        ++cur;
                    }
                    if (cur > best) {
                        best = cur;
                        ibest = i - best + 1;
                    }
                    while ((size_t)cur >= pf.size() && shift + cur < n) {
                        pf.push_back(pf.back());
                        while (pf.back() && sinfo.GetWordBuf(shift + cur) != sinfo.GetWordBuf(shift + pf[cur])) {
                            pf.back() = pf[pf.back() - 1];
                        }
                        if (sinfo.GetWordBuf(shift + cur) == sinfo.GetWordBuf(shift + pf[cur])) {
                            ++pf.back();
                        }
                    }
                    if (shift + cur == n) {
                        cur = pf[cur - 1]; //or break - final match found already
                    }
                    if (info->SentsInfo.IsWordIdLastInSent(i)) {
                        if (skip(info->SentsInfo.WordId2SentId(i))) {
                            cur = 0;
                        }
                    }
                }
                if (!best) { //not found in i0..
                    res.Snips.clear();
                    return res;
                }
                res.Snips.push_back(TSingleSnip(ibest, ibest + best - 1, *info));
                i0 = ibest + best;
                shift += best;
            }
            return res;
        }


        static bool IsArcFragment(const TSentsInfo& info, int si, int sj) {
            for (; si < sj; ++si) {
                const TArchiveSent& ai = info.GetArchiveSent(si);
                const TArchiveSent& aii = info.GetArchiveSent(si + 1);
                if (ai.SourceArc != aii.SourceArc) {
                    return false;
                }
                if (ai.SentId != aii.SentId && ai.SentId + 1 != aii.SentId) {
                    return false;
                }
                if (ai.SentId == aii.SentId) {
                    if (info.SentVal[si + 1].ArchiveOrigin.data() != info.SentVal[si].ArchiveOrigin.data() + info.SentVal[si].ArchiveOrigin.size()) {
                        return false;
                    }
                }
            }
            return true;
        }

        unsigned int SubtokenOffset(const TSentsInfo& info, int wordId) {
            return info.WordVal[wordId].Word.Ofs - info.WordVal[info.WordId2SentWord(wordId).MultiwordFirst().ToWordId()].Word.Ofs;
        }

        TArcFragments SnipToArcFragments(const TSnip& snip, bool allowMetaDescr, bool allowCustomSource) {
            TArcFragments res;
            for (TSnip::TSnips::const_iterator it = snip.Snips.begin(); it != snip.Snips.end(); ++it) {
                const TSingleSnip& s = *it;
                const TSentsInfo& info = s.GetSentsMatchInfo()->SentsInfo;
                const int i = s.GetFirstWord();
                const int j = s.GetLastWord();
                if (!info.WordVal[i].Origin.size() || !info.WordVal[j].Origin.size()) {
                    res = TArcFragments();
                    return res;
                }
                const int si = info.WordId2SentId(i);
                const int sj = info.WordId2SentId(j);
                Y_ASSERT(IsArcFragment(info, si, sj));
                const TArchiveSent& ai = info.GetArchiveSent(si);
                const TArchiveSent& aj = info.GetArchiveSent(sj);
                bool isMetaDescr = false;
                if (ai.SourceArc != ARC_TEXT && ai.SourceArc != ARC_LINK && !allowCustomSource) {
                    if (allowMetaDescr &&
                        ai.SourceArc == ARC_MISC &&
                        info.SentVal[si].SourceType == SST_META_DESCR &&
                        info.SentVal[sj].SourceType == SST_META_DESCR)
                    {
                        isMetaDescr = true;
                    } else {
                        res = TArcFragments();
                        return res; // sentid is not a global identifier for random ARC_MISC, unless we're capturing the entire source in SerializeCustomSource() as well
                    }
                }

                const bool linkArc = ai.SourceArc == ARC_LINK;
                const int oi = info.WordVal[i].Origin.data() - ai.Sent.data();
                const int oj = info.WordVal[j].Origin.data() + info.WordVal[j].Origin.size() - 1 - aj.Sent.data();
                const int iSub = SubtokenOffset(info, i);
                const int jSub = SubtokenOffset(info, j);
                res.Fragments.push_back(TArcFragment(linkArc, ai.SentId, aj.SentId, oi, oj, iSub, jSub, isMetaDescr ? SST_META_DESCR : SST_TEXT));
            }
            return res;
        }

        void GetRetexts(const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSentsMatchInfo& textInfo, const TSnipTitle& defaultTitle, float maxLenForLPFactors, ISnippetCandidateDebugHandler* callback) {
            const auto& retexts = cfg.GetRetexts();
            for (const auto& retext : retexts) {
                TMakeTitleOptions options(cfg);
                options.TitleCutMethod = TCM_SYMBOL;
                options.MaxTitleLen = Max<int>();
                options.DefinitionMode = TDM_IGNORE;
                options.TitleGeneratingAlgo = TGA_SMART;
                TUtf16String titleText = retext.second.second;
                ClearChars(titleText, /* allowSlash */ false, cfg.AllowBreveInTitle());
                TSnipTitle title = MakeTitle(titleText, cfg, textInfo.Query, options); // succeeds even if title text is not within the archive

                const TSnipTitle* wantTitle = titleText.size() ? &title : &defaultTitle;
                FindSnippet(cfg, wordSpanLen, &textInfo, retext.second.first, callback, (retext.first).data(), !cfg.TitWeight() ? nullptr : wantTitle, maxLenForLPFactors);
            }
        }

        TString SerializeCustomSource(const TSnip& snip) {
            if (snip.Snips.empty())
                return {};
            const TSentsMatchInfo* info = snip.Snips.front().GetSentsMatchInfo();
            Y_ASSERT(info);
            for (const TSingleSnip& ss : snip.Snips) {
                Y_ASSERT(ss.GetSentsMatchInfo() == info);
                Y_ASSERT(!ss.GetAllowInnerDots() && !ss.GetForceDotsAtBegin() && !ss.GetForceDotsAtEnd());
                Y_ASSERT(ss.GetSnipType() == SPT_COMMON);
                Y_ASSERT(!ss.GetPassageAttrs());
            }
            TString arc;
            const TArchiveSent* prev = nullptr;
            for (int i = 0; i < info->SentsInfo.SentencesCount(); i++) {
                const TArchiveSent* cur = &info->SentsInfo.GetArchiveSent(i);
                if (cur == prev)
                    continue;
                prev = cur;
                if (arc)
                    arc += '~';
                arc += Base64Encode(WideToUTF8(cur->Sent));
            }
            bool oneFullFragment = (snip.Snips.size() == 1 && snip.Snips.front().GetFirstWord() == 0 && snip.Snips.front().GetLastWord() == info->WordsCount() - 1);
            // oneFullFragment should be always true for custom snippets and often true for titles
            if (!oneFullFragment) {
                TArcFragments fragments = SnipToArcFragments(snip, true, true);
                arc += ':';
                arc += fragments.Save();
            }
            return "String_" + arc;
        }

        bool DeserializeCustomSource(TStringBuf serialized, TRetainedSentsMatchInfo& matchInfo, TSnip& snip, const TConfig& cfg, const TQueryy& query) {
            if (!serialized)
                return true; // empty snippet
            if (!serialized.SkipPrefix("String_"))
                return false;
            TStringBuf sourceEncoded = serialized.NextTok(':');
            TStringBuf sent;
            TVector<TUtf16String> source;
            while (sourceEncoded.NextTok('~', sent))
                source.push_back(UTF8ToWide(Base64Decode(sent)));
            matchInfo.SetView(source, TRetainedSentsMatchInfo::TParams(cfg, query));
            if (serialized.IsInited()) {
                TArcFragments fragments;
                if (!fragments.Load(serialized))
                    return false;
                snip = SnipFromArc(matchInfo.GetSentsMatchInfo(), fragments);
                if (snip.Snips.empty())
                    return false;
            } else {
                if (matchInfo.GetSentsMatchInfo()->WordsCount() == 0)
                    return false;
                snip.Snips.push_back(TSingleSnip(0, matchInfo.GetSentsMatchInfo()->WordsCount() - 1, *matchInfo.GetSentsMatchInfo()));
            }
            return true;
        }
    }

}
