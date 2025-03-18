#include "kmp_skip_search.h"

#include <util/generic/yexception.h>

#include <cstring>

#define CST (size_t)(unsigned char)

namespace NAntiRobot {
    namespace {
        void preMp(const char *x, size_t m, int mpNext[]) {
            size_t i = 0;
            int j = mpNext[0] = -1;
            while (i < m) {
                while (j > -1 && x[i] != x[j])
                    j = mpNext[j];
                mpNext[++i] = ++j;
            }
        }

        void preKmp(const char *x, size_t m, int kmpNext[]) {
            size_t i = 0;
            int j = kmpNext[0] = -1;
            while (i < m) {
                while (j > -1 && x[i] != x[j])
                    j = kmpNext[j];
                i++;
                j++;
                if (x[i] == x[j])
                    kmpNext[i] = kmpNext[j];
                else
                    kmpNext[i] = j;
            }
        }

        int attempt(const char *y, const char *x, size_t m, int start, int wall) {
            int k;

            k = wall - start;
            while (k < (int)m && x[k] == y[k + start])
                ++k;
            return(k);
        }
    }

    TKmpSkipSearch::TKmpSkipSearch(const char* searchString)
        : SearchStringLen(strlen(searchString))
    {
        if (SearchStringLen >= MaxSearchStringSize)
            ythrow yexception() << "TKmpSkipSearch: search string '" << searchString << "' too large";

        strcpy(SearchString, searchString);

        // Preprocessing
        preMp(SearchString, SearchStringLen, MpNext);
        preKmp(SearchString, SearchStringLen, KmpNext);

        memset(Z, -1, sizeof(Z));
        memset(List, -1, SearchStringLen*sizeof(int));

        Z[CST(SearchString[0])] = 0;
        for (int i = 1; i < (int)SearchStringLen; ++i) {
            List[i] = Z[CST(SearchString[i])];
            Z[CST(SearchString[i])] = i;
        }
    }

    TKmpSkipSearch::~TKmpSkipSearch() {
    }

    const char* TKmpSkipSearch::SearchInText(const char* text, size_t textLen) const {
        if (!text)
            return nullptr;

        int i = -1;
        int j = -1;
        int k;
        int kmpStart;
        int start;

        // Searching
        int wall = 0;

        do {
            j += (int)SearchStringLen;
        } while (j < (int)textLen && Z[CST(text[j])] < 0);
        if (j >= (int)textLen)
            return nullptr;
        i = Z[CST(text[j])];
        start = j - i;

        while (start <= (int)textLen - (int)SearchStringLen) {
            if (start > wall)
                wall = start;
            k = attempt(text, SearchString, SearchStringLen, start, wall);
            wall = start + k;
            if (k == (int)SearchStringLen) {
                return text +start;
//                i -= per;
            } else
                i = List[i];
            if (i < 0) {
                do {
                    j += (int)SearchStringLen;
                } while (j < (int)textLen && Z[CST(text[j])] < 0);
                if (j >= (int)textLen)
                    return nullptr;
                i = Z[CST(text[j])];
            }
            kmpStart = start + k - KmpNext[k];
            k = KmpNext[k];
            start = j - i;
            while (start < kmpStart ||
                (kmpStart < start && start < wall)) {
                    if (start < kmpStart) {
                        i = List[i];
                        if (i < 0) {
                            do {
                                j += (int)SearchStringLen;
                            } while (j < (int)textLen && Z[CST(text[j])] < 0);
                            if (j >= (int)textLen)
                                return nullptr;
                            i = Z[CST(text[j])];
                        }
                        start = j - i;
                    } else {
                        kmpStart += (k - MpNext[k]);
                        k = MpNext[k];
                    }
            }
        }
        return nullptr;
    }
}
