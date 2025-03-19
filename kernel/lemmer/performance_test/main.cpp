#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmaforms.h>
#include <util/datetime/cputimer.h>
#include <util/generic/string.h>
#include <util/stream/file.h>

using namespace NLastGetopt;

class TCycleTimer {
public:
    TCycleTimer() {
        BeginTime = GetCycleCount();
    }

    ui64 Get() {
        return GetCycleCount() - BeginTime;
    }
private:
    ui64 BeginTime = 0;
};

struct TParams {
    TString PathInput;
    TString PathOutput;
    IInputStream* In = &Cin;
    IOutputStream* Out = &Cout;
    THolder<IInputStream> InHolder;
    THolder<IOutputStream> OutHolder;
    bool PrintWordsTime = false;
    size_t Multiplier = 20;
};

void AddOptions(TOpts& opts, TParams& params) {
    opts.AddLongOption('i', "input-file")
        .StoreResult(&params.PathInput)
        .Help("file: <word> <count>")
        .Handler1T<TString>([&params](const auto& path) {
            params.InHolder = MakeHolder<TFileInput>(path);
            params.In = params.InHolder.Get();
        });
    opts.AddLongOption('o', "output-file")
        .StoreResult(&params.PathOutput)
        .Handler1T<TString>([&params](const auto& path) {
            params.OutHolder = MakeHolder<TFixedBufferFileOutput>(path);
            params.Out = params.OutHolder.Get();
        });
    opts.AddLongOption('w', "print-words-time")
        .StoreResult(&params.PrintWordsTime, true)
        .NoArgument();
    opts.AddLongOption('m', "multiplier")
        .StoreResult(&params.Multiplier)
        .Help("Multiplier for word count; default = 20");
}

template <typename TMyTimer, typename TTime>
void PrintTime(IInputStream& in, IOutputStream& out, TParams& params, TTime sumTime) {
    TString line;
    while (in.ReadLine(line)) {
        TStringBuf lineBuf(line);
        auto word = lineBuf.NextTok('\t');
        auto countStr = lineBuf.NextTok('\t');
        size_t count = FromString<size_t>(countStr);
        TWLemmaArray output;
        TUtf16String wideWord = UTF8ToWide(TString(word));
        TMyTimer timer;
        for (size_t i = 0; i < count * params.Multiplier; i++) {
            NLemmer::AnalyzeWord(wideWord.data(), wideWord.size(), output, LANG_ENG);
            for (auto lemma : output) {
                TLemmaForms lemmaForms(lemma);
                lemmaForms.ObtainForms();
            }
        }
        TTime wordTime = timer.Get();
        if (params.PrintWordsTime) {
            out << word << "\t" << wordTime << "\t" << count << Endl;
        }
        sumTime += wordTime;
    }
    out << sumTime << Endl;
}

int ProfileTimer(int argc, const char** argv) {
    TParams params;
    TOpts opts = TOpts::Default();
    AddOptions(opts, params);
    TOptsParseResult res(&opts, argc, argv);
    PrintTime<TProfileTimer, TDuration>(*params.In, *params.Out, params, TDuration());
    return 0;
}

int CycleTimer(int argc, const char** argv) {
    TParams params;
    TOpts opts = TOpts::Default();
    AddOptions(opts, params);
    TOptsParseResult res(&opts, argc, argv);
    PrintTime<TCycleTimer, ui64>(*params.In, *params.Out, params, 0);
    return 0;
}


int main(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode(
        "profile-timer",
        ProfileTimer,
        "Profile timer");
    modChooser.AddMode(
        "cycle-timer",
        CycleTimer,
        "Cycle Timer");
    modChooser.Run(argc, argv);
    return 0;
}
