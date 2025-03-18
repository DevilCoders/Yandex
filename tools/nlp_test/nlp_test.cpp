#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/vector.h>

#include "options.h"
#include "numerator.h"

void TestFile(const TOptions& options, THtProcessor& htProcessor, const TString& path) {
    static const TString suffix(".nlp_test_output");

    TMappedFileInput input(path.c_str());
    THolder<IParsedDocProperties> docProps(htProcessor.ParseHtml(&input, "http://local.base/"));

    if (!options->encoding.empty() && docProps->SetProperty(PP_CHARSET, options->encoding.c_str()) != 0)
        ythrow yexception() << "can't set parser codepage to '" <<  options->encoding.c_str() << "'";

    THolder<TFixedBufferFileOutput> file;
    IOutputStream* output = &Cout;
    TString outPath = options->packet ? path + suffix : options->output;
    if (!outPath.empty()) {
        file.Reset(new TFixedBufferFileOutput(outPath));
        output = file.Get();
    }

    DoTest(&htProcessor, docProps.Get(), *output);
}

int main(int argc, char* const argv[]) {
    TOptions options(argc, argv);

    TVector<TString> files;

    if (options->packet) {
        THolder<TFileInput> file;
        IInputStream* input = &Cin;
        if (!options->input.empty()) {
            file.Reset(new TFileInput(options->input));
            input = file.Get();
        }

        TString line;
        while (input->ReadLine(line)) {
            if (!line.empty()) {
                files.push_back(line);
            }
        }
    } else {
        files.push_back(options->input);
    }

    THtProcessor htProcessor;
    if (!!options->parserConfig)
        htProcessor.Configure(options->parserConfig.data());
    //TODO: support other mimes with options->parserName

    try {
        for (TVector<TString>::const_iterator i = files.begin(), mi = files.end(); i != mi; ++i) {
            TestFile(options, htProcessor, *i);
        }
    } catch (const yexception& e) {
        Cerr << e.what() << Endl;
        return 1;
    }

    return 0;
}
