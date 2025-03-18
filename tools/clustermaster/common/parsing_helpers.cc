#include <util/string/printf.h>
#include <util/string/split.h>

#include <cstdlib>

bool ParseRange(const TString &first, const TString &last, TVector<TString> &output) {
    if (first.length() != last.length())
        return false;

    size_t len = first.length();
    size_t numstart, numend;

    // find first different char
    for (numstart = 0; numstart < len && first[numstart] == last[numstart]; numstart++) {
        // empty
    }

    // strings equal -> not really a range
    if (numstart >= len) {
        output.push_back(first);
        return true;
    }

    // difference not in numeric characters -> incorrect range
    if (!isdigit(first[numstart]) || !isdigit(last[numstart]))
        return false;

    // find the end of numbers
    for (numend = numstart; numend < len && isdigit(first[numend]) && isdigit(last[numend]); numend++) {
        // empty
    }

    int firstn = atoi(first.substr(numstart, numend-numstart).data());
    int lastn = atoi(last.substr(numstart, numend-numstart).data());

    if (firstn > lastn)
        return false;

    // Sanity check - other parts must be equal
    if (first.substr(numend, TString::npos) != last.substr(numend, TString::npos))
        return false;

    for (int i = firstn; i <= lastn; i++)
        output.push_back(Sprintf("%s%.*d%s", first.substr(0, numstart).data(), (int)(numend-numstart), i, first.substr(numend, TString::npos).data()));

    return true;
}
