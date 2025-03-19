#include <cmath>

#include <util/generic/utility.h>

#include "idf.h"

inline double Rf2Idf(double reverseFreq)
{
    const double count = 340985390.0 / reverseFreq;
    static const double MULT = 750000.0;
    // f2p = 1 - exp((-count - 0.1f) / MULT);
    // return -log(f2p); // 2poison p based
    if (count < MULT * 1e-4) {
        const double f2p = (count + 0.1) / MULT;
        return -log(f2p);
    } else {
        const double tmp = exp((-count - 0.1) / MULT);
        if (tmp < 1e-5) {
            return tmp;
        } else {
            return -log(1.0 - tmp);
        }
    }
}

double ReverseFreq2Idf(double reverseFreq)
{
    if (reverseFreq == 0)
        return 0;
    return Rf2Idf(reverseFreq);
}

// min termCount == 1
double TermCount2Idf(i64 allTermsCount, i64 termCount)
{
    if (termCount == 0) {
        return 20.5;
    }
    return Rf2Idf(double(allTermsCount)/termCount);
}

i64 TermCount2RevFreq(i64 allTermsCount, i64 termCount)
{
    if (termCount == 0) { // it's absent in pure-index
        return allTermsCount;
    }
    return Min(allTermsCount, allTermsCount/termCount);
}
