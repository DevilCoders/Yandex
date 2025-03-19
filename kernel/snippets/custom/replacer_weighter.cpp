#include "replacer_weighter.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/extended_length.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/lang_check.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/weight/weighter.h>

#include <library/cpp/langmask/langmask.h>

#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/string/printf.h>
#include <utility>

namespace NSnippets
{

class THandicapValues
{
    typedef TMap<std::pair<TString, TString>, double> THandicapTable;
    THandicapTable Table;

    double GetValue(const TString& current, const TString& pretender) {
        std::pair<TString, TString> directKey(current, pretender);
        THandicapTable::iterator ii = Table.find(directKey);
        if (ii != Table.end()) {
            return ii->second;
        }

        std::pair<TString, TString> inverseKey(pretender, current);
        ii = Table.find(inverseKey);
        if (ii != Table.end()) {
            return -ii->second;
        }

        return 0.0;
    }

public:
    THandicapValues()
    {
        // Handicap value is applied to the second headline_src when compared against the first
        Table[std::make_pair("", "yaca")]  = +2;
        Table[std::make_pair("", "dmoz")]  = +1;
    }

    // Get additional bias for the pretender's weight
    double GetValue(const TSpecSnippetCandidate* leader, const TSpecSnippetCandidate* pretender)
    {
        return GetValue(leader->Source, pretender->Source);
    }
};

struct TCandidateWeight
{
   const TReplaceContext& Context;
   double Weight;
   int Priority;
   const TSpecSnippetCandidate* Candidate;
   ELanguage Lang;
   TReplaceManager& Manager;
   ECompareAlgo Algo;

   TCandidateWeight(const TReplaceContext& context, TReplaceManager& manager, ELanguage lang, ECompareAlgo algo)
       : Context(context)
       , Weight(INVALID_SNIP_WEIGHT)
       , Priority(0)
       , Candidate(nullptr)
       , Lang(lang)
       , Manager(manager)
       , Algo(algo)
   {
   }

   bool HasCandidate()
   {
       return !!Candidate;
   }

   bool IsReplacer()
   {
       return Candidate && !!Candidate->Source;
   }

   double GetWeightRandom(const TSpecSnippetCandidate& candidate) const
   {
       TUtf16String hashSrc = candidate.Title.GetTitleString() + candidate.Text;
       return ComputeHash(hashSrc) & 0xFFFFFFFF;
   }

   double GetWeightSimpleHilite(const TSpecSnippetCandidate& candidate) const
   {
       TSimpleSnipCmp cmp(Context.QueryCtx, Context.Cfg, true, true);
       cmp.Add(candidate.Title);
       cmp.Add(candidate.Text);
       size_t totalLen = candidate.Title.GetTitleString().size();
       if (candidate.ExtendedText.Long) {
           totalLen += candidate.ExtendedText.Long.size();
       } else {
           totalLen += candidate.Text.size();
       }
       return cmp.GetWeight() + totalLen * 0.000001;
   }

   double GetWeightMxNet(const TSpecSnippetCandidate& candidate, double handicap) const
   {
       if (candidate.PrecalcedWeight != INVALID_SNIP_WEIGHT)
           return candidate.PrecalcedWeight;

       TVector<TUtf16String> source;
       source.push_back(candidate.Title.GetTitleString());
       source.push_back(candidate.Text);

       TRetainedSentsMatchInfo retainedInfo;
       retainedInfo.SetView(source, TRetainedSentsMatchInfo::TParams(Context.Cfg, Context.QueryCtx));
       const TSentsMatchInfo& smi = *retainedInfo.GetSentsMatchInfo();
       if (smi.WordsCount() == 0)
           return INVALID_SNIP_WEIGHT;

       TMxNetWeighter weighter(smi, Context.Cfg, Context.SnipWordSpanLen, &Context.SuperNaturalTitle, Context.LenCfg.GetMaxSnipLen());
       weighter.SetSpan(0, smi.WordsCount() - 1);
       double result = weighter.GetWeight();

       ISnippetCandidateDebugHandler* candidateHandler = Manager.GetCallback().GetCandidateHandler();
       if (candidateHandler && candidate.Source) {
           IAlgoTop* top = candidateHandler->AddTop(candidate.Source.data(), CS_METADATA);
           if (top) {
               Manager.GetCustomSnippets().RetainCopy(retainedInfo);
               TSingleSnip span(0, smi.WordsCount() - 1, smi);
               TSnip snip(span, result, weighter.GetFactors());
               top->Push(snip);
           }
       }

       return result + handicap;
   }

