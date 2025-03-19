#include "metrics.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/writer/json_value.h>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

namespace {

////////////////////////////////////////////////////////////////////////////////

NJson::TJsonValue MakeDataSheme()
{
    NJson::TJsonValue dataSheme;

    dataSheme.AppendValue("Kind");
    dataSheme.AppendValue("GroupId");
    dataSheme.AppendValue("ReadByteCount");
    dataSheme.AppendValue("ReadByteIops");
    dataSheme.AppendValue("WriteByteCount");
    dataSheme.AppendValue("WriteByteIops");

    return dataSheme;
}

const NJson::TJsonValue DataSheme = MakeDataSheme();

inline THostMetrics::TLoadData SubAndDiv(
    const THostMetrics::TLoadData& lhs,
    const THostMetrics::TLoadData& rhs,
    ui64 seconds)
{
    if (seconds == 0) {
        return {};
    }

    auto sub = [](ui64 l, ui64 r) {
        return l >= r ? l - r : 0;
    };

    return {
        {
            sub(lhs.ReadOperations.ByteCount, rhs.ReadOperations.ByteCount) / seconds,
            sub(lhs.ReadOperations.Iops, rhs.ReadOperations.Iops) / seconds,
        },
        {
            sub(lhs.WriteOperations.ByteCount, rhs.WriteOperations.ByteCount) / seconds,
            sub(lhs.WriteOperations.Iops, rhs.WriteOperations.Iops) / seconds,
        },
    };
}

inline THostMetrics::TLoadData& operator+=(
    THostMetrics::TLoadData& lhs,
    const THostMetrics::TLoadData& rsv)
{
    lhs.ReadOperations.ByteCount += rsv.ReadOperations.ByteCount;
    lhs.ReadOperations.Iops += rsv.ReadOperations.Iops;
    lhs.WriteOperations.ByteCount += rsv.WriteOperations.ByteCount;
    lhs.WriteOperations.Iops += rsv.WriteOperations.Iops;

    return lhs;
}

THostMetrics ConvertToData(const NJson::TJsonValue& currentData)
{
    THostMetrics result;

    if (currentData.GetType() != NJson::JSON_ARRAY) {
        return result;
    }

    auto& array = currentData.GetArray();
    if (array.size() == 0 ||
        array.begin()->GetType() != NJson::JSON_ARRAY ||
        *array.begin() != DataSheme)
    {
        return result;
    }

    for (size_t i = 1; i < array.size(); ++i) {
        auto& data = array[i];
        if (data.GetType() == NJson::JSON_ARRAY) {
            result.PoolKind2LoadData
                [data[0].GetString()]
                [static_cast<ui32>(data[1].GetUInteger())] = {
                    { data[2].GetUInteger(),
                      data[3].GetUInteger() },
                    { data[4].GetUInteger(),
                      data[5].GetUInteger() } };
        }
    }

    return result;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

THostMetrics ConvertToData(const TString& currentData)
{
    NJson::TJsonValue jsonData;
    NJson::ReadJsonTree(currentData.data(), &jsonData);
    return ConvertToData(jsonData);
}

THostMetrics ConvertoToRate(
    const THostMetrics& previousData,
    const THostMetrics& currentData)
{
    THostMetrics result{.Timestamp = previousData.Timestamp};

    const ui64 seconds =
        (currentData.Timestamp - previousData.Timestamp).Seconds();

    for (const auto& [currentKind, currentGroupMap]:
        currentData.PoolKind2LoadData)
    {
        auto previouskind = previousData.PoolKind2LoadData.find(currentKind);
        if (previouskind != previousData.PoolKind2LoadData.end()) {
            for (const auto& [currentGroupId, data]: currentGroupMap) {
                auto previousCroupId = previouskind->second.find(currentGroupId);
                if (previousCroupId != previouskind->second.end()) {
                    result.PoolKind2LoadData[currentKind][currentGroupId] =
                       SubAndDiv(data, previousCroupId->second, seconds);
                }
            }
        }
    }

    return result;
}

void Append(
    THostMetrics& dst,
    const THostMetrics& src)
{
    for (const auto& [kind, groupMap]: src.PoolKind2LoadData) {
        for (const auto& [groupId, data]: groupMap) {
            dst.PoolKind2LoadData[kind][groupId] += data;
        }
    }
}

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
