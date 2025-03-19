#pragma once

#include <contrib/libs/re2/re2/re2.h>

#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>

#include <functional>
#include <string>


namespace NFacts {

class TBatchReplacer : public TNonCopyable {
public:
    explicit TBatchReplacer(TStringBuf config);
    bool Process(std::string* text, const TString& batchName) const;

private:
    class TReplaceOperationBase {
    public:
        using TProcessorType = std::function< bool /*isReplacementOccured*/ (std::string* /*text*/) >;

        TReplaceOperationBase(TAtomicSharedPtr<TProcessorType> processor, bool global, bool repeated);
        bool /*isReplacementOccured*/ Process(std::string* text) const;

    private:
        static constexpr size_t MAX_REPETITIONS_LIMIT = 100;

        TAtomicSharedPtr<TProcessorType> Processor;
        size_t RepetitionsLimit;
    };

    class TShrinkSpacesOperation : public TReplaceOperationBase {
    public:
        TShrinkSpacesOperation();
    };

    class TReplaceByPatternOperation : public TReplaceOperationBase {
    public:
        TReplaceByPatternOperation(TStringBuf pattern, TStringBuf replacement, bool global, bool repeated);
    };

    class TReplaceByRegexOperation : public TReplaceOperationBase {
    public:
        TReplaceByRegexOperation(TStringBuf regex, TStringBuf replacement, bool global, bool repeated);

    private:
        static TAtomicSharedPtr<re2::RE2> MakeScanner(TStringBuf regex);
    };

    using TBatch = TVector<std::variant<
        TShrinkSpacesOperation,
        TReplaceByPatternOperation,
        TReplaceByRegexOperation
    >>;

    THashMap<TString /*batchName*/, TBatch> Batches;
};

}  // namespace NFacts
