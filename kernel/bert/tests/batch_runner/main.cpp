#include "consts.h"

#include <kernel/bert/tests/batch_runner/protos/config.pb.h>

#include <kernel/bert/batch_processor.h>
#include <kernel/bert/tokenizer.h>

#include <dict/mt/libs/nn/ynmt_backend/cpu/backend.h>

#include <library/cpp/getoptpb/getoptpb.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/buffered.h>
#include <util/stream/file.h>

using namespace NBertApplier;
using namespace NBatchRunner;


class TProcessor {
    private:
        TBertIdConverter Converter;
        TBertSegmenter Segmenter;
        TIntrusivePtr<ITimeProvider> TimeProvider;
        TBertModel<float, NDict::NMT::NYNMT::TCopyHead> Model;
        TBatchProcessor<float, NDict::NMT::NYNMT::TCopyHead> Processor;

    public:
        TProcessor(TConfig const& config)
            : Converter(config.GetVocabulary())
            , Segmenter(config.GetStartTrie(), config.GetContinuationTrie())
            , TimeProvider(CreateDefaultTimeProvider())
            , Model(config.GetModel()
                    , config.GetMaxBatchSize()
                    , config.GetMaxInputLength() >= 3 ? config.GetMaxInputLength() : 3
                    , MakeIntrusive<NDict::NMT::NYNMT::TCpuBackend>())
            , Processor(Model, *TimeProvider)
        {
        }

        NThreading::TFuture<decltype(Processor)::TModelResult> Process(TStringBuf input) {
            auto const tokens = Tokenize(input);
            auto tokenIds = Converter.Convert(tokens);
            return Processor.Process({tokenIds.data(), tokenIds.size()});
        }

    private:
        TVector<TUtf32String> Tokenize(TStringBuf text) {
            // split input request into tokens
            auto const utf32Text = TUtf32String::FromUtf8(text);
            auto const maxTokens = Model.GetMaxInputLength() - 2;
            auto tokens = Segmenter.Split(utf32Text, maxTokens);
            // add start and stop tokens
            AddServiceTokens(tokens);
            Y_VERIFY(tokens.size() <= Model.GetMaxInputLength());
            return tokens;
        }

        void AddServiceTokens(TVector<TUtf32String>& tokens) {
            tokens.reserve(tokens.size() + 2);
            tokens.insert(tokens.begin(), TUtf32String::FromUtf8("[CLS]"));
            tokens.push_back(TUtf32String::FromUtf8("[SEP]"));
        }
};

struct TRequest {
    TString Text;
    NThreading::TFuture<TBatchProcessor<float, NDict::NMT::NYNMT::TCopyHead>::TModelResult> Future;
};

int main(int argc, char const* argv[]) {
    TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv);

    TFileInput in(config.GetInput());
    TProcessor processor(config);

    TFileOutput out(config.GetOutput());
    TVector<TRequest> requests;
    for (TString text; in.ReadLine(text) > 0; ) {
        auto f = processor.Process(text);
        requests.push_back({std::move(text), f});
    }
    for (auto const& r : requests) {
        out << r.Text << Endl;
        for (auto f : r.Future.GetValueSync()) {
            out << f << Endl;
        }
        out << ResultsDelimiter << Endl;
    }
    return 0;
}
