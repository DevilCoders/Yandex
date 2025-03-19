#pragma once

#include <library/cpp/regex/pire/pire.h>

#include <util/generic/string.h>


namespace NHtmlXPath {

class TUrlScanner {
public:
    typedef NPire::TScanner::State TScanResult;
    typedef NPire::TScanner TScanner;

public:
    bool IsEmpty() const {
        return RegexpNumbers.empty();
    }

    size_t GetRegexpsNumber() const {
        return RegexpNumbers.size();
    }

    TScanResult Scan(const TStringBuf& str) const {
        TScanResult finalState =
            NPire::Runner(Scanner)
            .Run(str.begin(), str.end())
            .State();

        return finalState;
    }

    bool HasMatches(const TScanResult& finalState) const {
        std::pair<const size_t*, const size_t*> matches = Scanner.AcceptedRegexps(finalState);

        return matches.first != matches.second;
    }

    size_t GetMaxMatch(const TScanResult& finalState) const {
        std::pair<const size_t*, const size_t*> matches = Scanner.AcceptedRegexps(finalState);

        const size_t* begin = matches.first;
        const size_t* end = matches.second;

        if (begin == end) {
            throw yexception() << "This state has no matches";
        }

        size_t result = RegexpNumbers[*begin];
        for (const size_t* match = begin; match < end; ++match) {
            result = Max(result, RegexpNumbers[*match]);
        }

        return result;
    }


    void AddScanner(const NPire::TScanner& scanner, size_t number) {
        if (RegexpNumbers.empty()) {
            Scanner = scanner;
        } else {
            Scanner = NPire::TScanner::Glue(Scanner, scanner);
        }
        RegexpNumbers.push_back(number);
    }

    void Clear() {
        Scanner = NPire::TScanner();
        RegexpNumbers.clear();
    }

    static TScanner CreateScanner(const TString& regexp) {
        return NPire::TLexer(regexp).Parse().Compile<NPire::TScanner>();
    }

private:
    NPire::TScanner Scanner;
    TVector<size_t> RegexpNumbers;
};

}
