#pragma once

#include <tools/wizard_yt/shard_packer/shard_packer.h>

#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/fast_sax/parser.h>
#include <library/cpp/threading/serial_postprocess_queue/serial_postprocess_queue.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <apphost/lib/service_testing/service_testing.h>
#include <apphost/lib/json/common.h>

#include <util/digest/city.h>
#include <util/folder/path.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/hash_set.h>
#include <util/string/builder.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/system/shellcommand.h>
#include <util/system/user.h>
#include <util/random/random.h>
#include <util/generic/set.h>

#include <functional>

constexpr size_t GB = 1UL << 30;
constexpr size_t MB = 1UL << 20;

auto REQUESTS_SCHEMA = NYT::TTableSchema()
    .AddColumn("reqid", NYT::EValueType::VT_STRING)
    .AddColumn("prepared_request", NYT::EValueType::VT_STRING)
    .AddColumn("__query_hash__", NYT::EValueType::VT_UINT64);

auto WORKER_SCHEMA = NYT::TTableSchema()
    .AddColumn("reqid", NYT::EValueType::VT_STRING)
    .AddColumn("begemot_answer", NYT::EValueType::VT_STRING)
    .AddColumn("shard", NYT::EValueType::VT_STRING)
    .AddColumn("has-reask", NYT::EValueType::VT_BOOLEAN)
    .AddColumn("has-misspelling", NYT::EValueType::VT_BOOLEAN)
    .AddColumn("has-grunwald", NYT::EValueType::VT_BOOLEAN)
    .AddColumn("begemot_grunwald_out", NYT::EValueType::VT_STRING)
    .AddColumn("__query_hash__", NYT::EValueType::VT_UINT64);

auto MERGER_SCHEMA = NYT::TTableSchema()
    .AddColumn("reqid", NYT::EValueType::VT_STRING)
    .AddColumn("begemot_answer", NYT::EValueType::VT_STRING)
    .AddColumn("__query_hash__", NYT::EValueType::VT_UINT64);

class TBegemotMapperBase : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void ExtractShard(TVector<TString>& files, TString extractPath) {
        Cerr << "Started shard unpacking" << Endl;
        Cerr << "Extract path: " << extractPath << Endl;
        TFsPath(extractPath).MkDirs();
        TThreadPool q(TThreadPool::TParams().SetBlocking(true).SetCatching(true));
        q.Start(30, 1024);
        bool failed = false;
        for (const auto& packedDir: files) {
            q.SafeAddFunc([&failed, &extractPath, &packedDir]() {
                try {
                    size_t size = TFileStat(packedDir).Size;
                    char *data = new char[size];
                    TFile(packedDir, RdOnly).Load(data, size);
                    TFsPath(packedDir).DeleteIfExists();
                    TMemoryInput input(data, size);
                    ShardPacker::RestoreDirectory(&input, extractPath);
                    delete[] data;
                } catch(...) {
                    failed = true;
                    TString msg = CurrentExceptionMessage();
                    fprintf(stderr, "Exception was thrown while unpacking %s: %s\n", packedDir.data(), msg.data());
                    fflush(stderr);
                }
            });
        }
        q.Stop();
        if (failed) {
            throw yexception() << "Failed to unpack shard";
        }
        Cerr << "Finished shard unpacking" << Endl;
    }
};

class TSimpleLogMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& requestRow = input->GetRow().AsMap();
            NYT::TNode resultRow = NYT::TNode()
                ("reqid", ToString(RandomNumber<ui64>()));
            TStringBuilder request;
            TBuffer buffer;
            for (auto it = requestRow.begin(); it != requestRow.end(); ++it) {
                if (it != requestRow.begin())
                    request << "&";
                TString value;
                if (it->second.IsString()) {
                    value = it->second.AsString();
                } else if (it->second.IsInt64()) {
                    value = ToString(it->second.AsInt64());
                } else if (it->second.IsUint64()) {
                    value = ToString(it->second.AsUint64());
                }
                buffer.Reserve(CgiEscapeBufLen(value.size()));
                CgiEscape(buffer.Data(), TStringBuf(value.data(), value.size()));
                if (it->first == "reqid") {
                    resultRow["reqid"] = value;
                } else {
                    request << it->first << "=" << TString(buffer.Data(), strlen(buffer.Data()));
                }
            }
            resultRow["prepared_request"] = request;
            output->AddRow(resultRow);
        }
    }
};

REGISTER_MAPPER(TSimpleLogMapper);

template<>
inline void Save(IOutputStream* out, const NJson::TJsonValue& t) {
    ::Save(out, ToString(t));
}

template<>
inline void Load(IInputStream* in, NJson::TJsonValue& t) {
    TString s;
    ::Load(in, s);
    NJson::ReadJsonFastTree(s, &t, true);
}

