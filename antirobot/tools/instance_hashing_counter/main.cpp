#include "params.h"
#include "util.h"

#include <antirobot/tools/instance_hashing_counter/proto/l7_access_log_row.pb.h>
#include <antirobot/tools/instance_hashing_counter/proto/daemon_log_row.pb.h>
#include <antirobot/tools/instance_hashing_counter/proto/mapper_output_row.pb.h>
#include <antirobot/tools/instance_hashing_counter/proto/reducer_output_row.pb.h>

#include <antirobot/daemon_lib/config_global.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/common.h>
#include <mapreduce/yt/interface/io.h>
#include <mapreduce/yt/library/table_schema/protobuf.h>
#include <mapreduce/yt/util/temp_table.h>

#include <util/generic/strbuf.h>
#include <util/stream/format.h>
#include <util/stream/output.h>
#include <util/system/env.h>

using namespace NAntiRobot;
using namespace NYT;

template <typename TInputRow>
class TChooseInstanceMapper
   : public IMapper<
          TTableReader<TInputRow>,
          TTableWriter<TMapperOutputRow>> {
public:
    using TReader = TTableReader<TInputRow>;
    using TWriter = TTableWriter<TMapperOutputRow>;

    TChooseInstanceMapper() = default;

    TChooseInstanceMapper(size_t defaultIpv4Subnet, size_t defaultIpv6Subnet, ::TIpRangeMap<size_t> customHashRules)
        : DefaultIpv4Subnet(defaultIpv4Subnet)
        , DefaultIpv6Subnet(defaultIpv6Subnet)
        , CustomHashRules(std::move(customHashRules))
    {
    }

    void Do(TReader* reader, TWriter* writer) override {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.IpV4SubnetBitsSizeForHashing = DefaultIpv4Subnet;
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.IpV6SubnetBitsSizeForHashing = DefaultIpv6Subnet;
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            auto maybeIndexAndSubnet = TryGetIndexAndSubnetByRow(row, CustomHashRules);

            if (maybeIndexAndSubnet.Empty()) {
                continue;
            }

            const auto& [index, subnet] = maybeIndexAndSubnet.GetRef();

            TMapperOutputRow outRow;
            outRow.SetIndex(index);
            outRow.SetSubnet(subnet);
            writer->AddRow(outRow);
        }
    }

    Y_SAVELOAD_JOB(DefaultIpv4Subnet, DefaultIpv6Subnet, CustomHashRules);

private:
    size_t DefaultIpv4Subnet = 16;
    size_t DefaultIpv6Subnet = 48;
    ::TIpRangeMap<size_t> CustomHashRules = {};
};
REGISTER_MAPPER(TChooseInstanceMapper<TL7AccessLogRow>);
REGISTER_MAPPER(TChooseInstanceMapper<TDaemonLogRow>);

class TCountInstanceReducer
   : public IReducer<
          TTableReader<TMapperOutputRow>,
          TTableWriter<TReducerOutputRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        TReducerOutputRow result;

        if (reader->IsValid()) {
            result.SetIndex(reader->GetRow().GetIndex());
            result.SetSubnet(reader->GetRow().GetSubnet());
        }

        size_t count = 0;
        for (; reader->IsValid(); reader->Next()) {
            ++count;
        }

        result.SetCount(count);
        writer->AddRow(result);
    }
};
REGISTER_REDUCER(TCountInstanceReducer);

struct TInstanceSubnetCount {
    size_t InstanceIndex;
    TString Subnet;
    size_t Count;

    TInstanceSubnetCount(size_t instanceIndex, TString subnet, size_t count)
        : InstanceIndex(instanceIndex)
        , Subnet(std::move(subnet))
        , Count(count)
    {
    }
};

template<typename TInputRow>
TVector<TInstanceSubnetCount> ProcessByMapReduceJob(IClientPtr client, const TParams& params) {
    TString path = params.TablePath;

    TTempTable result{client};

    TVector<TString> inputColumns;
    if constexpr (std::is_same_v<TInputRow, TL7AccessLogRow>) {
        inputColumns = {"ip_port", "workflow"};
    } else if constexpr (std::is_same_v<TInputRow, TDaemonLogRow>) {
        inputColumns  = {"spravka_ip"};
    } else {
        static_assert(TDependentFalse<TInputRow>);
    }

    client->MapReduce(
        NYT::TMapReduceOperationSpec()
            .template AddInput<TInputRow>(TRichYPath(path).Columns(inputColumns))
            .template AddOutput<TReducerOutputRow>(result.Name())
            .SortBy({"idx", "subnet"})
            .ReduceBy({"idx", "subnet"}),
        new TChooseInstanceMapper<TInputRow>(params.Ipv4DefaultSubnet, params.Ipv6DefaultSubnet, params.CustomHashRules),
        new TCountInstanceReducer);

    auto reader = client->CreateTableReader<TReducerOutputRow>(result.Name());

    TVector<TInstanceSubnetCount> instanceCountPairs;

    for (; reader->IsValid(); reader->Next()) {
        auto& row = reader->GetRow();
        size_t idx = row.GetIndex();
        size_t cnt = row.GetCount();
        const TString& subnet = row.GetSubnet();
        instanceCountPairs.emplace_back(idx, subnet, cnt);
    }

    return instanceCountPairs;
}

