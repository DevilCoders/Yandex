#include "dynamic_list_replacer.h"

#include <library/cpp/string_utils/base64/base64.h>

#include <util/charset/unidata.h>
#include <util/digest/murmur.h>
#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/iterator.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/string/split.h>
#include <util/system/yassert.h>


namespace NFacts {


const size_t MAX_NUMBER_OF_WORDS_IN_FACT_TEXT_FOR_REPLACEMENT = 80;
const size_t MAX_NUMBER_OF_WORDS_IN_SERIALIZED_LIST_INDEX = 200;

static const NSc::TJsonOpts JSON_OPTS_STRICT = NSc::TJsonOpts(
        NSc::TJsonOpts::JO_SORT_KEYS |
        NSc::TJsonOpts::JO_PARSER_STRICT_JSON |
        NSc::TJsonOpts::JO_PARSER_STRICT_UTF8 |
        NSc::TJsonOpts::JO_PARSER_DISALLOW_COMMENTS |
        NSc::TJsonOpts::JO_PARSER_DISALLOW_DUPLICATE_KEYS);


using TSlice = TArrayRef<const THashVector::value_type>;


// Find an LCS (longest common slice) between A and B longer than or equal to the given limit.
// This helper function does not shift A and B relative to each other and, thus, performs LCS on fixed vectors.
// Valid LCS must have size greater than or equal to cSizeMinAllowed, which in turn must be greater or equal to 1.
// If valid LCS found, return its size and assign cStart with the offset to the origin of the LCS found.
// If there is no valid LCS, return 0.
static inline size_t FindLongestCommonSliceHelper(const TSlice& a, const TSlice& b, size_t& cStart, size_t cSizeMinAllowed) {
    Y_ASSERT(cSizeMinAllowed >= 1);

    // Exclude unmatched suffix from comparison. Do not modify slices themselves, just use the virtual limit.
    const size_t abSize = std::min(a.size(), b.size());

    // Cancel this call if it is definitely not capable of improving the global result.
    if (abSize < cSizeMinAllowed) {
        return 0;
    }

    // Find the longest common subslice C between A and B slices.

    size_t cSize = 0;
    size_t cSizeMax = cSizeMinAllowed - 1;  // Force rejection of a priory invalid LCSs

    for (size_t i = 0; i < abSize; ++i) {
        if (a[i] == b[i]) {
            ++cSize;

        } else {
            if (cSize > cSizeMax) {
                cSizeMax = cSize;
                Y_ASSERT(i >= cSize);
                cStart = i - cSize;
            }

            cSize = 0;
        }
    }

    if (cSize > cSizeMax) {
        cSizeMax = cSize;
        Y_ASSERT(abSize >= cSize);
        cStart = abSize - cSize;
    }

    // Valid LCS must have size greater than or equal to cSizeMinAllowed.
    return (cSizeMax >= cSizeMinAllowed) ? cSizeMax : 0;
}


size_t FindLongestCommonSlice(const THashVector& a, const THashVector& b, size_t& aToCommon, size_t& bToCommon, size_t minAllowedCommonSliceSize) {

    // Inspired by https://en.wikipedia.org/wiki/Longest_common_substring_problem#Dynamic_programming
    //
    // Unlike the code from Wikipedia, this one:
    // - establishes 'diagonal' cycles over the DP matrix instead of 'horizontal and vertical' cycles
    // - and thus requires O(1) additional memory in virtual DP matrix for storing results from DP subtasks
    //


    // With the outer loop we will shift B vector relative to A vector, thus, selecting one of diagonals of the virtual DP matrix.
    //
    // The shift of B vector will have sign according to the rule:
    //
    // - positive shift - B is shifted to the right of A origin
    //                    AAAAAAAAAAAA
    //                    |+++++>|
    //                           BBBBBBBB
    //
    // - negative shift - B is shifted to the left of A origin
    //                    AAAAAAAAAAAA
    //               |<---|
    //               BBBBBBBB
    //


    // Obviously there is no common slice C with size above some limit if any of the given A and B vectors are shorter than the desired limit of LCS size.
    if (a.size() < minAllowedCommonSliceSize || b.size() < minAllowedCommonSliceSize) {
        return 0;
    }


    // Determine the limits for shifting B vector relative to A vector.
    // Note that after shifting B, the intersection of A and B must not be shorter than the desired limit of the LCS size.
    const size_t shiftPosLimit = a.size() - minAllowedCommonSliceSize;
    const size_t shiftNegLimit = b.size() - minAllowedCommonSliceSize;


    size_t maxCommonSliceSize = 0;


    // Traverse diagonals of the virtual DP matrix in the following order:
    //
    //     shift = [0, +1, -1, +2, -2, +3, -3, ... +N, -N], where N = max(|A|, |B|)
    //
    // This sequence gives the optimal order, because the search starts with the longest diagonal (shift = 0) and proceedes with more short ones (up to shift = ±N).
    // Note, that positive and negative subsequences may be cancelled in advance ones it became clear that there is no longer LCS in the rest of the subsequence.
    //
    // Establish loop for absolute vaue of the shift in the range from 0 to N.
    //
    const size_t shiftAbsLimit = std::max(shiftPosLimit, shiftNegLimit);
    for (size_t shiftAbs = 0; shiftAbs <= shiftAbsLimit; ++shiftAbs) {


        // Positive subsequence - B is shifted to the right of A origin.
        //                    AAAAAAAAAAAA
        //                    |+++++>|
        //                           BBBBBBBB
        //
        if (shiftAbs <= shiftPosLimit) {
            // Remove the unmatched prefix of A.
            const TSlice aSlice = TSlice(a).subspan(shiftAbs);
            const TSlice bSlice = TSlice(b);

            // Find LCS between fixed vectors having size greater or equal to cSizeMinAllowed. If one found, cSizeMax will be strictly greater than 0.
            size_t cStart = 0;
            const size_t cSizeMax = FindLongestCommonSliceHelper(aSlice, bSlice, cStart, /*cSizeMinAllowed*/ std::max(maxCommonSliceSize + 1, minAllowedCommonSliceSize));

            // Update the global longest common subslice if the better case is found.
            if (cSizeMax) {
                maxCommonSliceSize = cSizeMax;
                aToCommon = cStart + shiftAbs;
                bToCommon = cStart;
            }
        }


        // Negative subsequence - B is shifted to the left of A origin.
        //                    AAAAAAAAAAAA
        //               |<---|
        //               BBBBBBBB
        //
        if (shiftAbs <= shiftNegLimit && shiftAbs > 0) {
            // Remove the unmatched prefix of B.
            const TSlice aSlice = TSlice(a);
            const TSlice bSlice = TSlice(b).subspan(shiftAbs);

            // Find LCS between fixed vectors having size greater or equal to cSizeMinAllowed. If one found, cSizeMax will be strictly greater than 0.
            size_t cStart = 0;
            const size_t cSizeMax = FindLongestCommonSliceHelper(aSlice, bSlice, cStart, /*cSizeMinAllowed*/ std::max(maxCommonSliceSize + 1, minAllowedCommonSliceSize));

            // Update the global longest common subslice if the better case is found.
            if (cSizeMax) {
                maxCommonSliceSize = cSizeMax;
                aToCommon = cStart;
                bToCommon = cStart + shiftAbs;
            }
        }
    }

    return maxCommonSliceSize;
}


size_t MatchFactTextWithListCandidate(const THashVector& factTextHash, const NSc::TDict& listCandidateIndex, size_t& numberOfHeaderElementsToSkip) {
    THashVector listBodyHash;
    size_t listBodyHashDataSize = 0;
    {
        const TStringBuf bodyHash = listCandidateIndex.Get("body_hash").GetString();
        listBodyHash.resize((Base64DecodeBufSize(bodyHash.Size()) + 1) / 2);
        listBodyHashDataSize = Base64StrictDecode(listBodyHash.data(), bodyHash.begin(), bodyHash.end());
        listBodyHash.resize((listBodyHashDataSize + 1) / 2);
    }

    Y_ENSURE(listBodyHash.size() <= MAX_NUMBER_OF_WORDS_IN_SERIALIZED_LIST_INDEX, "Too many words in the serialized list index");

    // At least 50% of the original fact text must coincide with the list body.
    const size_t minAllowedCommonSliceSize = (factTextHash.size() + 1) / 2;

    size_t aToCommon;
    size_t bToCommon;
    const size_t maxCommonSliceSize = FindLongestCommonSlice(factTextHash, listBodyHash, aToCommon, bToCommon, minAllowedCommonSliceSize);

    if (maxCommonSliceSize < minAllowedCommonSliceSize) {
        return 0;
    }

    const size_t factTextToCommonPart = aToCommon;
    const size_t listBodyToCommonPart = bToCommon;

    // Either the original fact text may contain some leading extra text, or the list candidate header may contain some text not included into the original fact, but not both of them simultaneously.
    if (factTextToCommonPart > 0 && listBodyToCommonPart > 0) {
        return 0;
    }

    const size_t factTextToEndOfCommonPart = factTextToCommonPart + maxCommonSliceSize;
    const size_t listBodyToEndOfCommonPart = listBodyToCommonPart + maxCommonSliceSize;

    // Either the original fact text may contain some extra trailing text, or the list candidate last item(s) may contain some text not included into the original fact, but not both of them simultaneously.
    if (factTextToEndOfCommonPart < factTextHash.size() && listBodyToEndOfCommonPart < listBodyHash.size()) {
        return 0;
    }

    TVector<size_t> listHeaderElementLengths;
    size_t listHeaderElementLengthsTotal = 0;
    {
        const NSc::TArray& headerLens = listCandidateIndex.Get("header_lens").GetArray();
        listHeaderElementLengths.reserve(headerLens.size());
        for (const NSc::TValue& headerLen : headerLens) {
            i64 length = headerLen.GetIntNumber();
            Y_ENSURE(length >= 0, "Invalid header_lens value");
            listHeaderElementLengths.push_back(length);
            listHeaderElementLengthsTotal += length;
        }
    }

    TVector<size_t> listItemElementLengths;
    size_t listItemElementLengthsTotal = 0;
    {
        const NSc::TArray& itemLens = listCandidateIndex.Get("items_lens").GetArray();
        listItemElementLengths.reserve(itemLens.size());
        for (const NSc::TValue& itemLen : itemLens) {
            i64 length = itemLen.GetIntNumber();
            Y_ENSURE(length > 0, "Invalid items_lens value");
            listItemElementLengths.push_back(length);
            listItemElementLengthsTotal += length;
        }
    }

    Y_ENSURE(listHeaderElementLengths.size() >= 1, "Not enough header elements described");
    Y_ENSURE(listItemElementLengths.size() >= 2, "Not enough list items described");
    Y_ENSURE(listBodyHashDataSize == (listHeaderElementLengthsTotal + listItemElementLengthsTotal) * 2, "Invalid body_hash length");

    const size_t listBodyToFirstItem = listHeaderElementLengthsTotal;
    const size_t listBodyToMiddleOfSecondItem = listBodyToFirstItem + listItemElementLengths[0] + (listItemElementLengths[1] + 1) / 2;

    // The first list item must be fully included into the original fact text.
    if (listBodyToCommonPart > listBodyToFirstItem) {
        return 0;
    }

    // The whole first list item and at least leading 50% of the second list item must be included into the original fact text.
    if (listBodyToEndOfCommonPart < listBodyToMiddleOfSecondItem) {
        return 0;
    }

    // Will render the trailing element (sentence) of the list header unconditionally whether it intersects with the original fact text or not, and even if it has empty normalized representation.
    numberOfHeaderElementsToSkip = listHeaderElementLengths.size();
    size_t listBodyToHeaderElement = listHeaderElementLengthsTotal;

    for (size_t i = listHeaderElementLengths.size(); i > 0; --i) {
        const size_t currentHeaderElementIndex = (i - 1);
        const size_t length = listHeaderElementLengths[currentHeaderElementIndex];

        // Ignore those header elements having empty normalized representation, except they are surrounded with nonempty elements already included into the matched header part.
        if (length == 0) {
            continue;
        }

        listBodyToHeaderElement -= length;

        // Header element is matched if its intersection with the original fact text occupies at least 50% of the element.
        const size_t listBodyToMiddleOfHeaderElement = listBodyToHeaderElement + (length + 0) / 2;
        if (listBodyToCommonPart > listBodyToMiddleOfHeaderElement) {
            // Will render the first nonempty trailing element (sentence) of the list header unconditionally even if it does not intersect with the original fact text.
            if (numberOfHeaderElementsToSkip == listHeaderElementLengths.size()) {
                numberOfHeaderElementsToSkip = currentHeaderElementIndex;
            }

            break;
        }

        numberOfHeaderElementsToSkip = currentHeaderElementIndex;
    }

    // Will render the full header unconditionally if all trailing header elements are empty.
    if (numberOfHeaderElementsToSkip == listHeaderElementLengths.size()) {
        numberOfHeaderElementsToSkip = 0;
    }

    return maxCommonSliceSize;
}


TMatchResult MatchFactTextWithListCandidate(const THashVector& factTextHash, const TString& listCandidateIndexJson) {
    const NSc::TValue listCandidateIndex = NSc::TValue::FromJsonThrow(listCandidateIndexJson, JSON_OPTS_STRICT);

    TMatchResult matchResult;
    matchResult.ListCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), matchResult.NumberOfHeaderElementsToSkip);

    return matchResult;
}


