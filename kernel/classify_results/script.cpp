#include <ysite/yandex/doppelgangers/normalize.h>
#include <kernel/yawklib/wtrutil.h>
#include <util/charset/wide.h>
#include <util/stream/file.h>
#include <library/cpp/string_utils/url/url.h>
#include <kernel/yawklib/phrase_iterator.h>
#include <kernel/yawklib/script_aliases.h>
#include "script.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool StartsWith(const TUtf16String &s1, const TPatternElem &s2)
{
    if (!StartsWith(s1, s2.Word))
        return false;
    if (!s2.ExactWords)
        return true;
    if (s1.size() == s2.Word.size())
        return true;
    if (s1[s2.Word.size()] == ' ')
        return true;
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool EndsWith(const TUtf16String &s1, const TPatternElem &s2)
{
    if (!EndsWith(s1, s2.Word))
        return false;
    if (!s2.ExactWords)
        return true;
    if (s1.size() == s2.Word.size())
        return true;
    if (s1[s1.size() - (s2.Word.size()) - 1] == ' ')
        return true;
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsRegex(const TUtf16String &phrase) {
    return phrase.find_first_of('\\') != TUtf16String::npos ||
           phrase.find_first_of('(')  != TUtf16String::npos ||
           phrase.find_first_of('{')  != TUtf16String::npos;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static TUtf16String WholeLineRx(const TUtf16String& pattern) {
    return u"^" + pattern + u"$";
}
////////////////////////////////////////////////////////////////////////////////////////////////////
TUtf16String MakeUniformStyle(const TUtf16String &phrase)
{
    TDoppelgangersNormalize impl(false, false, false);
    return impl.Normalize(phrase, false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static TUtf16String CanonScriptSample(const TUtf16String& phrase) {
    if (IsRegex(phrase)) {
        return WholeLineRx(phrase);
    }
    return MakeUniformStyle(phrase);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TIntent::HasSameMatches(const TIntent &rhs) const
{
    return rhs.Wordings == Wordings && rhs.Buzzwords == Buzzwords;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TPatternElem::operator == (const TPatternElem &rhs) const
{
    return rhs.Word == Word && rhs.ExactWords == ExactWords;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int TPattern::operator&(IBinSaver &f)
{
    f.Add(2,&Examples);
    f.Add(3,&PatternsBegin);
    f.Add(4,&PatternsEnd);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TPattern::IsMatch(const TUtf16String &what, int* /*id*/) const {
    if (Examples.IsMatch(what))
        return true;
    for (int n = 0; n < PatternsBegin.ysize(); ++n)
        if (StartsWith(what, PatternsBegin[n]))
            return true;
    for (int n = 0; n < PatternsEnd.ysize(); ++n)
        if (EndsWith(what, PatternsEnd[n]))
            return true;
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TPattern::AddExampleImpl(const TUtf16String &ex) {
    if (ex.empty())
        return;
    if (ex[0] == '*' && ex.back() == '*')
        fprintf(stderr, "Warning: wildcards with multiple asterix not supported\n"
                        "Suspicious line: %s\n", WideToUTF8(ex).data());

    TUtf16String what = CanonScriptSample(ex);
    if (IsRegex(what)) {
        Examples.AddRx(what);
    } else if (ex[0] == '*') {
        PatternsEnd.emplace_back();
        if (ex.size() > 1 && ex[1] == ' ')
            PatternsEnd.back().ExactWords = true;
        PatternsEnd.back().Word = what;
    } else if (ex.back() == '*') {
        PatternsBegin.emplace_back();
        if (ex.size() > 1 && ex[ex.size() - 2] == ' ')
            PatternsBegin.back().ExactWords = true;
        PatternsBegin.back().Word = what;
    } else {
        if (ex.find('*') != TUtf16String::npos) {
            fprintf(stderr, "Warning: wildcards currently supported only in the beginning and at the end\n"
                            "Suspicious line: %s\n", WideToUTF8(ex).data());
        }
        Examples.AddSimple(what);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TPattern::AddExample(const TUtf16String &templ) {
    THolder<IPhraseIterator> iter(CreatePhraseIterator(templ));
    do {
        AddExampleImpl(iter->Get());
    } while (iter->Next());
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TPattern::operator == (const TPattern &rhs) const
{
    return rhs.Examples == Examples && rhs.PatternsBegin == PatternsBegin && rhs.PatternsEnd == PatternsEnd;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int TGeobaseMarker::operator&(IBinSaver &f) {
    f.Add(2,&GeoTypes);
    f.Add(3,&ParentRegions);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
TIntentGroup::TIntentGroup()
    : NumMarkerTypes(0)
    , IsUniversalIntent(false)
    , MainIntentProps(IPROP_NONE)
    , CombineDupes(true)
{}
////////////////////////////////////////////////////////////////////////////////////////////////////
int TIntentGroup::operator&(IBinSaver &f) {
    f.Add(2,&ExamplesWithSynonims);
    f.Add(3,&Markers);
    f.Add(4,&AntiMarkers);
    f.Add(5,&Intents);
    f.Add(6,&NumMarkerTypes);
    f.Add(7,&AntiPattern);
    f.Add(8,&PatternMarkers);
    f.Add(9,&IsUniversalIntent);
    f.Add(10,&IsIn);
    f.Add(11,&IsNotIn);
    f.Add(12,&MarkerSites);
    f.Add(13,&Synonyms);
    f.Add(14,&MainIntentProps);
    f.Add(15,&MarkerWikiCategs);
    f.Add(16,&GeobaseMarkers);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TIntentMarker
{
    const char* Marker;
    EIntentProps Prop;
};
static TIntentMarker IntentMarkers[] =
{
    { "prop:local", IPROP_LOCAL },
    { "prop:needlocal", IPROP_NEED_LOCAL },
    { "prop:needcountry", IPROP_NEED_COUNTRY },
    { "prop:fresh", IPROP_FRESH },
    { "prop:hard", IPROP_HARD },
    { nullptr, IPROP_NONE }
};
static bool AddPropByMarker(const TString &field, int *res) {
    for (TIntentMarker *it = IntentMarkers; it->Marker; ++it) {
        if (field == it->Marker) {
            *res |= it->Prop;
            return true;
        }
    }
    if (field.StartsWith("prop:"))
        fprintf(stderr, "Warning: unknown property descriptor %s\n", field.data());
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static TUtf16String RemoveDoubleSpaces(const TUtf16String &wording) {
    TUtf16String ret;
    if (!wording.empty() && wording[0] != ' ')
        ret.push_back(wording[0]);
    for (size_t n = 1; n < wording.size(); ++n) {
        if (wording[n] != ' ' || wording[n - 1] != ' ')
            ret.push_back(wording[n]);
    }
    if (!ret.empty() && ret.back() == ' ')
        ret.pop_back();
    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TIntentGroup::AddIntent(const TVector<TUtf16String> &tabs) {
    const TUtf16String optPhraseMarker = u"optional:";
    const TUtf16String nameMarker = u"uid:";
    const TUtf16String satisfiesMarker = u"satisfies:";
    const TUtf16String experimentMarker = u"experiment:";
    const TUtf16String disallowMarker = u"disallow:";
    Intents.emplace_back();
    TIntent &added = Intents.back();
    TUtf16String experimentTag;
    bool buzzwordsStarted = false;
    for (int n = 2; n < tabs.ysize(); ++n) {
        if (tabs[n] == optPhraseMarker) {
            buzzwordsStarted = true;
        } else if (StartsWith(tabs[n], nameMarker)) {
            added.UniqueName = tabs[n].substr(nameMarker.size());
            added.Props |= IPROP_HAS_GLOBAL_UID;
            if (added.UniqueName.find(':') != TUtf16String::npos)
                fprintf(stderr, "warning: uid contains colon(s), everything after the last colon will be treated as an experiment tag\n");
        } else if (StartsWith(tabs[n], satisfiesMarker)) {
            added.Satisfies.push_back(tabs[n].substr(satisfiesMarker.size()));
        } else if (StartsWith(tabs[n], experimentMarker)) {
            experimentTag = tabs[n].substr(experimentMarker.size());
        } else if (StartsWith(tabs[n], disallowMarker)) {
            added.QueriesDisallowed.push_back(tabs[n].substr(disallowMarker.size()));
        } else if (AddPropByMarker(WideToUTF8(tabs[n]), &added.Props)) {
            continue;
        } else {
            if (tabs[n].find(':') != TUtf16String::npos)
                fprintf(stderr, "Warning: intent contains a semicolon, but it isn't a known directive. Most probably it's an error\n"
                                "It's recommended to check space(s) vs tab(s) in %s\n", WideToUTF8(tabs[n]).data());
            THolder<IPhraseIterator> iter(CreatePhraseIterator(tabs[n]));
            do {
                TUtf16String wordingAsInFile = iter->Get();
                TUtf16String wording = MakeUniformStyle(wordingAsInFile);
                if (!wording.empty()) {
                    if (buzzwordsStarted) {
                        added.Buzzwords.push_back(wording);
                    } else {
                        added.WordingsAsInScript.push_back(RemoveDoubleSpaces(wordingAsInFile));
                        added.Wordings.push_back(wording);
                    }
                }
            } while (iter->Next());
        }
    }
    if (added.UniqueName.empty())
        added.UniqueName = added.Wordings[0];
    if (!experimentTag.empty()) {
        added.UniqueName.push_back(':');
        added.UniqueName += experimentTag;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TIntentGroup::AddMarker(const TVector<TUtf16String> &tabs) {
    ++NumMarkerTypes;
    const TUtf16String optPhraseMarker = u"optional:";
    for (int n = 2; n < tabs.ysize(); ++n) {
        if (tabs[n] == optPhraseMarker)
            break;
        THolder<IPhraseIterator> iter(CreatePhraseIterator(tabs[n]));
        do {
            const TUtf16String& example = CanonScriptSample(iter->Get());
            if (IsRegex(example)) {
                Markers.AddRx(example, NumMarkerTypes);
            } else {
                Markers.AddSimple(example, NumMarkerTypes);
            }
        } while (iter->Next());
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TIntentGroup::AddPattern(const TVector<TUtf16String> &tabs) {
    ++NumMarkerTypes;
    if (PatternMarkers.ysize() < NumMarkerTypes)
        PatternMarkers.resize(NumMarkerTypes);
    TPattern &dst = PatternMarkers.back();
    for (int n = 2; n < tabs.ysize(); ++n)
        dst.AddExample(tabs[n]);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TIntentGroup::AddSynonym(const TVector<TUtf16String> &tabs) {
    for (int n = 2; n < tabs.ysize(); ++n) {
        THolder<IPhraseIterator> iter(CreatePhraseIterator(tabs[n]));
        do {
            Synonyms.push_back(MakeUniformStyle(iter->Get()));
        } while (iter->Next());
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TIntentGroup::AddCounterExample(const TUtf16String &what) {
    AntiPattern.AddExample(what);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TIntentGroup::AddSiteDescription(const TVector<TUtf16String> &tabs) {
    if (tabs.size() < 3) {
        fprintf(stderr, "Wrong format for 'site' directive, not enough fields\n");
        return;
    }
    TString site = WideToUTF8(tabs[2]);
    if (!IsStringASCII(tabs[2].begin(), tabs[2].end()) || site.find('.') == TString::npos) {
        fprintf(stderr, "Suspicious site name '%s', something resembling an URL expected\n", site.data());
    }
    site = CutWWWPrefix(CutHttpPrefix(site));
    THashMap<TUtf16String, size_t> wordings2intents;
    for (size_t n = 0; n < Intents.size(); ++n) {
        const TVector<TUtf16String> &wordings = Intents[n].Wordings;
        for (size_t m = 0; m < wordings.size(); ++m)
            wordings2intents[wordings[m]] = n;
    }
    TVector<int> intentsOk(Intents.size());
    for (int n = 3; n < tabs.ysize(); ++n) {
        THashMap<TUtf16String, size_t>::const_iterator it = wordings2intents.find(tabs[n]);
        if (it == wordings2intents.end())
            fprintf(stderr, "Unknown intent description '%s' in site description for %s, consider spelling, or add it to intents list\n",
                WideToUTF8(tabs[n]).data(), site.data());
        else
            intentsOk[it->second] = 1;
    }
    for (size_t n = 0; n < Intents.size(); ++n)
        if (!intentsOk[n])
            Intents[n].SitesDisallowed.push_back(site);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TIntentGroup::AddGeobaseMarker(const TVector<const char*> &tabs) {
    if (tabs.size() < 3) {
        fprintf(stderr, "Wrong format for 'markergeo' directive, not enough fields\n");
        return;
    }
    GeobaseMarkers.emplace_back();
    TGeobaseMarker& dst = GeobaseMarkers.back();
    for (size_t n = 2; n < tabs.size(); ++n) {
        TString tab = tabs[n];
        if (tab.StartsWith("parent:"))
            dst.ParentRegions.push_back(FromString<int>(tab.substr(7)));
        else if (tab.StartsWith("type:"))
            dst.GeoTypes.push_back(FromString<int>(tab.substr(5)));
        else
            fprintf(stderr, "Wrong format for 'markergeo' directive, every field should be in form parent:RegionId or type:Type");
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool HasUniqueName(const TVector<TIntent>& intents, const TUtf16String& uid) {
    for (size_t n = 0; n < intents.size(); ++n)
        if (intents[n].UniqueName == uid)
            return true;
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TIntentGroup::TestConsistency(const TString &name) const {
    if (NumMarkerTypes > 20)
        fprintf(stderr, "Warning: too many marker groups (over 30) for %s\n", name.data());
    for (THashMap<TUtf16String, TUtf16String>::const_iterator it = ExamplesWithSynonims.begin(); it != ExamplesWithSynonims.end(); ++it) {
        if (AntiPattern.IsMatch(it->first))
            fprintf(stderr, "Error: wrong example %s for group %s, prohibited by the antipattern rules\n",
                WideToUTF8(it->first).data(), name.data());
    }
    THashMap<TUtf16String, size_t> wordings2intents;
    for (size_t n = 0; n < Intents.size(); ++n) {
        if (Intents[n].UniqueName.empty())
            fprintf(stderr, "Warning in %s: intent %d has empty unique name\n", name.data(), (int)n);
        for (size_t m = 0; m < Intents[n].Satisfies.size(); ++m) {
            TUtf16String second = Intents[n].Satisfies[m];
            if (!HasUniqueName(Intents, second))
                fprintf(stderr, "Warning in %s: intent satisfies %s, but no intent with this unique name found in category\n",
                    name.data(), WideToUTF8(second).data());
        }
        const TVector<TUtf16String> &wordings = Intents[n].Wordings;
        for (size_t m = 0; m < wordings.size(); ++m) {
            THashMap<TUtf16String, size_t>::const_iterator it = wordings2intents.find(wordings[m]);
            if (it == wordings2intents.end())
                wordings2intents[wordings[m]] = n;
            else if (it->second != n) {
                TUtf16String intent1 = Intents[it->second].UniqueName;
                TUtf16String intent2 = Intents[n].UniqueName;
                fprintf(stderr, "Warning in %s: intent wording [%s] is ambigious, could be %s (intent #%d) or %s (intent #%d)\n",
                    name.data(), WideToUTF8(wordings[m]).data(),
                    WideToUTF8(intent1).data(), (int)it->second,
                    WideToUTF8(intent2).data(), (int)n);
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int TIntentGroup::GetMask(const TUtf16String &req) const {
    if (AntiPattern.IsMatch(req))
        return 1;
    int res = 0;
    for (int n = 0; n < PatternMarkers.ysize(); ++n)
        if (PatternMarkers[n].IsMatch(req))
            res |= (1 << (n + 1));
    return res;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TIntentGroup::HasSameObjects(const TIntentGroup &rhs) const
{
    if (Intents.size() != rhs.Intents.size())
        return false;
    for (size_t n = 0; n < Intents.size(); ++n)
        if (!Intents[n].HasSameMatches(rhs.Intents[n]))
            return false;

    return Markers == rhs.Markers && Equals(rhs.AntiMarkers, AntiMarkers) &&
        AntiPattern == rhs.AntiPattern && PatternMarkers == rhs.PatternMarkers && MarkerSites == rhs.MarkerSites &&
        Synonyms == rhs.Synonyms && MarkerWikiCategs == rhs.MarkerWikiCategs;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template <>
void LoadScriptLine(TClassifierScript *res, const TVector<const char*> &tabsS, const TVector<TUtf16String> &tabs)
{
    TUtf16String groupName = tabs[0];
    TString what = tabsS[1];
    TIntentGroup &g = (*res)[groupName];
    if (tabs.ysize() == 2) {
        if (AddPropByMarker(what, &g.MainIntentProps)) {
            return;
        } else if (tabs[1] == u"duplicates:no") {
            g.CombineDupes = false;
        } else {
            TUtf16String example = tabs[1];
            if (example[0] == '-')
                g.AddCounterExample(example.substr(1));
            else
                g.ExamplesWithSynonims[MakeUniformStyle(example)];
        }
    } else if (what == "intent") {
        g.AddIntent(tabs);
    } else if (what == "marker") {
        g.AddMarker(tabs);
    } else if (what == "synonym") {
        g.AddSynonym(tabs);
    } else if (what == "universal-intent") {
        g.AddIntent(tabs);
        g.AddMarker(tabs);
        g.IsUniversalIntent = true;
    } else if (what == "antimarker") {
        for (int n = 2; n < tabs.ysize(); ++n)
            g.AntiMarkers.insert(MakeUniformStyle(tabs[n]));
    } else if (what == "pattern") {
        g.AddPattern(tabs);
    } else if (what == "isin") {
        for (int n = 2; n < tabs.ysize(); ++n)
            g.IsIn.push_back(tabs[n]);
    } else if (what == "isnotin") {
        for (int n = 2; n < tabs.ysize(); ++n)
            g.IsNotIn.push_back(tabs[n]);
    } else if (what == "markersite") {
        ++g.NumMarkerTypes;
        for (int n = 2; n < tabs.ysize(); ++n)
            g.MarkerSites.push_back(TString{CutWWWPrefix(CutHttpPrefix(tabsS[n]))});
    } else if (what == "markerwiki") {
        ++g.NumMarkerTypes;
        for (int n = 2; n < tabs.ysize(); ++n)
            g.MarkerWikiCategs.push_back(MakeUniformStyle(tabs[n]));
    } else if (what == "markergeo") {
        ++g.NumMarkerTypes;
        g.AddGeobaseMarker(tabsS);
    } else if (what == "site") {
        g.AddSiteDescription(tabs);
    } else if (what == "example") {
        TUtf16String example = tabs[2];
        TUtf16String &synonims = g.ExamplesWithSynonims[MakeUniformStyle(example)];
        for (int n = 3; n < tabsS.ysize(); ++n) {
            if (!synonims.empty())
                synonims += '\t';
            synonims += MakeUniformStyle(tabs[n]);
        }
    } else
        fprintf(stderr, "cannot understand command: %s\n", what.data());
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool LoadScriptFile(const TString &intentsFileName, TClassifierScript *res, bool utf8)
{
    if (!LoadTabDelimitedScript(intentsFileName, res, utf8? CODES_UTF8 : CODES_WIN))
        return false;
    for (TClassifierScript::const_iterator it = res->begin(); it != res->end(); ++it) {
        TString name = WideToUTF8(it->first);
        it->second.TestConsistency(name);
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
