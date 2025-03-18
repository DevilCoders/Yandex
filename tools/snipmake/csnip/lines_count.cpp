#include "lines_count.h"

namespace NSnippets {

    static const TString LINES_COUNT = "lines_count:";

    TString GetLinesCount(const TString& s) {
        TString res;
        size_t startPos = s.find(LINES_COUNT);
        if (startPos != TString::npos) {
            startPos += LINES_COUNT.size();
            size_t endPos = s.find('\t', startPos + 1);
            if (endPos == TString::npos) {
                endPos = s.size();
            }
            res = TString(s, startPos, endPos - startPos);
        }
        return res;
    }

}
