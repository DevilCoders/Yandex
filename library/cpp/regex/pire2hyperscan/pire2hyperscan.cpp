#include "pire2hyperscan.h"

#include <library/cpp/regex/hyperscan/hyperscan.h>
#include <library/cpp/regex/pire/pire.h>
#include "re_parser.h" // FIXME file from Pire, needed for YRE_AND, YRE_NOT

#include <util/charset/wide.h>

struct TCountedTerm {
    Pire::Term MainTerm;
    int MinCount;
    int MaxCount;

    TCountedTerm(const Pire::Term term)
        : MainTerm(term)
        , MinCount(1)
        , MaxCount(1)
    {
    }
};

using TCharacterRange = Pire::Term::CharacterRange;

static TString ToUtf8(wchar32 letter32) {
    TUtf16String wstroka = UTF32ToWide(&letter32, 1);
    return WideToUTF8(wstroka);
}

static bool NeedBrackets(const TCharacterRange& range) {
    if (range.second) {
        return true;
    }
    if (range.first.size() != 1) {
        return true;
    }
    auto wideLetter = *range.first.begin();
    if (wideLetter.size() != 1) {
        return true; // will throw NHyperscan::TCompileException
    }
    TString letter = ToUtf8(wideLetter[0]);
    if (letter.size() != 1) {
        return true;
    }
    return !isalnum(letter[0]);
}

static bool NeedEscape(const TString& ch) {
    return ch == "-" || ch == "[" || ch == "]" || ch == "\\" || ch == "^";
}

TString PireLexer2Hyperscan(NPire::TLexer& lexer) {
    // Step 1. Turn lexer into a vector of terms
    TVector<TCountedTerm> terms;
    for (Pire::Term term = lexer.Lex(); term.Type() != 0; term = lexer.Lex()) {
        if (term.Type() == YRE_COUNT) {
            using TRepetitionCount = Pire::Term::RepetitionCount;
            const TRepetitionCount& value = term.Value().As<TRepetitionCount>();
            Y_ASSERT(!terms.empty());
            terms.back().MinCount = value.first;
            terms.back().MaxCount = value.second;
        } else {
            terms.push_back(term);
        }
    }

    // Step 2. Turn the vector of terms back to regex string.
    TStringStream result;
    for (size_t i = 0; i < terms.size(); i++) {
        const TCountedTerm term = terms[i];

        // If first term is [^...], it matches text begin in Pire
        // Example: /[^4]submit/
        // The following conditions are required to match text begin/end:
        // 1. the term is first or last
        // 2. length can be 1, so it could be begin/end mark in Pire
        bool mayNeedMask = (term.MinCount == 1);
        // terms.size() > 1; https://github.com/01org/hyperscan/issues/25
        auto fixBefore = [&]() {
            if (mayNeedMask) {
                if (i == 0) {
                    result << "(^|";
                } else if (i == terms.size() - 1) {
                    result << "($|";
                }
            }
        };
        auto fixAfter = [&]() {
            if (mayNeedMask) {
                if (i == 0) {
                    result << ")";
                } else if (i == terms.size() - 1) {
                    result << ")";
                }
            }
        };

        auto printCount = [&]() {
            if (term.MinCount != 1 || term.MaxCount != 1) {
                result << '{' << term.MinCount << ',';
                if (term.MaxCount != Pire::Consts::Inf) {
                    result << term.MaxCount;
                }
                result << '}';
            }
        };

        int type = term.MainTerm.Type();
        if (type == YRE_LETTERS) {
            if (!term.MainTerm.Value().IsA<TCharacterRange>()) {
                ythrow NHyperscan::TCompileException();
            }
            const TCharacterRange& value = term.MainTerm.Value().As<TCharacterRange>();
            if (value.second) {
                fixBefore();
            }
            if (NeedBrackets(value)) {
                result << '[';
            }
            if (value.second) {
                result << '^';
            }
            for (const auto& str : value.first) {
                if (str.size() != 1) {
                    // members of [...] must be 1-letter
                    ythrow NHyperscan::TCompileException();
                }
                const auto stroka = lexer.Encoding().ToLocal(str[0]);
                if (NeedEscape(stroka)) {
                    result << '\\';
                }
                result << stroka;
            }
            if (NeedBrackets(value)) {
                result << ']';
            }
            printCount();
            if (value.second) {
                fixAfter();
            }
        } else if (type == YRE_DOT) {
            fixBefore();
            result << '.';
            printCount();
            fixAfter();
        } else if (type == YRE_AND) {
            ythrow NHyperscan::TCompileException();
        } else if (type == YRE_NOT) {
            ythrow NHyperscan::TCompileException();
        } else if (type == '(') {
            result << '(';
        } else if (type == ')') {
            result << ')';
            printCount();
        } else if (type == '|') {
            result << '|';
        } else if (type == '^') {
            result << '^';
        } else if (type == '$') {
            result << '$';
        } else {
            ythrow yexception() << "Unknown term type: " << type;
        }
    }
    return result.Str();
}

TString PireRegex2Hyperscan(const TStringBuf& regex) {
    std::vector<wchar32> ucs4;
    NPire::NEncodings::Utf8().FromLocal(
        regex.begin(),
        regex.end(),
        std::back_inserter(ucs4));
    NPire::TLexer lexer(ucs4.begin(), ucs4.end());
    lexer.SetEncoding(NPire::NEncodings::Utf8());
    lexer.AddFeature(NPire::NFeatures::AndNotSupport());
    return PireLexer2Hyperscan(lexer);
}
