#include "command.h"

#include <cloud/filestore/public/api/protos/fs.pb.h>

#include <util/generic/size_literals.h>

namespace NCloud::NFileStore::NClient {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TCreateCommand final
    : public TFileStoreCommand
{
private:
    TString CloudId;
    TString FolderId;
    ui32 BlockSize = 4_KB;
    ui64 BlocksCount = 0;
    NCloud::NProto::EStorageMediaKind StorageMediaKind =
        NCloud::NProto::STORAGE_MEDIA_HDD;
    TString StorageMediaKindArg;

public:
    TCreateCommand()
    {
        Opts.AddLongOption("cloud")
            .Required()
            .RequiredArgument("STR")
            .StoreResult(&CloudId);

        Opts.AddLongOption("folder")
            .Required()
            .RequiredArgument("STR")
            .StoreResult(&FolderId);

        Opts.AddLongOption("block-size")
            .RequiredArgument("NUM")
            .StoreResult(&BlockSize);

        Opts.AddLongOption("blocks-count")
            .Required()
            .RequiredArgument("NUM")
            .StoreResult(&BlocksCount);

        Opts.AddLongOption("storage-media-kind")
            .RequiredArgument("STR")
            .StoreResult(&StorageMediaKindArg);
    }

    bool Execute() override
    {
        auto callContext = PrepareCallContext();

        auto request = std::make_shared<NProto::TCreateFileStoreRequest>();
        request->SetFileSystemId(FileSystemId);
        request->SetCloudId(CloudId);
        request->SetFolderId(FolderId);
        request->SetBlockSize(BlockSize);
        request->SetBlocksCount(BlocksCount);

        if (StorageMediaKindArg == "ssd") {
            StorageMediaKind = NCloud::NProto::STORAGE_MEDIA_SSD;
        } else if (StorageMediaKindArg == "hdd") {
            StorageMediaKind = NCloud::NProto::STORAGE_MEDIA_HDD;
        } else if (StorageMediaKindArg == "hybrid") {
            StorageMediaKind = NCloud::NProto::STORAGE_MEDIA_HYBRID;
        } else if (StorageMediaKindArg) {
            ythrow yexception() << "invalid storage media kind: "
                << StorageMediaKindArg
                << ", should be one of 'ssd', 'hdd', 'hybrid'";
        }

        request->SetStorageMediaKind(StorageMediaKind);

        auto response = WaitFor(
            Client->CreateFileStore(
                std::move(callContext),
                std::move(request)));

        if (HasError(response)) {
            ythrow TServiceError(response.GetError());
        }

        return true;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TCommandPtr NewCreateCommand()
{
    return std::make_shared<TCreateCommand>();
}

}   // namespace NCloud::NFileStore::NClient
