#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/nodeiterator.h>
#include <library/cpp/getopt/last_getopt.h>

static void ProcessInput(IInputStream* input, IOutputStream* output) {
    TString query;
    while (input->ReadLine(query)) {
        TRichTreePtr tree = CreateRichTree(UTF8ToWide(query), TCreateTreeOptions());

        TConstNodesVector reqWords;
        GetChildNodes(*tree->Root, reqWords, IsWord, false);

        for (const TRichRequestNode* node : reqWords) {
            *output << WideToUTF8(node->GetText()) << " ";
        }
        *output << Endl;
    }
}

int main(int argc, char** argv)
{
    using namespace NLastGetopt;

    TOpts opts;
    TString inputFilename;
    TString outputFilename;

    opts.SetTitle("for each line of the input prints normalized line to the output");
    opts.AddLongOption('i', "input", "Path to input file. Default - cin").Optional().RequiredArgument("PATH").StoreResult(&inputFilename);
    opts.AddLongOption('o', "output", "Path to output file. Default - cout").Optional().RequiredArgument("PATH").StoreResult(&outputFilename);
    TOpt& optHelp = opts.AddHelpOption('h');

    TOptsParseResult r(&opts, argc, argv);

    if (r.Has(&optHelp)) {
        opts.PrintUsage(TStringBuf("convertor"));
        exit(0);
    }

    IInputStream* input = &Cin;
    IOutputStream* output = &Cout;

    THolder<IInputStream> inputHolder;
    THolder<IOutputStream> outputHolder;

    if (inputFilename) {
        inputHolder.Reset(new TFileInput(inputFilename));
        input = inputHolder.Get();
    }
    if (outputFilename) {
        outputHolder.Reset(new TFixedBufferFileOutput(outputFilename));
        output = outputHolder.Get();
    }

    ProcessInput(input, output);
}
