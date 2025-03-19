#include "reqscan.h"

namespace NTokenizerVersionsInfo {
    const size_t Default = 1;
    const size_t DefaultWithOperators = 1;
    const size_t DefaultWithoutOperators = 4;
    const size_t LegacyForQueryNormalizer = 1;
};

TReqTokenizer* TReqTokenizer::GetByVersion(size_t version, const TUtf16String& text, const tRequest& req, TRequestNode::TFactory* nodeFactory, size_t offset, const TLengthLimits* limits) {
    // version 0 (the classic parser) is removed
    // versions 2,3,4 do not pass through this code
    // version 1 is the classic parser + unicode emojis, version 5 adds ascii emojis; both use the same basic class and differ in one option only
    switch (version) {
    case 1:
    default:
        return new TVersionedReqTokenizer<1>(text, req, nodeFactory, offset, limits);
    case 5:
        return new TVersionedReqTokenizer<5>(text, req, nodeFactory, offset, limits);
    }
}