void ConvertListDataToRichFact(NSc::TValue& serpData, const NSc::TDict& listData, size_t numberOfHeaderElementsToSkip) {
    Y_ENSURE(serpData.Get("type") != "rich_fact", "Fact is already rich");

    NSc::TValue& visibleItems = serpData.Add("visible_items");

    {
        const NSc::TArray& headerElements = listData.Get("header").GetArray();
        numberOfHeaderElementsToSkip = std::min(numberOfHeaderElementsToSkip, headerElements.size());

        TString joinedHeader;
        for (auto it = headerElements.begin() + numberOfHeaderElementsToSkip; it != headerElements.end(); ++it) {
            joinedHeader.append(it->GetString());
            if (it + 1 != headerElements.end()) {
                joinedHeader.append(" ");
            }
        }

        Y_ENSURE(!joinedHeader.empty(), "Cannot enrich with empty list header");

        NSc::TValue& contentItem = visibleItems.Push()["content"][0];
        contentItem["type"] = "text";
        contentItem["text"] = joinedHeader;
    }

    const NSc::TArray& listItems = listData.Get("items").GetArray();
    const bool isOrderedList = listData.Get("type").GetNumber() != 0;
    for (size_t i = 0; i < listItems.size(); ++i) {
        NSc::TValue& visibleItem = visibleItems.Push();

        NSc::TValue& marker = visibleItem["marker"];
        if (isOrderedList) {
            marker.SetIntNumber(i + 1);
        } else {
            marker.SetString("bullet");
        }

        NSc::TValue& contentItem = visibleItem["content"][0];
        contentItem["type"] = "text";
        contentItem["text"] = listItems[i].GetString();
    }

    Y_ENSURE(serpData.Get("visible_items").GetArray().size() >= (1 + 2), "Cannot enrich with less than two list items");

    serpData.Add("type") = "rich_fact";
    serpData.Add("use_this_type") = 1;
}


