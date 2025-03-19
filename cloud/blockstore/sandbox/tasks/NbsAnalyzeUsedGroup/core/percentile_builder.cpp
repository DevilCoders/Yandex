#include "percentile_builder.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/writer/json_value.h>

#include <algorithm>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TGroupData
{
    ui32 GroupId = 0;
    ui64 Data = 0;
};

TVector<TGroupData> GetSortedData(
    const THostMetrics::TGroupMap& metrics,
    TPercentileBuilder::TType type)
{
    TVector<TGroupData> result;
    result.reserve(metrics.size());

    switch(type)
    {
    case TPercentileBuilder::TType::READ_BYTES:
        for (const auto& [groupId, ops]: metrics) {
            result.push_back({groupId, ops.ReadOperations.ByteCount});
        }
        break;

    case TPercentileBuilder::TType::READ_IOPS:
        for (const auto& [groupId, ops]: metrics) {
            result.push_back({groupId, ops.ReadOperations.Iops});
        }
        break;

    case TPercentileBuilder::TType::WRITE_BYTES:
        for (const auto& [groupId, ops]: metrics) {
            result.push_back({groupId, ops.WriteOperations.ByteCount});
        }
        break;
    case TPercentileBuilder::TType::WRITE_IOPS:
        for (const auto& [groupId, ops]: metrics) {
            result.push_back({groupId, ops.WriteOperations.Iops});
        }
        break;
    default:
        break;
    }

    SortBy(result, [](const auto& value) {return value.Data;});

    return result;
}

constexpr size_t GetIndex(double percentile, size_t size)
{
    return size * percentile / 100 - 1;
}

TVector<ui32> GetTopGroup(const THashMap<ui32, int>& groups, size_t count)
{
    TVector<std::pair<ui32, int>> tmp(groups.begin(), groups.end());
    SortBy(tmp, [](const auto& value) {return value.second;});

    TVector<ui32> result(std::min(tmp.size(), count));
    auto it = tmp.rbegin();

    std::transform(
        it,
        std::next(it, result.size()),
        result.begin(),
        [] (auto value) { return value.first; });

    return result;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TPercentileBuilder::TKindToData TPercentileBuilder::Build(
    const TVector<THostMetrics>& metrics)
{
    static const double percentiles[] = {50, 90, 99, 99.9, 100};

    TKindToData result;
    auto fillResult = [&](
        const TString& kind,
        const THostMetrics::TGroupMap& groupMap,
        ui64 timestamp,
        TType type)
    {
        TVector<TGroupData> sortedData =
            GetSortedData(groupMap, type);

        for (auto const& percentile: percentiles) {
            const size_t index = GetIndex(percentile, sortedData.size());
            result[kind].percentileToData[percentile][type].x.push_back(
                timestamp);
            result[kind].percentileToData[percentile][type].y.push_back(
                sortedData[index].Data);
            for(size_t i = 0; i < sortedData.size(); ++i) {
                result[kind].groupCount.insert(sortedData[i].GroupId);
                if (i >= index) {
                    ++result[kind]
                        .percentileToData[percentile][type]
                        .Groups[sortedData[i].GroupId];
                }
            }
        }
    };

    for (const auto& metric: metrics) {
        for (const auto& [kind, groupMap]: metric.PoolKind2LoadData) {
            if (kind.empty()) {
                continue;
            }

            for(int type = 0; type < TType::COUNT; ++type) {
                fillResult(
                    kind,
                    groupMap,
                    metric.Timestamp.Seconds(),
                    static_cast<TType>(type));
            }
        }
    }

    return result;
}

TString TPercentileBuilder::ConverToJson(const TKindToData& metrics)
{
    NJsonWriter::TBuf result;

    auto list = result.BeginObject();
    for (const auto& [kind, percentileData]: metrics) {
        auto kindList = list.WriteKey(kind)
            .BeginObject()
            .WriteKey("percentile_array")
            .BeginList();
        for (const auto& [percentile, typeData]: percentileData.percentileToData) {
            for (const auto& [type, data]: typeData) {
                auto dataList = kindList.BeginObject()
                    .WriteKey("percentile")
                    .WriteDouble(percentile)
                    .WriteKey("type")
                    .WriteInt(type)
                    .WriteKey("data")
                    .BeginObject()
                    .WriteKey("x")
                    .BeginList();
                for (const auto& point: data.x) {
                    dataList.WriteULongLong(point);
                }
                dataList.EndList()
                    .WriteKey("y")
                    .BeginList();
                for (const auto& point: data.y) {
                    dataList.WriteULongLong(point);
                }
                dataList.EndList()
                    .WriteKey("groups")
                    .BeginList();
                for(auto&& tmp: GetTopGroup(data.Groups, 10)) {
                    dataList.WriteLongLong(tmp);
                }
                dataList.EndList()
                    .EndObject()
                    .EndObject();
            }
        }
        kindList.EndList()
            .WriteKey("groups_count")
            .WriteInt(percentileData.groupCount.size())
            .EndObject();
    }
    list.EndObject();

    return result.Str();
}


}   // NCloud::NBlockStore::NAnalyzeUsedGroup
