#include <kernel/mirrors/mirrors_wrapper.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/uri/http_url.h>

#include <util/generic/yexception.h>
#include <util/stream/file.h>

static const char* GetMainMirror(const IMirrors& m, const char* host) {
    const char* check = m.Check(host);
    return (check) ? check : host;
}

static void PrintMainMirrorsStdin(const IMirrors& m) {
    TString url;
    while (Cin.ReadLine(url))
        printf("%s\n", GetMainMirror(m, url.data()));
}

static void PrintMainMirrorsUrlsStdin(const IMirrors& m) {
    TString url;
    while (Cin.ReadLine(url)) {
        url = TString::Join("http://", url);
        THttpURL purl;
        purl.Parse(url);
        purl.Set(THttpURL::FieldHost, GetMainMirror(m, purl.Get(THttpURL::FieldHost)));
        printf("%s\n", purl.PrintS().substr(7).data());
    }
}

static void PrintMainMirrors(const IMirrors& m, int argc, char** argv) {
    for (int i = 0; i < argc; ++i)
        printf("%s\n", GetMainMirror(m, argv[i]));
}

static void JoinByMirrors(const IMirrors& m, const TString& input, const TString& output) {
    TFileInput fIn(input);
    TFixedBufferFileOutput fOut(output);
    TString line;
    while (fIn.ReadLine(line)) {
        size_t i = 0;
        while ( (i < line.size()) && (' ' != line[i]) && ('\t' != line[i]) && ('/' != line[i]) )
            ++i;
        TString host(line.data(), 0, i);
        TString tail(line.data(), i, line.size() - i);
        IMirrors::THosts hosts;
        m.GetGroup(host.data(), &hosts);
        if (!hosts.empty()) {
            for (size_t j = 0; j < hosts.size(); ++j)
                fOut << hosts[j] << tail << "\n";
        } else {
            fOut << host << tail << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        bool useStdin = false;
        bool processUrls = false;
        bool join = false;
        TString mirrorsInput = "/Berkanavt/dbspider/config/mirrors.res";
        TString mirrorsHashedInput;
        TString input;
        TString output;

        Opt opt(argc, argv, "i:sI:O:jh:p");
        OPTION_HANDLING_PROLOG;
        OPTION_HANDLE('i', (mirrorsInput = opt.Arg));
        OPTION_HANDLE('h', (mirrorsHashedInput = opt.Arg));
        OPTION_HANDLE('I', (input = opt.Arg));
        OPTION_HANDLE('O', (output = opt.Arg));
        OPTION_HANDLE('s', (useStdin = true));
        OPTION_HANDLE('j', (join = true));
        OPTION_HANDLE('p', (processUrls = true));
        OPTION_HANDLING_EPILOG;

        TAutoPtr<IMirrors> mirrors;
        if (!mirrorsHashedInput.empty())
            mirrors.Reset(new TMirrorsHashed(mirrorsHashedInput));
        else
            mirrors.Reset(new TMirrors(mirrorsInput));
        if (join)
            JoinByMirrors(*mirrors, input, output);
        else if (processUrls)
            PrintMainMirrorsUrlsStdin(*mirrors);
        else if (!useStdin)
            PrintMainMirrors(*mirrors, argc - opt.Ind, argv + opt.Ind);
        else
            PrintMainMirrorsStdin(*mirrors);

        return 0;
    } catch (...) {
        Cerr << "Exception: " << CurrentExceptionMessage() << Endl;
        return 1;
    }
}

