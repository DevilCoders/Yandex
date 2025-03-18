#include "generator.h"
#include "printer.h"

#include <contrib/libs/libpcap/pcap.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

struct TParams {
    TString CapFilePath;
    bool DolbilkaFormat = false;
    bool ProcessorAmmo = false;
    unsigned short HttpCodeFilter = 0;
    size_t MinimumRequestSize = 0;
    TString RequestRegExp;
};

TParams ParseParams(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "<file.cap>", "tcpdump dump file");
    TParams params;
    if (TOpt* helpOpt = opts.FindLongOption("help")) {
        helpOpt->AddShortName('h');
    }

    opts.AddLongOption("dolbilka-format")
        .NoArgument()
        .Optional()
        .SetFlag(&params.DolbilkaFormat)
        .DefaultValue(false)
        .Help("Do not write size of HTTP chunks");

    opts.AddLongOption("processor-ammo")
        .NoArgument()
        .Optional()
        .SetFlag(&params.ProcessorAmmo)
        .DefaultValue(false)
        .Help("Generate ammo for processor. Default is for cacher");

    opts.AddLongOption("filter-by-code")
        .Optional()
        .RequiredArgument("<HTTP code>")
        .StoreResult(&params.HttpCodeFilter)
        .Help("Filter requests by given HTTP response code");

    opts.AddLongOption("filter-by-regexp")
        .Optional()
        .RequiredArgument("<request regexp>")
        .StoreResult(&params.RequestRegExp)
        .Help("Filter requests by regexp");

    opts.AddLongOption("filter-by-size")
        .Optional()
        .RequiredArgument("<request size in bytes>")
        .StoreResult(&params.MinimumRequestSize)
        .Help("Skip requests smaller than given size");

    TOptsParseResult res(&opts, argc, argv);

    params.CapFilePath = res.GetFreeArgs()[0];

    return params;
}

pcap_t* GetPCapSession(TParams params) {
    char ErrorBuffer[PCAP_ERRBUF_SIZE]{0};
    const char* filePath = params.CapFilePath.c_str();
    pcap_t* session = pcap_open_offline(filePath, ErrorBuffer);
    if (session == nullptr) {
        ythrow yexception() << "Failed to open file ["sv
                            << filePath << "]: "sv
                            << ErrorBuffer;
    }
    return session;
}

int main(int argc, char** argv) {
    TParams params = ParseParams(argc, argv);
    auto printer = CreateAmmoPrinter(params.DolbilkaFormat);
    auto filter = CreateAmmoFilter(params.HttpCodeFilter, params.MinimumRequestSize, params.RequestRegExp);
    auto ammoGenerator = CreateAmmoGenerator(params.ProcessorAmmo, printer.Get(), filter);

    pcap_t* session = GetPCapSession(params);
    struct pcap_pkthdr header;
    const unsigned char* pkt;

    while ((pkt = pcap_next(session, &header)) != nullptr) {
        ammoGenerator->Process(header, pkt);
    }

    ammoGenerator->PrintStats();

    pcap_close(session);
}
