#include <util/generic/buffer.h>
#include <util/system/hp_timer.h>
#include <util/stream/output.h>
#include <util/stream/buffer.h>
#include <util/memory/blob.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/offroad/codec/decoder_16.h>
#include <library/cpp/offroad/codec/decoder_64.h>
#include <library/cpp/offroad/codec/interleaved_decoder.h>

using namespace NOffroad;

struct TOffroadPrintOptions {
    TString FilePath;
    TString ModelPath;
};

void ParseOptions(int argc, const char** argv, TOffroadPrintOptions* options) {
    NLastGetopt::TOpts opts;
    opts.SetTitle("offroad_print - printer for raw offroad streams.");
    opts.SetFreeArgsNum(0);
    opts.AddCharOption('i', "path to offroad-compressed file").RequiredArgument("<path>").Required().StoreResult(&options->FilePath);
    opts.AddCharOption('m', "path to model to use").RequiredArgument("<path>").StoreResult(&options->ModelPath);

    NLastGetopt::TOptsParseResult parseResult(&opts, argc, argv);

    if (options->ModelPath.empty())
        options->ModelPath = options->FilePath + ".model";
}

class IOffroadPrinter {
public:
    virtual ~IOffroadPrinter() {
    }
    virtual void Run(const TOffroadPrintOptions& options) = 0;
    virtual TString TypeName() const = 0;
};

template <class Decoder>
class TOffroadPrinter: public IOffroadPrinter {
public:
    using TDecoder = Decoder;
    using TTable = typename TDecoder::TTable;
    using TModel = typename TTable::TModel;

    enum {
        BlockSize = TDecoder::BlockSize,
        TupleSize = TDecoder::TupleSize,
    };

    void Run(const TOffroadPrintOptions& options) override {
        TModel model;
        model.Load(options.ModelPath);
        TTable table(model);

        TBlob source = TBlob::FromFile(options.FilePath);
        TVecInput input(source);

        TDecoder decoder(&table, &input);

        size_t index = 0, channel = 0;
        std::array<ui32, BlockSize> chunk;
        while (decoder.Read(channel, &chunk)) {
            Cout << "CHUNK #" << index++ << "\n";
            for (ui32 value : chunk)
                Cout << value << "\n";
            Cout << "\n";

            channel = (channel + 1) % TupleSize;
        }
    }

    TString TypeName() const override {
        return TModel::TypeName();
    }
};

class TDynamicOffroadPrinter: public IOffroadPrinter {
public:
    TDynamicOffroadPrinter() {
        Register(new TOffroadPrinter<TDecoder16>());
        Register(new TOffroadPrinter<TDecoder64>());
        Register(new TOffroadPrinter<TInterleavedDecoder<1, TDecoder16>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<2, TDecoder16>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<3, TDecoder16>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<4, TDecoder16>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<5, TDecoder16>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<6, TDecoder16>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<7, TDecoder16>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<1, TDecoder64>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<2, TDecoder64>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<3, TDecoder64>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<4, TDecoder64>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<5, TDecoder64>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<6, TDecoder64>>());
        Register(new TOffroadPrinter<TInterleavedDecoder<7, TDecoder64>>());
    }

    void Run(const TOffroadPrintOptions& options) override {
        TString typeName = ISerializableModel::TypeName(options.ModelPath);

        if (!PrinterByTypeName_.contains(typeName))
            ythrow yexception() << "Unknown model type '" << typeName << "'";

        PrinterByTypeName_[typeName]->Run(options);
    }

    TString TypeName() const override {
        return "*";
    }

private:
    void Register(IOffroadPrinter* printer) {
        PrinterByTypeName_[printer->TypeName()] = THolder<IOffroadPrinter>(printer);
    }

private:
    THashMap<TString, THolder<IOffroadPrinter>> PrinterByTypeName_;
};

int OffroadPrint(int argc, const char** argv) {
    TOffroadPrintOptions options;
    ParseOptions(argc, argv, &options);

    TDynamicOffroadPrinter printer;
    printer.Run(options);

    return 0;
}

int main(int argc, const char** argv) {
    try {
        return OffroadPrint(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