TString ConvertListDataToRichFact(const TString& serpDataJson, const TString& listDataJson, size_t numberOfHeaderElementsToSkip) {
    NSc::TValue serpData = NSc::TValue::FromJsonThrow(serpDataJson, JSON_OPTS_STRICT);
    const NSc::TValue listData = NSc::TValue::FromJsonThrow(listDataJson, JSON_OPTS_STRICT);

    ConvertListDataToRichFact(serpData, listData.GetDict(), numberOfHeaderElementsToSkip);

    return serpData.ToJson(JSON_OPTS_STRICT);
}


bool TryReplaceTextWithList(NSc::TValue& serpData, const THashVector& factTextHash, const NSc::TArray& listCandidates, i64& candidateDownCounter) {
    // Avoid facts with too many words in the text. The reason is the speed of FindLongestCommonSlice which is O(N*M), where N - is the number of words in the fact text.
    // The number of words (M) in each listCandidate is restricted during construction of the Database and additionaly insured in MatchFactTextWithListCandidate.
    if (factTextHash.size() > MAX_NUMBER_OF_WORDS_IN_FACT_TEXT_FOR_REPLACEMENT) {
        return false;
    }

    const NSc::TValue* bestListCandidate = nullptr;
    size_t bestListCandidateNumberOfHeaderElementsToSkip = 0;
    size_t bestListCandidateScore = 0;

    for (const NSc::TValue& listCandidate : listCandidates) {
        if (candidateDownCounter <= 0) {
            return false;
        }

        --candidateDownCounter;

        size_t numberOfHeaderElementsToSkip = 0;
        size_t listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidate.Get("index").GetDict(), numberOfHeaderElementsToSkip);
        if (listCandidateScore > bestListCandidateScore) {
            bestListCandidate = &listCandidate;
            bestListCandidateNumberOfHeaderElementsToSkip = numberOfHeaderElementsToSkip;
            bestListCandidateScore = listCandidateScore;
        }
    }

    if (bestListCandidate) {
        ConvertListDataToRichFact(serpData, bestListCandidate->Get("data").GetDict(), bestListCandidateNumberOfHeaderElementsToSkip);
        return true;
    }

    return false;
}


