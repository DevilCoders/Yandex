#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/mime/types/mime.h>

#include <ysite/parser/convert/face/load_conv.h>
#include <kernel/recshell/recshell.h>

#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/stream/mem.h>
#include <util/system/defaults.h>
#include <util/generic/yexception.h>

struct TConfig {
    TString MimeTypeString;
    TString DictFile;
    TString Input;

    TConfig()
        : MimeTypeString("")
        , DictFile("dict.dict")
        , Input("/dev/stdin")
    { }

    void Parse(int argc, const char** argv) {
        NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

        opts.AddHelpOption();
        opts.AddVersionOption();
        opts.AddLongOption("mime").Required().RequiredArgument("mime type of the content").
                StoreResult(&MimeTypeString);
        opts.AddLongOption("dict").Optional().RequiredArgument("path to the dict.dict").
                StoreResult(&DictFile).DefaultValue(DictFile);
        opts.AddLongOption("input").Optional().RequiredArgument("path to the file to convert").
                StoreResult(&Input).DefaultValue(Input);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }
};

void main_safe(int argc, const char** argv) {
    TConfig config;
    config.Parse(argc, argv);

    MimeTypes mime = mimeByStr(config.MimeTypeString.data());
    if (mime == MIME_UNKNOWN) {
        ythrow yexception() << "unknown mime string: " << config.MimeTypeString;
    }

    TRecognizerShell recognizer(config.DictFile);

    TAllConverters converters;
    converters.SetAll();
    converters.SetRecognizer(&recognizer);

    IConverter* converter = converters.Get(mime);
    if (converter == nullptr) {
        ythrow yexception() << "no converter for mime: " << config.MimeTypeString;
    }
    TString document;
    {
        TFileInput input(config.Input);
        document = input.ReadAll();
    }
    if (document.size() == 0) {
        ythrow yexception() << "empty document";
    }
    TMemoryInput memoryInput(document.data(), document.size());
    converter->SetInput(&memoryInput);

    ui64 convertedBytes = TransferData(converter->GetOutput(), &Cout);
    if (convertedBytes == 0) {
        ythrow yexception() << "empty doc after conversion";
    }

    Cerr << "Conversion metadata:" << Endl;
    const TMetaData* metadata = converter->GetMetaData();
    for (const auto& p : *metadata) {
        Cerr << p.first << ": " << p.second << Endl;
    }
}

int main(int argc, const char** argv) {
    try {
        main_safe(argc, argv);
        return 0;
    } catch (...) {
        Cerr << "ERROR: " << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
