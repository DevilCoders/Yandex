#include "compressor.h"

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/arcface.h>

#include <library/cpp/yson/varint.h>

#include <util/generic/array_ref.h>
#include <util/generic/buffer.h>
#include <util/generic/maybe.h>
#include <util/stream/buffer.h>
#include <util/stream/mem.h>
#include <util/stream/zlib.h>

constexpr TStringBuf ReservedDocDescrUrlKeyName = "urldescr_url";
constexpr std::initializer_list<TStringBuf> KeysWithCuttableScheme = {TStringBuf("mainContentUrl")};

TBlob DecompressExtArc(TBlob blob, TConstArrayRef<TString> keys) {
    TBuffer curVal;
    TDocDescr descr;
    TDocInfoExtWriter writer;
    TBuffer newInfo;
    descr.UseBlob(&newInfo);
    DocUrlDescr& urlDescr = descr.GetMutableUrlDescr();

    TMemoryInput sin(blob.AsCharPtr(), blob.Size());
    TZLibDecompress in(&sin, ZLib::ZLib);

    auto getInt32 = [&] {
        i32 res;
        NYson::ReadVarInt32(&in, &res);
        return res;
    };
    urlDescr.HostId = getInt32();
    urlDescr.UrlId = getInt32();
    urlDescr.HttpModTime = getInt32();
    urlDescr.Size = getInt32();
    in.Read(&(urlDescr.Encoding), 1);
    in.Read(&(urlDescr.MimeType), 1);

    i32 valuesSize;
    NYson::ReadVarInt32(&in, &valuesSize);
    struct TValueHeader {
        i32 Prefix;
        i32 Suffix;
        i32 Id;
    };
    TVector<TValueHeader> values(Reserve(valuesSize));

    for (i32 i = 0; i < valuesSize; ++i) {
        TValueHeader header;
        NYson::ReadVarInt32(&in, &header.Id);
        NYson::ReadVarInt32(&in, &header.Prefix);
        NYson::ReadVarInt32(&in, &header.Suffix);
        values.push_back(header);
    }

    THashMap<TString, TString> docInfos;
    for (i32 i = 0; i < valuesSize; ++i) {
        curVal.Resize(values[i].Prefix);
        curVal.Reserve(curVal.Size() + values[i].Suffix + 1);
        in.Load(curVal.Pos(), values[i].Suffix);
        curVal.Resize(curVal.Size() + values[i].Suffix);
        *(curVal.Pos()) = '\0';

        if (keys[values[i].Id] == ReservedDocDescrUrlKeyName) {
            descr.SetUrl(TString(curVal.data(), curVal.size()));
        } else {
            writer.Add(keys[values[i].Id].data(), curVal.Data());
        }
    }
    writer.Write(descr);

    TBuffer result;
    TBufferOutput out(result);
    WriteEmptyDoc(out, newInfo.data(), descr.CalculateBlobSize(), nullptr, 0, 0);
    result.ChopHead(sizeof(TArchiveHeader));
    return TBlob::FromBuffer(result);
}

/*
 * All extensions except DocInfo are deleted.
 */
TBlob CompressExtArc(TBlob blob, TVector<NJupiter::TExtArcKey> keys) {
    THashMap<TString, TString> docInfo;
    TDocDescr descr;
    const DocUrlDescr* urlDescr = reinterpret_cast<const DocUrlDescr*>(blob.Data());
    descr.UseBlob(blob.AsCharPtr(), blob.Size());
    descr.ConfigureDocInfos(docInfo);

    auto cutHttp = [](TString url) {
        if (url.StartsWith("http://")) {
            return url.substr(7);
        } else {
            return url;
        }
    };

    TVector<std::pair<TString, ui32>> values;
    for (size_t i = 0; i < keys.size(); ++i) {
        if (auto ptr = docInfo.FindPtr(keys[i].GetKey())) {
            values.emplace_back(*ptr, keys[i].GetId());
        } else if (keys[i].GetKey() == ReservedDocDescrUrlKeyName) {
            values.emplace_back(urlDescr->Url, keys[i].GetId());
        } else {
            continue;
        }

        for (auto name : KeysWithCuttableScheme) {
            if (keys[i].GetKey() == name) {
                values.back().first = cutHttp(values.back().first);
            }
        }
    }
    Sort(values);

    TBuffer result;;
    TBufferOutput resOut(result);
    TZLibCompress out(&resOut, ZLib::ZLib);
    NYson::WriteVarInt32(&out, urlDescr->HostId);
    NYson::WriteVarInt32(&out, urlDescr->UrlId);
    NYson::WriteVarInt32(&out, urlDescr->HttpModTime);
    NYson::WriteVarInt32(&out, urlDescr->Size);
    out.Write(&(urlDescr->Encoding), 1);
    out.Write(&(urlDescr->MimeType), 1);

    NYson::WriteVarInt32(&out, values.size());
    TMaybe<TStringBuf> prevVal;
    TVector<size_t> prefixes;
    for (auto value : values) {
        NYson::WriteVarInt32(&out, value.second);
        size_t prefix = 0;
        if (prevVal) {
            while (prefix < value.first.size()
                && prefix < prevVal->size()
                && value.first[prefix] == (*prevVal)[prefix])
            {
                ++prefix;
            }
        }
        NYson::WriteVarInt32(&out, prefix);
        NYson::WriteVarInt32(&out, value.first.size() - prefix);
        prevVal = value.first;
        prefixes.push_back(prefix);
    }

    for (size_t i = 0; i < prefixes.size(); ++i) {
        out.Write(values[i].first.data() + prefixes[i], values[i].first.size() - prefixes[i]);
    }

    out.Finish();
    return TBlob::FromBuffer(result);
}

TVector<TString> ExtractExtArcKeys(TBlob blob) {
    TVector<TString> result = { TString(ReservedDocDescrUrlKeyName) };
    THashMap<TString, TString> docInfo;
    TDocDescr descr;
    descr.UseBlob(blob.AsCharPtr(), blob.Size());
    descr.ConfigureDocInfos(docInfo);
    for (auto& i: docInfo) {
        result.push_back(i.first);
        Y_ENSURE(i.first != ReservedDocDescrUrlKeyName);
    }
    return result;
}
