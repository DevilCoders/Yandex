#include "feature_pool.h"

#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/json/json_reader.h>

#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/split.h>

template<>
void Out<NMLPool::EFeatureType>(IOutputStream& os, NMLPool::EFeatureType value)
{
    os << NMLPool::EFeatureType_Name(value);
}

template<>
bool TryFromStringImpl<NMLPool::EFeatureType>(const char* data, size_t size,
    NMLPool::EFeatureType& value)
{
    return NMLPool::EFeatureType_Parse(TString(data, size), &value);
}

template<>
void Out<NMLPool::TFeatureSlice>(IOutputStream& os, const NMLPool::TFeatureSlice& value)
{
    os << value.Name << '[' << value.Begin << ';' << value.End << ')';
}

template<>
bool TryFromStringImpl<NMLPool::TFeatureSlice>(const char* data, size_t size,
    NMLPool::TFeatureSlice& value)
{
    return NMLPool::TryParseBordersStr(TStringBuf(data, size), value);
}

template<>
void Out<NMLPool::TFeatureSlices>(IOutputStream& os, const NMLPool::TFeatureSlices& value)
{
    bool first = true;
    for (const auto& slice : value) {
        if (first) {
            first = false;
        } else {
            os << ' ';
        }
        os << slice;
    }
}

template<>
bool TryFromStringImpl<NMLPool::TFeatureSlices>(const char* data, size_t size,
    NMLPool::TFeatureSlices& value)
{
    return NMLPool::TryParseBordersStr(TStringBuf(data, size), value);
}

namespace NMLPool {
    bool HasTag(const TFeatureInfo& info, const TString& tagName)
    {
        for (const auto& name : info.GetTags()) {
            if (name == tagName) {
                return true;
            }
        }
        return false;
    }

    TString GetBordersStr(const TFeatureSlice& slice)
    {
        return TStringBuilder{} << slice;
    }

    TString GetBordersStr(const TFeatureSlices& slices)
    {
        return TStringBuilder{} << slices;
    }

    bool TryParseBordersStr(const TStringBuf bordersStr, TFeatureSlice& slice)
    {
        TStringBuf buf = bordersStr;

        TStringBuf name;
        buf.NextTok('[', name);
        TStringBuf beginStr;
        buf.NextTok(';', beginStr);
        TStringBuf endStr;
        buf.NextTok(')', endStr);

        size_t begin = 0;
        size_t end = 0;

        if (!name ||
            !TryFromString(beginStr, begin) ||
            !TryFromString(endStr, end) ||
            !!buf)
        {
            return false;
        }

        slice = TFeatureSlice(TString(name), begin, end);
        return true;
    }

    bool TryParseBordersStr(const TStringBuf bordersStr, TFeatureSlices& slices)
    {
        for (const auto& part : StringSplitter(bordersStr).SplitBySet(" \t\n")) {
            TStringBuf sliceBuf = part.Token();

            if (sliceBuf.empty()) {
                continue;
            }

            TFeatureSlice slice;
            if (!TryParseBordersStr(sliceBuf, slice)) {
                return false;
            }
            slices.push_back(slice);
        }
        return true;
    }

    TFeatureSlices GetSlices(const TPoolInfo& info)
    {
        TFeatureSlices slices;

        TString curSlice;
        size_t curBegin = 0;
        size_t index = 0;
        THashMap<TString, size_t> finishedSlices;

        for (const auto& elem : info.GetFeatureInfo()) {
            Y_ENSURE(!!elem.GetSlice(),
                "slice is not listed for factor " << elem.GetName());
            Y_ENSURE(!!curSlice || !!elem.GetSlice(),
                "pool starts with feature that has no slice name");

            if (elem.GetSlice() != curSlice) {
                if (!!curSlice) {
                    slices.emplace_back(curSlice, curBegin, index);
                    finishedSlices[curSlice] = curBegin;
                }
                curSlice = elem.GetSlice();
                curBegin = index;

                Y_ENSURE(!finishedSlices.contains(curSlice),
                    "slice \"" << curSlice << "\" occurs twice"
                    << ", first - starting at factor index " << finishedSlices[curSlice]
                    << ", second - starting at factor index " << curBegin);
            }
            index += 1;
        }

        if (!!curSlice) {
            slices.emplace_back(curSlice, curBegin, index);
        }
        return slices;
    }

    NJson::TJsonValue ToJson(const TFeatureInfo& info, bool extInfo)
    {
        NJson::TJsonValue res;
        NProtobufJson::TProto2JsonConfig config;
        config.SetEnumMode(NProtobufJson::TProto2JsonConfig::EnumName);

        TFeatureInfo mainInfo = info;
        mainInfo.ClearExtJson();
        NProtobufJson::Proto2Json(mainInfo, res, config);

        if (extInfo) {
            TStringInput extIn(info.GetExtJson());
            NJson::TJsonValue extValue;
            NJson::ReadJsonTree(&extIn, &extValue);
            res["ExtJson"] = extValue;
        }

        return res;
    }

    NJson::TJsonValue ToJson(const TPoolInfo& info, bool extInfo)
    {
        NJson::TJsonValue res;
        for (const auto& featureInfo : info.GetFeatureInfo()) {
            res.AppendValue(ToJson(featureInfo, extInfo));
        }
        return res;
    }
} // NMLPool
