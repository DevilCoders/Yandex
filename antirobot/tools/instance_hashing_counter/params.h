#pragma once

#include "ip_map.h"

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>

struct TParams {
    TString TablePath;
    TString YtCluster = "hahn";
    ui16 TopHostCount = 10;
    ui16 TopSubnetCount = 5;
    ui16 Ipv4DefaultSubnet = 16;
    ui16 Ipv6DefaultSubnet = 48;
    TIpRangeMap<size_t> CustomHashRules;
    bool LocalProcessing = false;
};

TParams ParseParams(int argc, const char** argv) {
    using namespace NLastGetopt;
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    TParams params;
    if (TOpt* helpOpt = opts.FindLongOption("help")) {
        helpOpt->AddShortName('h');
    }

    opts.AddLongOption("table", "Path to YT-table to read requests from")
        .StoreResult(&params.TablePath)
        .Required();

    opts.AddLongOption("yt-cluster", "YT cluster")
        .StoreResult(&params.YtCluster)
        .DefaultValue(params.YtCluster)
        .Optional();

    opts.AddLongOption("top-host-count", "Number of top hosts to print statistics about")
        .StoreResult(&params.TopHostCount)
        .DefaultValue(params.TopHostCount)
        .Optional();

    opts.AddLongOption("top-subnet-count", "Number of top subnets for each host to print statistics about")
        .StoreResult(&params.TopSubnetCount)
        .DefaultValue(params.TopSubnetCount)
        .Optional();

    opts.AddLongOption("ipv4-subnet", "Default subnet size for IPv4")
        .StoreResult(&params.Ipv4DefaultSubnet)
        .DefaultValue(params.Ipv4DefaultSubnet)
        .Optional();

    opts.AddLongOption("ipv6-subnet", "Default subnet size for IPv6")
        .StoreResult(&params.Ipv6DefaultSubnet)
        .DefaultValue(params.Ipv6DefaultSubnet)
        .Optional();

    opts.AddLongOption("local", "Do all the work on this host. Without YT.")
        .SetFlag(&params.LocalProcessing)
        .DefaultValue(params.LocalProcessing)
        .NoArgument();

    TVector<TString> rules;
    opts.AddLongOption("custom-hash-rules", "Custom subnet hash rules. For example: 176.59.0.0/16=18,213.87.0.0/17=17")
        .SplitHandler(&rules, ',')
        .Optional();

    TOptsParseResult res(&opts, argc, argv);

    for (const TString& rule : rules) {
        TStringBuf network, newSize;
        TStringBuf{rule}.Split('=', network, newSize);
        params.CustomHashRules.Insert({ParseNetwork(network), FromString<size_t>(newSize)});
    }
    params.CustomHashRules.EnsureNoIntersections();

    return params;
}
