#include "tlresult.h"
#include "rule.h"

#include <kernel/remorph/core/core.h>

#include <util/generic/algorithm.h>

namespace NTokenLogic {

TTokenLogicResult::TTokenLogicResult(const TString& rule, double priority, const std::pair<size_t, size_t>& origRange,
    const NPrivate::TMatchContext& ctx, const TVector<size_t>& track)
    : NMatcher::TResultBase(rule, priority, origRange, NRemorph::TMatchTrack(track, ctx.Range.first, ctx.Range.second))
{
    FillResult(ctx);
}

TTokenLogicResult::TTokenLogicResult(const TString& rule, double priority, const std::pair<size_t, size_t>& origRange,
    const NPrivate::TMatchContext& ctx)
    : NMatcher::TResultBase(rule, priority, origRange, NRemorph::TMatchTrack(ctx.Range.first, ctx.Range.second))
{
    FillResult(ctx);
}

void TTokenLogicResult::FillResult(const NPrivate::TMatchContext& ctx) {
    for (size_t i = 0; i < ctx.NamedMatches.size(); ++i) {
        // Update coordinates to be relative to the matched range
        Y_ASSERT(ctx.NamedMatches[i].second >= ctx.Range.first);
        Y_ASSERT(ctx.NamedMatches[i].second < ctx.Range.second);
        NamedTokens.push_back(std::make_pair(ctx.NamedMatches[i].first, ctx.NamedMatches[i].second - ctx.Range.first));
    }
    // Sort by both name and position
    ::StableSort(NamedTokens.begin(), NamedTokens.end());
    // Named tokens can be duplicated if they appear several times in a rule
    NamedTokens.erase(::Unique(NamedTokens.begin(), NamedTokens.end()), NamedTokens.end());

    if (ctx.Range.first != ctx.Range.second) {
        Y_ASSERT(ctx.Range.first < ctx.Contexts.size());
        Y_ASSERT(ctx.Range.second <= ctx.Contexts.size());
        MatchTrack.GetContexts().assign(ctx.Contexts.begin() + ctx.Range.first, ctx.Contexts.begin() + ctx.Range.second);
    }
}

void TTokenLogicResult::GetNamedRanges(NRemorph::TNamedSubmatches& ranges) const {
    // Convert positions of named tokens to single item ranges [pos, pos+1)
    for (TNamedTokens::const_iterator i = NamedTokens.begin(); i != NamedTokens.end(); ++i) {
        ranges.insert(std::make_pair(i->first, NRemorph::TSubmatch(i->second, i->second + 1)));
    }
}

} // NTokenLogic
