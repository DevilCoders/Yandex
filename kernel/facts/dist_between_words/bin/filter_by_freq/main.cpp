#include "options.h"

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <ysite/yandex/pure/pure.h>
#include <util/generic/vector.h>
#include <library/cpp/charset/wide.h>

#include <utility>

using namespace NYT;
using namespace NDistBetweenWords;

namespace {
    float CalcLangReverseFreq(const TUtf16String& word, const TPure& pure, ELanguage lang) {
        TPureBase::TPureData pureData = pure.GetByForm(word, NPure::AllCase, lang);
        return pureData.GetFreq();
    }

    float CalcReverseFreq(const TUtf16String& word, const TPure& pure) {
        float rf = CalcLangReverseFreq(word, pure, LANG_RUS);
        rf = Max(rf, CalcLangReverseFreq(word, pure, LANG_ENG));
        rf = Max(rf, CalcLangReverseFreq(word, pure, LANG_UNK));
        return rf;
    }

    const float HARD_THRESHOLD = 100;
}


class TFilterByFreq: public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    TFilterByFreq() = default;

    explicit TFilterByFreq(TString filename) :
            PureFile(std::move(filename))
    {}

    Y_SAVELOAD_JOB(PureFile);

    void Start(TWriter*) override {
        PurePtr.Reset(new TPure(PureFile));
    }

    void Do(TReader* input, TWriter* output) override {
        for (; input->IsValid(); input->Next()) {
            auto& row = input->GetRow();
            auto& word1 = row["word1"].AsString();
            const auto freq1 = word1.length() > 0 ?
                    CalcReverseFreq(CharToWide(word1, CODES_UTF8), *PurePtr) :
                    std::numeric_limits<float>::max();
            if (freq1 < HARD_THRESHOLD) {
                output->AddRow(row, 1);
                continue;
            }
            auto& word2 = row["word2"].AsString();
            const auto freq2 = CalcReverseFreq(CharToWide(word2, CODES_UTF8), *PurePtr);
            if (freq2 < HARD_THRESHOLD) {
                output->AddRow(row, 1);
                continue;
            }
            TNode newRow(row);
            newRow("freq", Min(freq1, freq2));
            output->AddRow(newRow, 0);
        }
    }

private:
    TString PureFile;
    THolder<TPure> PurePtr;
};


REGISTER_MAPPER(TFilterByFreq);



int main(int argc, const char** argv) {
    Initialize(argc, argv);

    TOptions opts(argc, argv);

    IClientPtr client = CreateClient("hahn");

    TMapOperationSpec spec;
    spec.MapperSpec(TUserJobSpec().AddLocalFile(opts.PureFile));
    spec.AddInput<TNode>(opts.InputTable)
            .AddOutput<TNode>(opts.OutputTable)
            .AddOutput<TNode>(opts.FilterByFreqTable);

    client->Map(spec, new TFilterByFreq(opts.PureFile));


//    TPure pure(opts.PureFile);
//    TVector<TString> examples = {
//            "привет",
//            "как",
//            "в",
//            "кайенн",
//            "gipfel",
//            "росэлектроника",
//            "newobrazovanie",
//            "ackac",
//            "киевский",
//            "киевское",
//            "уголовном",
//            "кодексе"
//    };
//
//    for (auto& text : examples) {
//        auto word = CharToWide(text, CODES_UTF8);
//        float rf = CalcReverseFreq(word, pure);
//        Cout << text << " = " << rf << Endl;
//    }
}




