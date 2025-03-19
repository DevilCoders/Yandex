#pragma once

#include "node_input_symbol.h"
#include "gzt_result_iter.h"

#include <kernel/remorph/proc_base/matcher_base.h>

#include <kernel/qtree/richrequest/richnode_fwd.h>
#include <kernel/qtree/richrequest/nodeiterator.h>

#include <kernel/gazetteer/richtree/gztres.h>

#include <library/cpp/langmask/langmask.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>


namespace NReMorph {

namespace NRichNode {

// Creates the collection of input symbols for given vector of rich nodes. If ignoreDelimiters = true then
// collection of symbols is mapped one to one to the source collection of nodes. Otherwise, the function creates
// additional input symbols for delimiters.
// Note: Gazetteer results are applied to rich nodes, so they don't take into account delimiters if ignoreDelimiters = false.
TNodeInputSymbols CreateInputSymbols(const TConstNodesVector& nodes, const TLangMask& queryLang = TLangMask(),
    bool ignoreDelimiters = true);

// Creates the collection of input symbols for given vector of rich nodes and additional input symbols
// for delimiters from the given set. The 'nodesToSymbols' collection contains offset of each node in the created vector of symbols.
// Attention: The returned collection of input symbols will be greater than collection of nodes,
// and TNodeInputSymbols cannot be directly mapped to TConstNodesVector.
// Use TNodeInputSymbol::GetRichNode() method to retrieve associated nodes (GetRichNode() returns NULL for delimiters).
TNodeInputSymbols CreateInputSymbols(const TConstNodesVector& nodes, TVector<size_t>& nodesToSymbols,
    const TLangMask& queryLang = TLangMask(),
    const NSorted::TSortedVector<wchar16>& delimiters = NSorted::TSortedVector<wchar16>());

// Creates reverse mapping from symbol position to node position.
// Use nodePos = res[symbolPos] to get a node position from the remorph result.
TVector<size_t> CreateSymbolsToNodesMap(const TVector<size_t>& nodesToSymbols, size_t countOfSymbols);

// Creates the collection of input symbols for given vector of rich nodes and gazetteer results. It uses the
// best non-conflicting coverage of gazetteer results, and creates multi-symbols for multi-word gazetteer articles.
// It creates additional input symbols for delimiters if the ignoreDelimiters = false.
// Note: Gazetteer results are applied to rich nodes, so they don't take into account delimiters if ignoreDelimiters = false.
// By default, the function ignores gazetteer results, which cover a comma. Use NULL for "filter" parameter
// to include all gazetteer results.
// Returns true if the created collection of symbols has required articles, and returns false, if the matcher
// definitely cannot find anything in the created collection.
bool CreateInputSymbols(TNodeInputSymbols& symbols, const NMatcher::TMatcherBase& matcher,
    const TConstNodesVector& nodes, const TGztResults& gztResults, const TLangMask& queryLang = TLangMask(),
    TGztResultFilterFunc filter = &IsDividedByComma, bool ignoreDelimiters = true);

// Creates the input for given vector of rich nodes and gazetteer results. It creates separate branches
// for all possible non-conflicting gazetteer coverage.
// It creates additional input symbols for delimiters if the ignoreDelimiters = false.
// Note: Gazetteer results are applied to rich nodes, so they don't take into account delimiters if ignoreDelimiters = false.
// By default, the function ignores gazetteer results, which cover a comma. Use NULL for "filter" parameter
// to include all gazetteer results.
// Returns true if the created input has at list one branch with the required article, and returns false, if the matcher
// definitely cannot find anything in the created input.
bool CreateInput(TNodeInput& input, const NMatcher::TMatcherBase& matcher,
    const TConstNodesVector& nodes, const TGztResults& gztResults, const TLangMask& queryLang = TLangMask(),
    TGztResultFilterFunc filter = &IsDividedByComma, bool ignoreDelimiters = true);

} // NRichNode

} // NReMorph
