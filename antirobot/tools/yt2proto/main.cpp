#include <antirobot/idl/cache_sync.pb.h>
#include <antirobot/idl/factors.pb.h>
#include <antirobot/lib/mini_geobase.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/io/lenval_table_reader.h>

#include <google/protobuf/messagext.h>

#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/protobuf/yt/yt2proto.h>

#include <util/digest/city.h>
#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/length.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/fs.h>
#include <util/system/tempfile.h>

#include <type_traits>
#include <utility>


struct TArgs {
    TString Proxy;
    TString Input;
    TString Type;
    TString Output;
    TVector<TString> RenameColumns;
    TString KeyHash;
    TString RowFunc;

    static TArgs Parse(int argc, char* argv[]) {
        NLastGetopt::TOpts opts;
        TArgs args;

        opts.SetTitle("convert YT table to protobuf");
        opts.SetFreeArgsMax(0);
        opts.AddHelpOption();

        opts.AddLongOption("proxy", "YT proxy")
            .RequiredArgument("YT_PROXY")
            .DefaultValue("hahn")
            .StoreResult(&args.Proxy);

        opts.AddLongOption("input", "input table path")
            .Required()
            .RequiredArgument("INPUT_PATH")
            .StoreResult(&args.Input);

        opts.AddLongOption("type", "protobuf type name")
            .Required()
            .RequiredArgument("TYPE_NAME")
            .StoreResult(&args.Type);

        opts.AddLongOption("output", "output file path")
            .Required()
            .RequiredArgument("OUTPUT_PATH")
            .StoreResult(&args.Output);

        opts.AddLongOption("rename-column", "rename column SRC to DST")
            .RequiredArgument("SRC:DST")
            .AppendTo(&args.RenameColumns);

        opts.AddLongOption("key-hash", "replace keys with hash HASH_FUNCTION")
            .RequiredArgument("HASH_FUNCTION")
            .DefaultValue("none")
            .StoreResult(&args.KeyHash);

        opts.AddLongOption("row-func", "process row with FUNCTION")
            .RequiredArgument("FUNCTION")
            .DefaultValue("none")
            .StoreResult(&args.RowFunc);

        NLastGetopt::TOptsParseResult parseResult(&opts, argc, argv);
        Y_UNUSED(parseResult);
        Y_ENSURE(args.RowFunc == "none" || args.KeyHash == "none", "Usage of key-hash and row-func together is not allowed");

        return args;
    }
};

template<typename T>
void MiniGeobaseRow(const NYT::TNode&, T&) {
    ythrow yexception() << "You need to use TFixed64Record only";
}

template<>
void MiniGeobaseRow(const NYT::TNode& row, NAntiRobot::NFeaturesProto::TFixed64Record& record) {
    record.SetKey(row.ChildAsString("Key"));
    ui64 mask = 0;
    for (ui32 i = 0; i < static_cast<ui32>(NAntiRobot::TMiniGeobase::EIpType::Count); i++) {
        if (row.ChildAsBool(ToString(static_cast<NAntiRobot::TMiniGeobase::EIpType>(i)))) {
            mask |= 1ull << i;
        }
    }
    record.SetValue(mask);
}

