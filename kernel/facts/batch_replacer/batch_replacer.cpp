#include "batch_replacer.h"

#include <kernel/facts/batch_replacer/batch_replacer_config.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/system/yassert.h>

#include <algorithm>
#include <iterator>
#include <utility>


namespace NFacts {

TBatchReplacer::TBatchReplacer(TStringBuf config) {
    NSc::TValue configFromJson;
    configFromJson["__root__"] = NSc::TValue::FromJsonThrow(config, NSc::TJsonOpts(  // Add fake root structure around the parsed JSON.
            NSc::TJsonOpts::JO_PARSER_STRICT_JSON |
            NSc::TJsonOpts::JO_PARSER_STRICT_UTF8 |
            NSc::TJsonOpts::JO_PARSER_DISALLOW_COMMENTS |
            NSc::TJsonOpts::JO_PARSER_DISALLOW_DUPLICATE_KEYS));

    NBatchReplacer::TBatchReplacerConfigConst<TSchemeTraits> parsedConfig(&configFromJson);
    parsedConfig.Validate(/*path*/ "", /*strict*/ false,
            /*onError*/ [](const TString& path, const TString& message, const NDomSchemeRuntime::TValidateInfo& status) -> void {
                ythrow yexception() << ToString(status.Severity) << ": path '" << path << "' " << message;
            });

    const auto& batchesDictConfig = parsedConfig.Root();  // Remove fake root structure and take the original configuration.

    for (const auto& batchItemConfig : batchesDictConfig) {
        const auto [batchItemIt, isNew] = Batches.emplace(batchItemConfig.Key(), TBatch());
        if (!isNew) {
            ythrow yexception() << "Redefinition of batch '" << batchItemIt->first << "'.";  // Actually this must not happen due to JO_PARSER_DISALLOW_DUPLICATE_KEYS.
        }

        TBatch& batch = batchItemIt->second;
        const auto& batchConfig = batchItemConfig.Value();
        if (!batchConfig.Empty()) {
            batch.reserve(batchConfig.Size());
            for (const auto& operationConfig : batchConfig) {
                try {
                    if (operationConfig.Nop()) {  // Must go the first in the if-else chain to overcome other directives.
                        // no operation
                    } else if (operationConfig.ShrinkSpaces()) {
                        batch.emplace_back(TShrinkSpacesOperation{});
                    } else if (!operationConfig.Pattern()->Empty()) {
                        batch.emplace_back(TReplaceByPatternOperation(operationConfig.Pattern(), operationConfig.Replacement(), operationConfig.Global(), operationConfig.Repeated()));
                    } else if (!operationConfig.Regex()->Empty()) {
                        batch.emplace_back(TReplaceByRegexOperation(operationConfig.Regex(), operationConfig.Replacement(), operationConfig.Global(), operationConfig.Repeated()));
                    }
                } catch (const yexception& error) {
                    ythrow yexception() << error.what() << " in batch '" << batchItemIt->first << "'.";
                }
            }
        }
    }
}

bool TBatchReplacer::Process(std::string* text, const TString& batchName) const {
    Y_ASSERT(text);
    bool isReplacementOccured = false;

    const auto batchItemIt = Batches.find(batchName);
    if (batchItemIt == Batches.end()) {
        ythrow yexception() << "Batch '" << batchName << "' not found.";
    }

    for (const auto& replacementOperation : batchItemIt->second) {
        if (text->empty()) {
            break;
        }
        std::visit(
                [&](auto&& operation){
                    isReplacementOccured |= operation.Process(text);
                },
                replacementOperation);
    }

    return isReplacementOccured;
}

TBatchReplacer::TReplaceOperationBase::TReplaceOperationBase(TAtomicSharedPtr<TProcessorType> processor, bool global, bool repeated)
    : Processor(processor)
    , RepetitionsLimit(!global && repeated ? MAX_REPETITIONS_LIMIT : 1) {
}

bool TBatchReplacer::TReplaceOperationBase::Process(std::string* text) const {
    bool isReplacementOccured = false;
    bool isPatternMatched = true;  // Fake initial true just to unlock the following for-next loop.
    for (size_t i = 0; isPatternMatched && i < RepetitionsLimit; ++i) {
        isPatternMatched = (*Processor)(text);
        isReplacementOccured |= isPatternMatched;
    }
    return isReplacementOccured;
}

TBatchReplacer::TShrinkSpacesOperation::TShrinkSpacesOperation()
    : TReplaceOperationBase(
            MakeAtomicShared<TProcessorType>(
                    [](std::string* text) -> bool {
                        bool prevSpace = true;  // To remove the very first leading space if there is one.
                        auto textNewEnd = std::remove_if(text->begin(), text->end(),
                                [&prevSpace](char c) -> bool {
                                    if (c == ' ') {
                                        if (prevSpace) {
                                            return true;
                                        }
                                        prevSpace = true;
                                    } else {
                                        prevSpace = false;
                                    }
                                    return false;
                                });

                        if (prevSpace && textNewEnd != text->begin()) {  // Means that there is a single trailing space in the remained text. So, cut if off.
                            --textNewEnd;
                        }

                        if (textNewEnd != text->end()) {  // Mean that at least one space symbol was removed from the text.
                            text->erase(textNewEnd, text->end());
                            return /*isReplacementOccured*/ true;
                        }

                        return /*isReplacementOccured*/ false;
                    }),
            true,  // global
            false  // repeated
    ) {
}

TBatchReplacer::TReplaceByPatternOperation::TReplaceByPatternOperation(TStringBuf pattern, TStringBuf replacement, bool global, bool repeated)
    : TReplaceOperationBase(
            [&]() -> TAtomicSharedPtr<TProcessorType> {
                if (replacement.length() <= pattern.length()) {
                    return MakeAtomicShared<TProcessorType>(
                            [Pattern = std::string(pattern), Replacement = std::string(replacement), Global = global]
                            (std::string* text) -> bool {
                                // Inplace version.
                                bool isReplacementOccured = false;
                                auto iIt = text->begin();
                                auto oIt = text->begin();
                                for (;;) {
                                    size_t matchPos = Global || !isReplacementOccured
                                            ? text->find(Pattern, /*offset*/ iIt - text->begin())
                                            : text->npos;  // Force "not found" in the case of (!Global && isReplacementOccured).

                                    auto matchIt = (matchPos != text->npos) ? (text->begin() + matchPos) : text->end();

                                    if (oIt != iIt) {
                                        oIt = std::move(/*from*/ iIt, matchIt, /*to*/ oIt);
                                    } else {
                                        oIt += std::distance(iIt, matchIt);
                                    }

                                    if (matchIt == text->end()) {
                                        break;
                                    }

                                    iIt = matchIt + Pattern.length();
                                    oIt = std::copy(/*from*/ Replacement.begin(), Replacement.end(), /*to*/ oIt);
                                    isReplacementOccured = true;
                                }

                                text->resize(oIt - text->begin());
                                return isReplacementOccured;
                            });
                } else {
                    return MakeAtomicShared<TProcessorType>(
                            [Pattern = std::string(pattern), Replacement = std::string(replacement), Global = global]
                            (std::string* text) -> bool {
                                // Non-inplace version.
                                bool isReplacementOccured = false;
                                auto iIt = text->begin();
                                std::string oText;
                                for (;;) {
                                    size_t matchPos = Global || !isReplacementOccured
                                            ? text->find(Pattern, /*offset*/ iIt - text->begin())
                                            : text->npos;  // Force "not found" in the case of (!Global && isReplacementOccured).

                                    auto matchIt = (matchPos != text->npos) ? (text->begin() + matchPos) : text->end();

                                    // This block is optional for the algorithm and added just to improve performance omitting unnecessary oText (re-)allocations.
                                    if (!isReplacementOccured) {  // Means that this is the first iteration of the endless-for loop.
                                        if (matchIt != text->end()) {
                                            Y_ASSERT(Replacement.length() > Pattern.length());
                                            oText.reserve(text->length() + Replacement.length() - Pattern.length());  // Just the best guess for the most frequent single-match case.
                                        } else {
                                            return isReplacementOccured;  // false
                                        }
                                    }

                                    oText.append(iIt, matchIt);

                                    if (matchIt == text->end()) {
                                        break;
                                    }

                                    iIt = matchIt + Pattern.length();
                                    oText.append(Replacement);
                                    isReplacementOccured = true;
                                }

                                *text = std::move(oText);
                                return isReplacementOccured;
                            });
                }
            }(),
            global,
            repeated
    ) {
}

TBatchReplacer::TReplaceByRegexOperation::TReplaceByRegexOperation(TStringBuf regex, TStringBuf replacement, bool global, bool repeated)
    : TReplaceOperationBase(
            MakeAtomicShared<TProcessorType>(
                    [Scanner = MakeScanner(regex), Replacement = std::string(replacement), Global = global]
                    (std::string* text) -> bool {
                        return Global
                                ? re2::RE2::GlobalReplace(text, *Scanner, Replacement)
                                : re2::RE2::Replace(text, *Scanner, Replacement);
                    }),
            global,
            repeated
    ) {
}

TAtomicSharedPtr<re2::RE2> TBatchReplacer::TReplaceByRegexOperation::MakeScanner(TStringBuf regex) {
    TAtomicSharedPtr<re2::RE2> scanner = MakeAtomicShared<re2::RE2>(re2::StringPiece(regex.data(), regex.size()), re2::RE2::CannedOptions::Quiet);
    if (!scanner->ok()) {
        ythrow yexception() << "Invalid regex r'" << regex << "'";
    }
    return scanner;
}

}  // namespace NFacts