   double GetWeight(const TSpecSnippetCandidate& cand, double handicap)
   {
       switch(Algo) {
           case COMPARE_RANDOM:
               return GetWeightRandom(cand);
           case COMPARE_HILITE:
               return GetWeightSimpleHilite(cand);
           case COMPARE_MXNET:
           default:
               return GetWeightMxNet(cand, handicap);
       }
   }

   void CompareAndSwap(const TSpecSnippetCandidate* other) {
       if (!other)
           return;

       double handicapValue = Candidate ? Singleton<THandicapValues>()->GetValue(Candidate, other) : 0;
       double otherWeight = GetWeight(*other, handicapValue);

       if (otherWeight == INVALID_SNIP_WEIGHT) {
           return;
       }

       TReplaceResult result;
       result.UseText(other->Text, other->Source);
       if (other->Priority < Priority) {
           Manager.ReplacerDebug(Sprintf("new candidate has less priority than previous: %d < %d", other->Priority, Priority), result);
           return;
       }
       else if (Candidate && other->Priority == Priority) {
           if (Weight != INVALID_SNIP_WEIGHT && otherWeight < Weight) {
               return;
           }

           Manager.ReplacerDebug(
               Sprintf("new candidate (%s, language %s, priority %d) is better than previous candidate (%s): %.3f > %.3g, bias %.3f",
                    !!other->Source ? other->Source.data() : "natural",
                    NameByLanguage(Lang),
                    other->Priority,
                    Candidate ? (!!Candidate->Source ? Candidate->Source.data() : "natural") : "empty",
                    otherWeight,
                    Weight,
                    handicapValue),
               result);
       }
       else if (!Candidate) {
           Manager.ReplacerDebug(
                   Sprintf("initial candidate (%s, language %s, priority %d) has weight %.3g",
                       !!other->Source ? other->Source.data() : "natural",
                       NameByLanguage(Lang),
                       other->Priority,
                       otherWeight),
                   result);
       }
       else /* other->Priority > Priority */ {
           Manager.ReplacerDebug(
                   Sprintf("new candidate (%s, language %s) has more priority than previous candidate (%s): %d > %d",
                       !!other->Source ? other->Source.data() : "natural",
                       NameByLanguage(Lang),
                       !!Candidate->Source ? Candidate->Source.data() : "natural",
                       other->Priority,
                       Priority),
                   result);
       }

       Priority = other->Priority;
       Weight = otherWeight;
       Candidate = other;
   }
};

bool TReplacerWeighter::LangMatch(const TUtf16String& text)
{
    return Lang == LANG_UNK || HasWordsOfAlphabet(text, Lang);
}

int TReplacerWeighter::TitleUglinessLevel(const TSnipTitle& title)
{
    if (!LangMatch(title.GetTitleString())) {
        return 3;
    }
    const TSnip& titleSnip = title.GetTitleSnip();
    if (titleSnip.Snips.size() == 0) {
        return 3;
    }
    if (titleSnip.Snips.size() > 1) {
        return 2;
    }
    if (titleSnip.Snips.front().GetFirstWord() != 0) {
        return 2;
    }
    if (titleSnip.Snips.front().GetLastWord() != 0 &&
            titleSnip.Snips.front().GetLastWord() != title.GetSentsInfo()->WordCount() - 1) {
        return 1;
    }
    if (title.GetTitleString().length() < 15) {
        return 1;
    }
    return 0;
}

static const double PLM_SPAM_THRESHOLD = 0.0001;
static const double PLM_DIFF_THRESHOLD = 0.32;

bool TReplacerWeighter::HasBetterTitle(const TSpecSnippetCandidate* chosenOne)
{
    if (!(chosenOne->Title.GetTitleString()))
        return false;

    if (Context.Cfg.ShortYacaTitles() &&
        chosenOne->Title.GetTitleString().size() <= Context.Cfg.ShortYacaTitleLen() &&
        Context.NaturalTitle.GetTitleString().size() > Context.Cfg.ShortYacaTitleLen())
    {
        return true;
    }

    double naturalWeight = TSimpleSnipCmp(Context.QueryCtx, Context.Cfg, false, true)
        .Add(Context.NaturalTitle).AddUrl(Context.Url).GetWeight();
    double replacerWeight = TSimpleSnipCmp(Context.QueryCtx, Context.Cfg, false, true)
        .Add(chosenOne->Title).AddUrl(Context.Url).GetWeight();

    if (replacerWeight < naturalWeight) {
        return false;
    } else if (replacerWeight > naturalWeight) {
        return true;
    }

    int naturalUgliness = TitleUglinessLevel(Context.NaturalTitle);
    int replacerUgliness = TitleUglinessLevel(chosenOne->Title);
    if (naturalUgliness < replacerUgliness)
        return false;
    else if (naturalUgliness > replacerUgliness)
        return true;

    double plmNaturalScore = Context.NaturalTitle.GetPLMScore();
    double plmReplacerScore = chosenOne->Title.GetPLMScore();

    if (plmNaturalScore != -Max<double>() || plmReplacerScore != -Max<double>()) {
        double minPlm = Min(plmNaturalScore, plmReplacerScore);
        if (minPlm == -Max<double>()) {
            return plmReplacerScore > plmNaturalScore;
        }

        if (plmReplacerScore > -PLM_SPAM_THRESHOLD || plmNaturalScore > -PLM_SPAM_THRESHOLD) {
            return true;
        }

        if (Abs((plmNaturalScore - plmReplacerScore) / minPlm) > PLM_DIFF_THRESHOLD) {
            return plmReplacerScore > plmNaturalScore;
        }
    }

    bool titleLengthPrevails =
        Context.NaturalTitle.GetTitleString().length() > chosenOne->Title.GetTitleString().length() * 1.10;
    return titleLengthPrevails;
}

ESubReplacerResult TReplacerWeighter::PerformReplace(TReplaceManager& manager, const EMarker marker,
                                                     const TList<TSpecSnippetCandidate>& candidates, ECompareAlgo algo)
{
    if (candidates.empty()) {
        return ReplacementNotYetFound;
    }

    TSpecSnippetCandidate natural;
    TCandidateWeight weight(Context, manager, Lang, algo);

    bool docLangMatches = (Context.DocLangId == LANG_UNK || Lang == Context.DocLangId);

    if (!Context.IsByLink && docLangMatches) {
        natural.DontReplace = true;
        natural.Text = Context.Snip.GetRawTextWithEllipsis();
        natural.PrecalcedWeight = Context.Snip.Weight;
        weight.CompareAndSwap(&natural);
    }

    for (auto&& candidate : candidates) {
        weight.CompareAndSwap(&candidate);
    }

    if (!weight.HasCandidate())
        return ReplacementNotYetFound;

    const TSpecSnippetCandidate* best = weight.Candidate;
    if (best->DontReplace)
        return AbortReplacing;

    TReplaceResult result;
    result.UseText(best->ExtendedText, best->Source);
    if (HasBetterTitle(best))
        result.UseTitle(best->Title);
    manager.Commit(result, marker);

    return ReplacementFound;
}

}
