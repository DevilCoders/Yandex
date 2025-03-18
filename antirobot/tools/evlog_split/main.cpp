#include <antirobot/tools/evlog_split/evlog.pb.h>

#include <antirobot/tools/evlogdump/lib/evlog_descriptors.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>
#include <mapreduce/yt/util/temp_table.h>
#include <mapreduce/yt/util/ypath_join.h>

#include <security/ant-secret/snooper/cpp/snooper.h>

#include <google/protobuf/dynamic_message.h>

#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/zip.h>

#include <library/cpp/protobuf/yql/descriptor.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


struct TArgs {
    TString Proxy;
    TString CompressionCodec;
    TString OptimizeFor;
    TString ErasureCodec;
    TString MapperErasureCodec;
    TString AutoMerge;
    bool Merge = false;
    bool ForceTransform = false;
    TString Input;
    TVector<TString> Outputs;

    static TArgs Parse(int argc, char* argv[]) {
        NLastGetopt::TOpts opts;
        TArgs args;

        opts.SetTitle("split antirobot evlog into multiple tables");
        opts.SetFreeArgsMax(0);
        opts.AddHelpOption();

        opts.AddLongOption("proxy", "YT proxy")
            .RequiredArgument("YT_PROXY")
            .DefaultValue("hahn")
            .StoreResult(&args.Proxy);

        opts.AddLongOption("compression-codec", "YT compression codec")
            .RequiredArgument("COMPRESSION_CODEC")
            .DefaultValue("zstd_5")
            .StoreResult(&args.CompressionCodec);

        opts.AddLongOption("optimize-for", "YT optimize for scan/lookup")
            .RequiredArgument("OPTIMIZE_FOR")
            .DefaultValue("scan")
            .StoreResult(&args.OptimizeFor);

        opts.AddLongOption("erasure-codec", "YT erasure codec")
            .RequiredArgument("ERASURE_CODEC")
            .DefaultValue("none")
            .StoreResult(&args.ErasureCodec);

        opts.AddLongOption(
            "mapper-erasure-codec",
            "YT erasure codec to be used during the intermediate map stage"
        )
            .RequiredArgument("MAPPER_ERASURE_CODEC")
            .DefaultValue("none")
            .StoreResult(&args.MapperErasureCodec);

        opts.AddLongOption("auto-merge", "YT auto-merge mode")
            .RequiredArgument("AUTO_MERGE_MODE")
            .DefaultValue("relaxed")
            .StoreResult(&args.AutoMerge);

        opts.AddLongOption("merge", "run an YT merge")
            .NoArgument()
            .SetFlag(&args.Merge);

        opts.AddLongOption("force-transform", "enable force_transform during the merge stage")
            .NoArgument()
            .SetFlag(&args.ForceTransform);

        opts.AddLongOption("input", "input table path")
            .Required()
            .RequiredArgument("INPUT_PATH")
            .StoreResult(&args.Input);

        opts.AddLongOption("output", "output table path (multiple allowed, format: TYPE:PATH)")
            .Required()
            .RequiredArgument("TYPE:PATH")
            .AppendTo(&args.Outputs);

        NLastGetopt::TOptsParseResult parseResult(&opts, argc, argv);
        Y_UNUSED(parseResult);

        return args;
    }
};

class TEvlogSplitMapper : public NYT::IMapper<
    NYT::TTableReader<NAntiRobot::TEvlogRow>,
    NYT::TTableWriter<NProtoBuf::Message>
> {
public:
    TEvlogSplitMapper()
        : Searcher(Snooper.Searcher())
    {}

    explicit TEvlogSplitMapper(TVector<TString> typeNames)
        : TypeNames(std::move(typeNames))
    {}

    void Start(TWriter* output) override {
        Y_UNUSED(output);

        const auto& evlogTypeNameToDescriptor = NAntiRobot::GetEvlogTypeNameToDescriptor();
        const auto messageFactory = NProtoBuf::DynamicMessageFactory::generated_factory();

        for (const auto& [tableIndex, typeName] : Enumerate(TypeNames)) {
            const auto descriptor = evlogTypeNameToDescriptor.at(typeName);
            const ui32 messageId = descriptor->options().GetExtension(message_id);

            MessageIdToPrototype[messageId] = {
                .Prototype = messageFactory->GetPrototype(descriptor),
                .HeaderDescriptor = descriptor->FindFieldByName("Header"),
                .DataDescriptor = descriptor->FindFieldByName("Data"),
                .TableIndex = tableIndex
            };
        }
    }

    void Do(TReader* input, TWriter* output) override {
        for (const auto& cursor : *input) {
            const auto& row = cursor.GetRow();

            const auto& eventBytes = row.event();
            const NAntiRobot::TEvlogEvent event(eventBytes);

            const auto data = MessageIdToPrototype.FindPtr(event.MessageId);

            if (!data) {
                continue;
            }

            THolder<NProtoBuf::Message> message{data->Prototype->New()};

            Y_ENSURE(
                message->ParseFromArray(event.MessageBytes.data(), event.MessageBytes.size()),
                "Failed to parse message"
            );

            if (data->DataDescriptor && data->DataDescriptor->type() == NProtoBuf::FieldDescriptor::TYPE_STRING) {
                const auto reflection = message->GetReflection();

                TString messageData = reflection->GetString(*message, data->DataDescriptor);
                Searcher->Mask(messageData);

                reflection->SetString(&*message, data->DataDescriptor, messageData);
            }

            if (data->HeaderDescriptor) {
                const auto reflection = message->GetReflection();
                const auto header = static_cast<NAntirobotEvClass::THeader*>(
                    reflection->MutableMessage(&*message, data->HeaderDescriptor)
                );

                header->SetTimestamp(event.Timestamp);
                header->SetSourceUri(row.source_uri());
            }

            output->AddRow(*message, data->TableIndex);
        }
    }

    Y_SAVELOAD_JOB(TypeNames);

private:
    struct TMessageData {
        const NProtoBuf::Message* Prototype = nullptr;
        const NProtoBuf::FieldDescriptor* HeaderDescriptor = nullptr;
        const NProtoBuf::FieldDescriptor* DataDescriptor = nullptr;
        size_t TableIndex = 0;
    };

    TVector<TString> TypeNames;
    THashMap<ui32, TMessageData> MessageIdToPrototype;

    NSnooper::TSnooper Snooper;
    THolder<NSnooper::TSearcher> Searcher;
};