TUtf16String NormalizeTextForIndexer(const TUtf16String& text) {

    // Inspired by https://a.yandex-team.ru/arc/trunk/arcadia/kernel/facts/common/normalize_text.cpp?rev=r8510192#L5-25

    const wchar16 CYRILLIC_SMALL_LETTER_IO = u'\u0451';
    const wchar16 CYRILLIC_SMALL_LETTER_IE = u'\u0435';

    const wchar16 SOFT_HYPHEN = u'\u00AD';
    const wchar16 COMBINING_ACUTE_ACCENT = u'\u0301';

    TUtf16String normalizedText;
    normalizedText.reserve(text.size());

    bool needInsertSpace = false;
    for (wchar16 wc : text) {
        if (IsAlnum(static_cast<wchar32>(wc))) {
            if (needInsertSpace) {
                normalizedText.append(u' ');
                needInsertSpace = false;
            }

            wc = static_cast<wchar16>(ToLower(static_cast<wchar32>(wc)));

            if (wc == CYRILLIC_SMALL_LETTER_IO) {
                wc = CYRILLIC_SMALL_LETTER_IE;
            }

            normalizedText.append(wc);

        } else if (wc == SOFT_HYPHEN || wc == COMBINING_ACUTE_ACCENT) {
            // Ignore these symbols. Even do not modify needInsertSpace flag.

        } else {
            needInsertSpace = !normalizedText.empty();
        }
    }

    return normalizedText;
}


TString NormalizeTextForIndexer(const TString& text) {
    return WideToUTF8(NormalizeTextForIndexer(UTF8ToWide(text)));
}


THashVector DynamicListReplacerHashVector(const TUtf16String& text) {
    const TUtf16String normalizedText = NormalizeTextForIndexer(text);
    const TVector<TWtringBuf> wordVector = StringSplitter(TWtringBuf(normalizedText)).Split(u' ').SkipEmpty();

    THashVector hashVector;
    hashVector.reserve(wordVector.size());
    Transform(
            wordVector.begin(), wordVector.end(),
            std::back_inserter(hashVector),
            [](TWtringBuf word) {
                return static_cast<ui16>(MurmurHash<ui32>(word.Data(), word.Size() * 2) & 0xFFFFu);
            });

    return hashVector;
}


THashVector DynamicListReplacerHashVector(const TString& text) {
    return DynamicListReplacerHashVector(UTF8ToWide(text));
}

}  // namespace NFacts
