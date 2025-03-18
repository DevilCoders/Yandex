#include <ysite/yandex/spamfilt/spamfilt.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/html/face/propface.h>
#include <library/cpp/html/pdoc/pds.h>
#include <library/cpp/uri/http_url.h>

#include <library/cpp/charset/doccodes.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/network/socket.h>
#include <util/stream/file.h>
#include <library/cpp/http/io/stream.h>

#include <cctype>
#include <cerrno>
#include <cstdio>

#ifdef _win32_
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <winsock2.h>
#endif

void DoWork(THtProcessor& htProcessor, const char* fname, const char* cpName, bool printDocStat, bool printSpamStat)
{
    char tmpfilename[L_tmpnam];
    if (!strncmp(fname, "http://", 7)) {
        THttpURL pUrl;
        if (pUrl.Parse(fname) != THttpURL::ParsedOK || pUrl.IsNull(THttpURL::FlagHost))
            ythrow yexception() << "Error: can't download " <<  fname << ": " << LastSystemErrorText();
        TString relUrl = pUrl.PrintS(THttpURL::FlagPath | THttpURL::FlagQuery);
        TNetworkAddress addr(pUrl.Get(THttpURL::FieldHost), pUrl.GetPort());
        TSocket s(addr);
        SendMinimalHttpRequest(s, pUrl.Get(THttpURL::FieldHost), relUrl.data());
        TSocketInput si(s);
        THttpInput hi(&si);
        unsigned nRet = ParseHttpRetCode(hi.FirstLine());
        if (nRet != 200)
            ythrow yexception() << "Error: can't download " <<  fname << ": " << LastSystemErrorText();
        TFixedBufferFileOutput so(fname);
        TransferData(&hi, &so);
    }
    TFileInput inp(fname);
    THolder<IParsedDocProperties> docProps(htProcessor.ParseHtml((IInputStream*)&inp, "http://127.0.0.1/"));
    if (cpName)
        docProps->SetProperty(PP_CHARSET, cpName);
    FilterSpam(&htProcessor, docProps.Get(), printDocStat, printSpamStat);
    if (fname == tmpfilename)
        remove(fname);
}


void Usage(char* argv[]) {
    fprintf(stderr, "Copyright (c) OOO \"Yandex\". All rights reserved.\n");
    fprintf(stderr, "printwordstat - prints word statistics from document\n");
    const char* moreopt = " [-sd] ";

    fprintf(stderr, "Usage: %s [-p html|rtf|pdf|mp3|txt|swf]%s[-c cfgname] [-e cpName] [-o fname] filename\n", argv[0], moreopt);
    fprintf(stderr, "      -p html|rtf|pdf|mp3|txt|swf -- dynamic parser name\n");
    fprintf(stderr, "      -c cfgname -- configuration filename for parser\n");
    fprintf(stderr, "      -e cpName -- use specified encoding (windows-1251 by default)\n");
    fprintf(stderr, "      -o fname -- associates stdout with fname\n");
    fprintf(stderr, "      -s: do NOT print spam stat\n");
    fprintf(stderr, "      -d: do NOT print doc stat\n");
}

int main(int argc, char *argv[])
{
    InitNetworkSubSystem();
    Opt opt(argc, argv, "sdo:p:c:e:");
    int optlet;
    const char * parser_name = "html";
    const char * parser_config = nullptr;
    const char * cpName = "windows-1251";
    bool printDocStat = true;
    bool printSpamStat = true;
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
        case 'o':
            if (freopen(opt.Arg, "w", stdout) == nullptr) {
                perror(opt.Arg);
                return EXIT_FAILURE;
            }
            break;
        case 'p':
            parser_name = strdup(opt.Arg);
            fprintf(stderr, "Error: option \"p\" is not supported\n");
            break;
        case 'e':
            cpName = opt.Arg;
            break;
        case 'c':
            parser_config = strdup(opt.Arg);
            break;
        case 's': {
            printSpamStat = false;
            break;
        }
        case 'd': {
            printDocStat = false;
            break;
        }
        case '?':
            Usage(argv);
            return EXIT_FAILURE;
        }
    }
    Y_UNUSED(parser_name);
    if (argc == opt.Ind) {
        Usage(argv);
        return EXIT_FAILURE;
    }

    try {
        THtProcessor htProcessor;
        if (parser_config)
            htProcessor.Configure(parser_config);
        DoWork(htProcessor, argv[opt.Ind], cpName, printDocStat, printSpamStat);
    } catch(std::exception& re) {
        fprintf(stderr, "Error:  %s\n", re.what());
        return EXIT_FAILURE;
    }

#ifdef _win32_
    WSACleanup();
#endif
    return EXIT_SUCCESS;
}
