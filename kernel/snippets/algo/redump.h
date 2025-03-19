#pragma once

#include <kernel/snippets/config/enums.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/strbuf.h>

class TFactorStorage;

namespace NSnippets
{
    class TConfig;
    class TQueryy;
    class TSnip;
    class TSnipTitle;
    class TSentsMatchInfo;
    class TRetainedSentsMatchInfo;
    class TFactorsCalcer;
    struct ISnippetCandidateDebugHandler;
    class TWordSpanLen;
    class TCustomSnippetsStorage;

    struct TArcFragment {
        bool LinkArc = false;
        int SentBeg = 0;
        int SentEnd = 0;
        int OffsetBeg = 0;
        int OffsetEnd = 0;
        int SubOffsetBeg = 0;
        int SubOffsetEnd = 0;
        ESentsSourceType SourceType = SST_TEXT;

        TArcFragment()
        {
        }

        TArcFragment(bool linkArc, int sentBeg, int sentEnd,
                     int offsetBeg, int offsetEnd,
                     int subOffsetBeg, int subOffsetEnd,
                     ESentsSourceType st = SST_TEXT)
          : LinkArc(linkArc)
          , SentBeg(sentBeg)
          , SentEnd(sentEnd)
          , OffsetBeg(offsetBeg)
          , OffsetEnd(offsetEnd)
          , SubOffsetBeg(subOffsetBeg)
          , SubOffsetEnd(subOffsetEnd)
          , SourceType(st)
        {
        }
    };

    struct TArcFragments {
        TVector<TArcFragment> Fragments;

        TString Save() const;
        bool Load(TStringBuf s);

        bool AllLink() const {
            for (size_t i = 0; i < Fragments.size(); ++i) {
                if (Fragments[i].SourceType == SST_TEXT && !Fragments[i].LinkArc) {
                    return false;
                }
            }
            return true;
        }
        bool AllText() const {
            for (size_t i = 0; i < Fragments.size(); ++i) {
                if (Fragments[i].SourceType == SST_TEXT && Fragments[i].LinkArc) {
                    return false;
                }
            }
            return true;
        }
    };

    namespace NSnipRedump
    {
        TSnip SnipFromArc(const TSentsMatchInfo* info, const TArcFragments& snip);
        TSnip SnipFromText(const TConfig& cfg, const TSentsMatchInfo* info, const TUtf16String& text);
        TArcFragments SnipToArcFragments(const TSnip& snip, bool allowMetaDescr, bool allowCustomSource = false);
        TString SerializeCustomSource(const TSnip& snip);
        bool DeserializeCustomSource(TStringBuf serialized, TRetainedSentsMatchInfo& sents, TSnip& snip, const TConfig& cfg, const TQueryy& query);
        void DoCalc(TFactorStorage& factors, TFactorsCalcer& fcalc, const TSnip& snip);

        void GetSnippet(const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSentsMatchInfo* textInfo, const TSentsMatchInfo* linkInfo, TStringBuf coordsDump, ISnippetCandidateDebugHandler* cb, const char* aname, const TSnipTitle* title, float maxLenForLPFactors, const TString& url, TCustomSnippetsStorage& customSnippetsStorage);
        void FindSnippet(const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSentsMatchInfo* textInfo, const TUtf16String& snipText, ISnippetCandidateDebugHandler* cb, const char* aname, const TSnipTitle* title, float maxLenForLPFactors);
        void GetRetexts(const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSentsMatchInfo& textInfo, const TSnipTitle& defaultTitle, float maxLenForLPFactors, ISnippetCandidateDebugHandler* callback);
    }
}
