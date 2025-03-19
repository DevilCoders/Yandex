#include "command.h"

namespace NCloud::NFileStore::NClient {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TDescribeCommand final
    : public TFileStoreCommand
{
public:
    TDescribeCommand() = default;

    bool Execute() override
    {
        auto callContext = PrepareCallContext();

        auto request = std::make_shared<NProto::TGetFileStoreInfoRequest>();
        request->SetFileSystemId(FileSystemId);

        auto response = WaitFor(
            Client->GetFileStoreInfo(
                std::move(callContext),
                std::move(request)));

        if (HasError(response)) {
            ythrow TServiceError(response.GetError());
        }

        STORAGE_INFO("Filestore: " << response.DebugString());

        return true;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TCommandPtr NewDescribeCommand()
{
    return std::make_shared<TDescribeCommand>();
}

}   // namespace NCloud::NFileStore::NClient