template <typename T>
void Download(
    NYT::IClient* client,
    const TString& inputPath,
    const TString& typeName,
    const THashMap<TString, TString>& oldToNewName,
    const TString& keyHash,
    const TString& rowFunc,
    TFileOutput* output
) {
    static_assert(std::is_base_of_v<NProtoBuf::Message, T>);

    const auto reader = client->CreateTableReader<NYT::TNode>(inputPath);
    NProtoBuf::io::TCopyingOutputStreamAdaptor adaptor(output);

    NAntiRobot::NFeaturesProto::THeader header;
    header.SetHeader(typeName);
    header.SetNum(client->Get(inputPath + "/@row_count").AsInt64());

    Y_ENSURE(
        NProtoBuf::io::SerializeToZeroCopyStreamSeq(&header, &adaptor),
        "Failed to serialize"
    );

    ui64 (*keyHashFunc)(const TString&) = nullptr;
    void (*rowFuncFunc)(const NYT::TNode&, T&) = nullptr;

    if (keyHash == "cityhash64") {
        keyHashFunc = CityHash64;
    } else {
        Y_ENSURE(keyHash == "none", "Unknown hash function: " << keyHash);
    }

    if (rowFunc == "mini_geobase") {
        rowFuncFunc = MiniGeobaseRow;
    } else {
        Y_ENSURE(rowFunc == "none", "Unknown row function: " << rowFunc);
    }

    T pbRow;

    for (const auto& cursor : *reader) {
        NYT::TNode row;
        for (const auto& [key, value] : cursor.GetRow().AsMap()) {
            if (const auto newName = oldToNewName.FindPtr(key)) {
                row[*newName] = value;
            } else {
                row[key] = value;
            }
        }

        if (rowFuncFunc) {
            rowFuncFunc(row, pbRow);
        } else {
            if (keyHashFunc && row.HasKey("Key")) {
                row["Key"] = keyHashFunc(row.ChildAsString("Key"));
            }
            YtNodeToProto(row, pbRow, TParseConfig().SetCastRobust(true));
        }

        Y_ENSURE(
            NProtoBuf::io::SerializeToZeroCopyStreamSeq(&pbRow, &adaptor),
            "Failed to serialize"
        );
    }
}


template <typename T, typename... Ts>
void DownloadOneOf(
    NYT::IClient* client,
    const TString& inputPath,
    const TString& typeName,
    const THashMap<TString, TString>& oldToNewName,
    const TString& keyHash,
    const TString& rowFunc,
    TFileOutput* output
) {
    if (T::descriptor()->name() == typeName) {
        Download<T>(client, inputPath, typeName, oldToNewName, keyHash, rowFunc, output);
    } else if constexpr (sizeof...(Ts) > 0) {
        DownloadOneOf<Ts...>(client, inputPath, typeName, oldToNewName, keyHash, rowFunc, output);
    } else {
        Y_ENSURE(false, "Unknown type name");
    }
}


void Main(int argc, char* argv[]) {
    NYT::JoblessInitialize();

    const auto args = TArgs::Parse(argc, argv);
    const auto client = NYT::CreateClient(args.Proxy);

    THashMap<TString, TString> oldToNewName;

    for (const auto& renameColumn : args.RenameColumns) {
        TStringBuf src, dst;

        Y_ENSURE(
            TStringBuf(renameColumn).TrySplit(':', src, dst),
            "Invalid column renaming: " << renameColumn
        );

        oldToNewName[src] = dst;
    }

    const auto outputDirPath = TFsPath(args.Output).Dirname();
    TTempFileHandle tmpFile = TTempFileHandle::InDir(outputDirPath);

    {
        TFileOutput output(tmpFile);

        DownloadOneOf<
            // List of supported types.
            NAntiRobot::NFeaturesProto::TFloatRecord,
            NAntiRobot::NFeaturesProto::TStringRecord,
            NAntiRobot::NFeaturesProto::TFixed64Record,
            NAntiRobot::NFeaturesProto::TCityHash64FloatRecord,
            NAntiRobot::NFeaturesProto::TUidRecord,
            NAntiRobot::NFeaturesProto::TMarketJwsStatesStats,
            NAntiRobot::NFeaturesProto::TMarketStats
        >(
            &*client,
            args.Input,
            args.Type,
            oldToNewName,
            args.KeyHash,
            args.RowFunc,
            &output
        );
    }

    tmpFile.Close();

    Y_ENSURE(
        NFs::Rename(tmpFile.Name(), args.Output),
        "Failed to rename: " << LastSystemErrorText()
    );
}


int main(int argc, char* argv[]) {
    try {
        Main(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
