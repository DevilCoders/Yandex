#include <kernel/facts/heads_in_facts/heads_in_facts.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <kernel/searchlog/errorlog.h>

namespace NFacts {

NSc::TValue FindAuthorByComment(const NSc::TArray &authors,
                                const TStringBuf &factText,
                                const size_t maxCommentLength,
                                const size_t minAllowedCommonSliceSize)
{
    Y_ENSURE(!factText.empty(), "Cannot process with empty fact");

    const NFacts::THashVector factTextHash = NFacts::DynamicListReplacerHashVector(TString(factText));

    NSc::TValue bestAuthor = NSc::TValue::Null();
    size_t maxCommonSliceSize = 0;

    for (const NSc::TValue &author : authors) {
        const TStringBuf commentText = author.Get("comment").GetString();
        Y_ENSURE(!commentText.empty(), "Cannot process with empty comment");

        size_t aToCommon;
        size_t bToCommon;

        const NFacts::THashVector commentTextHash = NFacts::DynamicListReplacerHashVector(TString(commentText));

        const size_t minCommonSliceSize = (commentText.size() < maxCommentLength)
                ? (factTextHash.size() + 1) / 2
                : (commentTextHash.size() + 1) / 3;

        const size_t commonSliceSize = NFacts::FindLongestCommonSlice(factTextHash, commentTextHash, aToCommon, bToCommon,
                                                                      std::max(minCommonSliceSize, minAllowedCommonSliceSize));

        if (commonSliceSize > maxCommonSliceSize) {
            maxCommonSliceSize = commonSliceSize;
            bestAuthor = author;
        }
    }

    return bestAuthor;
}

}  // namespace NFacts
