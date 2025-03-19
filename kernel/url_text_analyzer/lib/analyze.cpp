#include "analyze.h"

#include "prepare.h"
#include "token.h"

namespace NUta {
    using namespace NPrivate;

    TVector<TString> AnalyzeUrlUTF8Impl(const THolder<NPrivate::IUrlAnalyzerImpl>& impl, const TOpts& options, const TStringBuf& url) {
        TUtf16String preparedUrl = PrepareUrl(url, options.CutWWW, options.CutZeroDomain,
            options.ProcessWinCode);
        preparedUrl.to_lower();

        TWtringBuf host, pathAndParams;
        TWtringBuf(preparedUrl).Split('/', host, pathAndParams);
        TWtringBuf path, params;
        pathAndParams.Split('?', path, params);

        TVector<TUtf16String> parsedToks;
        THostWordHandler hostHandler(&parsedToks, options.JoinHostSmallTokens);
        Tokenize(host, &hostHandler);
        const size_t idxHost = parsedToks.size();

        TUrlWordHandler urlHandler(&parsedToks, options.IgnoreMarks,
            options.NumTransitionsMarkThreshold, options.IgnoreNumbers);

        static const TUtf16String WIDE_INDEX = u"index";

        TWtringBuf pathLevel;
        while (path.NextTok('/', pathLevel)) {
            if (pathLevel.empty() || pathLevel.size() < options.PathTokenMinLen) {
                continue;
            }
            const size_t prevSize = parsedToks.size();
            Tokenize(pathLevel, &urlHandler);
            const size_t curSize = parsedToks.size();

            const auto tokenType = GetPathTokenType(pathLevel);
            if (tokenType == PTT_PAGE_FILE) {
                parsedToks.pop_back();
                if (curSize == prevSize + 2 && parsedToks.back() == WIDE_INDEX) {
                    parsedToks.pop_back();  // "/index.html" is not informative
                }
            } else if (tokenType == PTT_SCRIPT_FILE) {
                parsedToks.pop_back();
                if (curSize == prevSize + 2
                    && (options.IgnoreScriptNames || parsedToks.back() == WIDE_INDEX))
                {
                    parsedToks.pop_back();  // get rid of "/index.php", or (optionally) all script names
                }
            }
        }

        TWtringBuf paramAndValue;
        while (params.NextTok('&', paramAndValue)) {
            if (paramAndValue.empty()) {
                continue;
            }

            TWtringBuf param, value;
            paramAndValue.Split('=', param, value);
            // usually ignore param name and short values
            if (!options.IgnoreParamName) {
                Tokenize(param, &urlHandler);
            }
            if (value.size() < options.MinParamValueLen) {
                continue;
            }
            Tokenize(value, &urlHandler);
        }

        size_t idxSplitFrom;
        if (options.DoSearchSplit) {
            idxSplitFrom = options.SplitHostWords ? 0 : idxHost;
        } else {
            idxSplitFrom = parsedToks.size();
        }

        TVector<TString> urlWords;
        for (size_t i = 0; i < idxSplitFrom; ++i) {
            TUtf16String& toOutput = parsedToks[i];
            if (options.TryUntranslit) {
                TUtf16String untraslited;
                if (impl->TryUntranslit(toOutput, &untraslited, options.MaxTranslitCandidates)) {
                    std::swap(parsedToks[i], untraslited);
                }
            }
            if (toOutput.size() >= options.MinResultWordLen) {
                urlWords.push_back(WideToUTF8(toOutput));
            }
        }

        for (size_t i = idxSplitFrom; i < parsedToks.size(); ++i) {
            TUtf16String& original = parsedToks[i];
            bool untranslitParts = options.TryUntranslit;
            if (options.TryUntranslit) {
                TUtf16String untraslited;
                if (impl->TryUntranslit(original, &untraslited, options.MaxTranslitCandidates)) {
                    std::swap(parsedToks[i], untraslited);
                    untranslitParts = false;
                }
            }

            TVector<TUtf16String> split;
            if (original.size() >= options.SplitPerformThreshold) {
                split = impl->FindSplit(original,
                    options.PenalizeForShortSplits, untranslitParts, options.MinSubTokenLen,
                    options.MaxTranslitCandidates);
                for (const auto& word : split) {
                    if (word.size() >= options.MinResultWordLen) {
                        urlWords.push_back(WideToUTF8(word));
                    }
                }
            }

            if (((split.empty() && options.IgnoreEmptySplit) ||
                    original.size() < options.SplitPerformThreshold) &&
                    original.size() >= options.MinResultWordLen) {
                urlWords.push_back(WideToUTF8(original));
            }
        }
        return urlWords;
    }

}
