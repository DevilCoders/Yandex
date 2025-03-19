#include "filter.h"


namespace NAddressFilter {

void TFilter::OnText(const TUtf16String& sentenceText, const TVector<NToken::TTokenInfo>& tokens,  const TVector<TTokenType>& tokenTypes) {
    TSimpleSharedPtr< TVector<TAddressPosition> > detectorResult = Detector.FilterText(tokenTypes);

    if (FilterOpts.PrintResult)
        Printer->PrintText(sentenceText);

    for (size_t resultIndex = 0; resultIndex < detectorResult->size(); resultIndex++) {
        int tokensToSkip = FilterOpts.GoLeft;
        int start = detectorResult->at(resultIndex).first;
        while((start < tokens.ysize() - 1) && (tokensToSkip > 0)) {
            if (tokens[start + 1].IsNormalToken())
                tokensToSkip--;
            start++;
        }

        tokensToSkip = FilterOpts.GoRight;
        int end = detectorResult->at(resultIndex).second;
        while((end > 0) && (tokensToSkip > 0)) {
            if (tokens[end - 1].IsNormalToken())
                tokensToSkip--;
            end--;
        }

        Result->push_back(std::pair<size_t, size_t>(tokens[start].OriginalOffset,
                                                tokens[end].OriginalOffset + tokens[end].Length));

        if (FilterOpts.PrintResult) {
            Printer->StartPrintTextResult(tokens[start].OriginalOffset,
                                          tokens[end].OriginalOffset + tokens[end].Length,
                                          TWtringBuf(sentenceText,
                                                     tokens[start].OriginalOffset,
                                                     tokens[end].OriginalOffset - tokens[start].OriginalOffset + tokens[end].Length));

            for(int tokenIndex = start; tokenIndex >= (int)end; tokenIndex--) {
                Printer->PrintToken(TWtringBuf(sentenceText, tokens[tokenIndex].OriginalOffset, tokens[tokenIndex].Length));
            }

            Printer->EndPrintTextResult();
        }
    }
}

} //NAddressFilter
