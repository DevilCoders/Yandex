#pragma once

#include "regex_glue.h"

#include <library/cpp/regex/pire/pire.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/subst.h>

#include <iterator>
#include <utility>


namespace NAntiRobot {


struct TRegexMatcherEntry {
    TString Expr;
    bool CaseSensitive = true;

    TRegexMatcherEntry() = default;

    TRegexMatcherEntry(TString expr, bool caseSensitive)
        : Expr(std::move(expr))
        , CaseSensitive(caseSensitive)
    {}

    static TMaybe<TRegexMatcherEntry> Parse(TStringBuf s) {
        TRegexMatcherEntry ret;

        if (s.size() < 2) {
            return Nothing();
        }

        size_t lastSlashIdx = s.size() - 1;

        if (s[lastSlashIdx] == 'i') {
            ret.CaseSensitive = false;
            --lastSlashIdx;
        }

        if (s[0] != '/' || s[lastSlashIdx] != '/') {
            return Nothing();
        }

        ret.Expr = TString(s.SubStr(1, lastSlashIdx - 1));
        SubstGlobal(ret.Expr, R"(\/)", "/");

        return ret;
    }

    template <typename TScanner>
    TScanner ToScanner() const {
        NPire::TLexer lexer(Expr);
        SetUpLexer(&lexer);

        return lexer.Parse().Compile<TScanner>();
    }

    void SetUpLexer(NPire::TLexer* lexer) const {
        if (!CaseSensitive) {
            lexer->AddFeature(NPire::NFeatures::CaseInsensitive());
        }
    }

    auto operator<=>(const TRegexMatcherEntry&) const = default;
};


class TRegexMatcher {
public:
    TRegexMatcher() = default;

    explicit TRegexMatcher(const TRegexMatcherEntry& entry) {
        this->AddScanner(entry);
    }

    template <typename TIter /* over TRegexMatcherEntry */>
    explicit TRegexMatcher(
        TIter begin, TIter end,
        size_t maxScannerSize = DEFAULT_MAX_SCANNER_SIZE
    ) {
        Scanners.reserve(std::distance(begin, end));

        for (auto it = begin; it != end; ++it) {
            Scanners.push_back(it->template ToScanner<NPire::TNonrelocScanner>());
        }

        GlueScanners(
            &Scanners,
            [] (auto& scanner) -> NPire::TNonrelocScanner& {
                return scanner;
            },
            {},
            maxScannerSize
        );
    }

    template <typename TRange>
    explicit TRegexMatcher(
        const TRange& range,
        size_t maxScannerSize = DEFAULT_MAX_SCANNER_SIZE
    ) : TRegexMatcher(
        range.begin(), range.end(),
        maxScannerSize
    ) {}

    void AddScanner(const TRegexMatcherEntry& entry) {
        Scanners.push_back(entry.ToScanner<NPire::TNonrelocScanner>());
    }

    void Prepare(size_t maxScannerSize = DEFAULT_MAX_SCANNER_SIZE) {
        GlueScanners(
            &Scanners,
            [] (auto& scanner) -> NPire::TNonrelocScanner& {
                return scanner;
            },
            {},
            maxScannerSize
        );
    }

    bool Empty() const {
        return Scanners.empty();
    }

    size_t NumScanners() const {
        return Scanners.size();
    }

    bool Matches(TStringBuf s) const {
        for (const auto& scanner : Scanners) {
            if (scanner.Final(NPire::Runner(scanner).Run(s).State())) {
                return true;
            }
        }

        return false;
    }

private:
    TVector<NPire::TNonrelocScanner> Scanners;
};


class TSingleRegexMatcher {
public:
    TSingleRegexMatcher() = default;

    explicit TSingleRegexMatcher(const TRegexMatcherEntry& entry)
        : Scanner(entry.ToScanner<NPire::TNonrelocScanner>())
    {}

    bool Matches(TStringBuf s) const {
        return Scanner.Final(NPire::Runner(Scanner).Run(s).State());
    }

private:
    NPire::TNonrelocScanner Scanner;
};


class TRegexConjunction {
public:
    TRegexConjunction() = default;

    explicit TRegexConjunction(const TRegexMatcherEntry& entry) {
        this->AddScanner(entry);
    }

    void AddScanner(const TRegexMatcherEntry& entry) {
        Scanners.push_back(entry.ToScanner<NPire::TNonrelocScanner>());
    }

    void Prepare(size_t maxScannerSize = DEFAULT_MAX_SCANNER_SIZE) {
        GlueScanners(
            &Scanners,
            [] (auto& scanner) -> NPire::TNonrelocScanner& {
                return scanner;
            },
            {},
            maxScannerSize
        );
    }

    size_t NumScanners() const {
        return Scanners.size();
    }

    bool Matches(TStringBuf s) const {
        for (const auto& scanner : Scanners) {
            auto [begin, end] = scanner.AcceptedRegexps(Pire::Runner(scanner).Run(s).State());
            if (static_cast<size_t>(end - begin) != scanner.RegexpsCount()) {
                return false;
            }
        }

        return true;
    }

private:
    TVector<NPire::TNonrelocScanner> Scanners;
};


} // namespace NAntiRobot
