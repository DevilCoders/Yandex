#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <library/cpp/binsaver/bin_saver.h>
#include "regex_wrapper.h"
#include "matchable.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
TUtf16String MakeUniformStyle(const TUtf16String &phrase);
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EIntentProps
{
    IPROP_NONE  = 0,
    IPROP_LOCAL = 1,
    IPROP_NEED_LOCAL = 2,
    IPROP_NEED_COUNTRY = 4,
    IPROP_FRESH = 8,
    IPROP_HARD = 16,
    IPROP_HAS_GLOBAL_UID = 32,
    IPROP_LAST
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TIntent
{
    TVector<TUtf16String> Wordings;
    TVector<TUtf16String> WordingsAsInScript;
    TVector<TUtf16String> Buzzwords;
    TUtf16String UniqueName;
    int Props; // set of flags from EIntentProps
    TVector<TString> SitesDisallowed;
    TVector<TUtf16String> Satisfies;
    TVector<TUtf16String> QueriesDisallowed;
    TIntent()
        : Props(IPROP_NONE)
    {
    }
    int operator&(IBinSaver &f) { f.Add(1,&Wordings); f.Add(2,&WordingsAsInScript); f.Add(3, &Buzzwords); f.Add(4, &UniqueName); f.Add(5, &Props); f.Add(6, &SitesDisallowed); f.Add(7, &Satisfies); f.Add(8, &QueriesDisallowed); return 0; }
    bool HasSameMatches(const TIntent &rhs) const;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TPatternElem
{
    TUtf16String Word;
    bool ExactWords;
    TPatternElem() : ExactWords(false) {
    }
    int operator&(IBinSaver &f) { f.Add(2,&Word); f.Add(3,&ExactWords); return 0; }
    bool operator == (const TPatternElem &rhs) const;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class TPattern
{
    TMatchable Examples;
    TVector<TPatternElem> PatternsBegin;
    TVector<TPatternElem> PatternsEnd;

    void AddExampleImpl(const TUtf16String &what);
public:
    int operator&(IBinSaver &f);
    bool IsMatch(const TUtf16String &what, int* id = nullptr) const;
    void AddExample(const TUtf16String &templ);
    bool operator == (const TPattern &rhs) const;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TGeobaseMarker
{
    TVector<int> GeoTypes;
    TVector<int> ParentRegions;
    int operator&(IBinSaver &f);
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TIntentGroup
{
    THashMap<TUtf16String, TUtf16String> ExamplesWithSynonims;
    TMatchable Markers;
    THashSet<TUtf16String> AntiMarkers;
    TVector<TIntent> Intents;
    int NumMarkerTypes;
    TPattern AntiPattern;
    TVector<TPattern> PatternMarkers;
    bool IsUniversalIntent;
    TVector<TUtf16String> IsIn;
    TVector<TUtf16String> IsNotIn;
    TVector<TString> MarkerSites;
    TVector<TUtf16String> MarkerWikiCategs;
    TVector<TGeobaseMarker> GeobaseMarkers;
    TVector<TUtf16String> Synonyms;
    int MainIntentProps; // set of flags from EIntentProps
    bool CombineDupes;

    TIntentGroup();
    int operator&(IBinSaver &f);
    void AddIntent(const TVector<TUtf16String> &tabs);
    void AddMarker(const TVector<TUtf16String> &tabs);
    void AddPattern(const TVector<TUtf16String> &tabs);
    void AddSynonym(const TVector<TUtf16String> &tabs);
    void AddSiteDescription(const TVector<TUtf16String> &tabs);
    void AddGeobaseMarker(const TVector<const char*> &tabs);
    void AddCounterExample(const TUtf16String &what);
    void TestConsistency(const TString &name) const;
    int GetMask(const TUtf16String &req) const;
    bool HasSameObjects(const TIntentGroup &rhs) const;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef THashMap<TUtf16String, TIntentGroup> TClassifierScript;
bool LoadScriptFile(const TString &intentsFileName, TClassifierScript *res, bool utf8);
////////////////////////////////////////////////////////////////////////////////////////////////////
