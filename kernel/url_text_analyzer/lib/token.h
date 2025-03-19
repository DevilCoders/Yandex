#pragma once

#include <library/cpp/tokenizer/tokenizer.h>
#include <util/generic/set.h>
#include <util/string/split.h>
#include <util/charset/utf8.h>

namespace NUta::NPrivate {
    class IUrlTokenHandler : public ITokenHandler {
    protected:
        bool Error;
        TVector<TUtf16String>* const Tokens;

    public:
        IUrlTokenHandler(TVector<TUtf16String>* tokens)
            : Error(false)
            , Tokens(tokens)
        {
        }
    };

    static const TUtf16String WIDE_DASH = u"-";
    static const TUtf16String WIDE_FLOAT_POINT = u"."; // depends on language, but "." by default

    class THostWordHandler : public IUrlTokenHandler {
    private:
        bool JoinSmallTokens;

    public:
        explicit THostWordHandler(TVector<TUtf16String>* tokens, bool joinSmallTokens)
            : IUrlTokenHandler(tokens)
            , JoinSmallTokens(joinSmallTokens)
        {
        }

        void OnToken(const TWideToken& token, size_t /* origLeng */, NLP_TYPE type) override {
            if (Error) {
                return;
            }

            if (type != NLP_MISCTEXT) {
                TWtringBuf word(token.Token, token.Leng);
                TVector<TWtringBuf> parts;
                StringSplitter(word.data(), word.data() + word.size()).SplitByString(WIDE_DASH.data()).AddTo(&parts);
                for (size_t i = 0; i < parts.size(); ++i) {
                    if (JoinSmallTokens && parts[i].size() < 3 && !Tokens->empty()) {
                        // FIXME: here is a possible mistake for "some.host.w-t-f" -> {"some", "hostwtf"}
                        // need '&& i > 0'
                        Tokens->back().append(parts[i]);
                    } else {
                        Tokens->push_back(ToWtring(parts[i]));
                    }
                }
            }
        }
    };

    class TUrlWordHandler : public IUrlTokenHandler {
    private:
        bool IgnoreMarks;
        ui32 TransitionsThreshold;
        bool IgnoreNumbers;

    public:
        explicit TUrlWordHandler(TVector<TUtf16String>* tokens,
                bool ignoreMarks,
                ui32 transitionsThreshold,
                bool ignoreNumbers)
            : IUrlTokenHandler(tokens)
            , IgnoreMarks(ignoreMarks)
            , TransitionsThreshold(transitionsThreshold)
            , IgnoreNumbers(ignoreNumbers)
        {
        }

        void OnToken(const TWideToken& token, size_t /* origLeng */, NLP_TYPE type) override {
            if (Error) {
                return;
            }

            TWtringBuf word(token.Token, token.Leng);
            switch (type) {
                case NLP_WORD: {
                    ProcessWordToken(word);
                    break;
                }
                case NLP_INTEGER: {
                    if (!IgnoreNumbers) {
                        ProcessIntegerToken(word);
                    }
                    break;
                }
                case NLP_FLOAT: {
                    if (!IgnoreNumbers) {
                        ProcessFloatToken(word);
                    }
                    break;
                }
                case NLP_MARK: {
                    if (!IgnoreMarks) {
                        ProcessMarkToken(word);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }

    private:
        void ProcessWordToken(TWtringBuf word) {
            TVector<TWtringBuf> parts;
            StringSplitter(word.data(), word.data() + word.size()).SplitByString(WIDE_DASH.data()).AddTo(&parts);
            for (const auto& p : parts) {
                Tokens->push_back(ToWtring(p));
            }
        }

        void ProcessIntegerToken(TWtringBuf word) {
            Tokens->push_back(ToWtring(word));
        }

        void ProcessFloatToken(TWtringBuf word) {
            TVector<TWtringBuf> parts;
            StringSplitter(word.cbegin(), word.cend()).SplitByString(WIDE_FLOAT_POINT.data()).AddTo(&parts);
            for (const auto& p : parts) {
                Tokens->push_back(ToWtring(p));
            }
        }

        void ProcessMarkToken(TWtringBuf word) {
            const TString wordUtf8 = WideToUTF8(word);
            bool isPrevNumber = (wordUtf8[0] >= '0' && wordUtf8[0] <= '9');
            size_t transitions = 0;
            for (const char l : wordUtf8) {
                const bool isCurNumber = (l >= '0' && l <= '9');
                if (isPrevNumber != isCurNumber) {
                    ++transitions;
                }
                if (transitions > TransitionsThreshold) {
                    return;
                }
                isPrevNumber = isCurNumber;
            }
            Tokens->push_back(ToWtring(word));
        }
    };

    enum EPathTokenType {
        PTT_NO_EXT = 0,
        PTT_PAGE_FILE = 1,
        PTT_SCRIPT_FILE = 2,
        PTT_UNKNOWN_EXT = 3,
    };

    inline EPathTokenType GetPathTokenType(const TWtringBuf& token) {
        TWtringBuf suffix = token.RAfter('.');
        if (suffix == token) {
            return PTT_NO_EXT;
        }

        static const TSet<TUtf16String> pageExtensions{u"html", u"shtml",
            u"xml", u"htm", u"mhtml", u"jpg",
            u"png", u"sea", u"xhtml", u"xhtm"};
        static const TSet<TUtf16String> scriptExtensions{u"php", u"asp", u"aspx"};

        if (pageExtensions.contains(suffix)) {
            return PTT_PAGE_FILE;
        } else if (scriptExtensions.contains(suffix)) {
            return PTT_SCRIPT_FILE;
        } else {
            return PTT_UNKNOWN_EXT;
        }
    }

    void Tokenize(const TWtringBuf& src, IUrlTokenHandler* handler);
}