template <typename TInputRow>
TVector<TInstanceSubnetCount> ProcessLocally(IClientPtr client, const TParams& params) {
    auto path = params.TablePath;
    auto rows = client->Get(path + "/@row_count").AsInt64();
    auto reader = client->CreateTableReader<TInputRow>(NYT::TRichYPath(path).Columns({"ip_port", "workflow"}));

    NAntiRobot::ANTIROBOT_DAEMON_CONFIG_MUTABLE.IpV4SubnetBitsSizeForHashing = params.Ipv4DefaultSubnet;
    NAntiRobot::ANTIROBOT_DAEMON_CONFIG_MUTABLE.IpV6SubnetBitsSizeForHashing = params.Ipv6DefaultSubnet;

    size_t iteration = 0;

    TMap<TString, size_t> countByInstances[GetInstanceCount()] = {};

    TInstant start = TInstant::Now();
    for (; reader->IsValid(); reader->Next()) {
        auto& row = reader->GetRow();
        const auto& maybeIndexAndSubnet = TryGetIndexAndSubnetByRow(row, params.CustomHashRules);

        if (maybeIndexAndSubnet.Empty()) {
            continue;
        }

        const auto& [index, subnet] = maybeIndexAndSubnet.GetRef();

        countByInstances[index][subnet]++;

        iteration++;
        if (iteration % 16384 == 0) {
            double speed = iteration * 1.0 / (TInstant::Now() - start).MicroSeconds();
            auto leftRows = rows - iteration;
            auto leftTime = TDuration::MicroSeconds(static_cast<size_t>(leftRows / speed));
            Cerr << '\r' << iteration << " " << static_cast<int>(iteration * 100.0 / rows) << '%'
                 << " ETA: " << leftTime << "                              \r";
            Cerr.Flush();
        }
    }

    Cerr << "                                           \r";
    Cerr.Flush();

    TVector<TInstanceSubnetCount> instanceCountPairs;

    for (size_t i = 0; i < GetInstanceCount(); i++) {
        for (const auto& p : countByInstances[i]) {
            instanceCountPairs.emplace_back(i, p.first, p.second);
        }
    }

    return instanceCountPairs;
}

int main(int argc, const char** argv) {
    SetEnv("YT_LOG_LEVEL", "INFO");
    Initialize(argc, argv);

    TParams params = ParseParams(argc, argv);

    auto client = CreateClient(params.YtCluster);

    TVector<TInstanceSubnetCount> instanceCountPairs;
    if (params.LocalProcessing) {
        instanceCountPairs = ProcessLocally<TDaemonLogRow>(client, params);
    } else {
        instanceCountPairs = ProcessByMapReduceJob<TDaemonLogRow>(client, params);
    }

    TVector<std::pair<size_t, std::pair<size_t, TVector<std::pair<TString, size_t>>>>> mp(GetInstanceCount());

    for (const auto& x : instanceCountPairs) {
        mp[x.InstanceIndex].first = x.InstanceIndex;
        mp[x.InstanceIndex].second.first += x.Count;
        mp[x.InstanceIndex].second.second.emplace_back(x.Subnet, x.Count);
    }

    SortBy(mp, [](const std::pair<size_t, std::pair<size_t, TVector<std::pair<TString, size_t>>>>& x) {
        return Max<size_t>() - x.second.first;
    });

    const size_t topHostCount = params.TopHostCount;
    const size_t topSubnetCount = params.TopSubnetCount;

    Cout << "Top " << Min(topHostCount, mp.size()) << ":\n";
    for (size_t i = 0; i < Min(topHostCount, mp.size()); i++) {
        if (mp[i].second.first == 0) {
            continue;
        }

        Cout << "\tbackend = " << GetInstanceName(mp[i].first)
             << " count = " << mp[i].second.first << Endl;
        SortBy(mp[i].second.second, [](const std::pair<TString, size_t>& p) { return Max<size_t>() - p.second; });
        for (size_t j = 0; j < Min(topSubnetCount, mp[i].second.second.size()); j++) {
            double percent = mp[i].second.second[j].second * 100.0 / mp[i].second.first;
            Cerr << "\t\tsubnet = " << mp[i].second.second[j].first
                 << " count = " << mp[i].second.second[j].second
                 << " (" << Prec(percent, EFloatToStringMode::PREC_POINT_DIGITS, 2) << "%)" << Endl;
        }
    }
    Cout << Endl;
}