REGISTER_MAPPER(TEvlogSplitMapper);


int main(int argc, char* argv[]) {
    NYT::Initialize(argc, argv);

    const auto args = TArgs::Parse(argc, argv);
    const auto client = NYT::CreateClient(args.Proxy);
    const auto tx = client->StartTransaction();

    auto spec = NYT::TMapOperationSpec()
        .AddInput<NAntiRobot::TEvlogRow>(args.Input);

    TVector<const NProtoBuf::Descriptor*> descriptors;
    TVector<TString> outputPaths;
    TVector<TString> typeNames;

    const auto optimizeFor = FromString<NYT::EOptimizeForAttr>(args.OptimizeFor);
    const auto erasureCodec = FromString<NYT::EErasureCodecAttr>(args.ErasureCodec);
    const auto mapperErasureCodec = FromString<NYT::EErasureCodecAttr>(args.MapperErasureCodec);

    for (const auto& output : args.Outputs) {
        TStringBuf typeName, path;

        Y_ENSURE(
            TStringBuf(output).TrySplit(':', typeName, path),
            "Invalid output: " << output
        );

        const auto descriptorIt = NAntiRobot::GetEvlogTypeNameToDescriptor().find(typeName);

        Y_ENSURE(
            descriptorIt != NAntiRobot::GetEvlogTypeNameToDescriptor().end(),
            "Unknown type name: " << typeName
        );

        descriptors.push_back(descriptorIt->second);

        const TString sPath(path);
        outputPaths.push_back(sPath);

        auto richPath = NYT::TRichYPath(sPath)
            .Schema(NYT::CreateTableSchema(*descriptorIt->second))
            .CompressionCodec(args.CompressionCodec)
            .ErasureCodec(mapperErasureCodec)
            .OptimizeFor(optimizeFor);

        spec.AddStructuredOutput(NYT::TStructuredTablePath(std::move(richPath), descriptorIt->second));
        typeNames.emplace_back(typeName);
    }

    const auto options = NYT::TOperationOptions()
        .Spec(NYT::TNode()("auto_merge", NYT::TNode()("mode", args.AutoMerge)));

    tx->Map(spec, new TEvlogSplitMapper(typeNames), options);

    for (const auto& [descriptor, outputPath] : Zip(descriptors, outputPaths)) {
        for (int fieldNum = 0; fieldNum < descriptor->field_count(); ++fieldNum) {
            const auto fieldDescriptor = descriptor->field(fieldNum);

            // Non-message and SERIALIZATION_YT types are already automatically parsed.

            if (fieldDescriptor->type() != NProtoBuf::FieldDescriptor::TYPE_MESSAGE) {
                continue;
            }

            if (
                const auto ytFlags = fieldDescriptor->options().GetRepeatedExtension(NYT::flags);
                Find(ytFlags, NYT::EWrapperFieldFlag_Enum_SERIALIZATION_YT) != ytFlags.end()
            ) {
                continue;
            }

            tx->Set(
                NYT::JoinYPaths(outputPath, "@_yql_proto_field_" + fieldDescriptor->name()),
                GenerateProtobufTypeConfig(fieldDescriptor->message_type())
            );
        }
    }

    if (args.Merge) {
        NYT::TOperationTracker operationTracker;

        for (const auto& outputPath : outputPaths) {
            operationTracker.AddOperation(tx->Merge(
                NYT::TMergeOperationSpec()
                    .AddInput(outputPath)
                    .Output(NYT::TRichYPath(outputPath)
                        .CompressionCodec(args.CompressionCodec)
                        .ErasureCodec(erasureCodec)
                    )
                    .ForceTransform(args.ForceTransform)
                    .CombineChunks(true),
                NYT::TOperationOptions().Wait(false)
            ));
        }

        operationTracker.WaitAllCompleted();
    }

    tx->Commit();
}
