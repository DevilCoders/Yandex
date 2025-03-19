#include "command.h"

#include <cloud/filestore/public/api/protos/fs.pb.h>

#include <library/cpp/protobuf/json/proto2json.h>

namespace NCloud::NFileStore::NClient {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TLsCommand final
    : public TFileStoreCommand
{
private:
    TString Path;

public:
    TLsCommand()
    {
        Opts.AddLongOption("path")
            .Required()
            .RequiredArgument("PATH")
            .StoreResult(&Path);
    }

    bool Execute() override
    {
        CreateSession();

        const auto node = ResolvePath(Path, false).back().Node;

        auto printNode = [] (const auto& node) {
            NProtobufJson::TProto2JsonConfig config;
            config.SetFormatOutput(false);

            NProtobufJson::Proto2Json(
                node,
                Cout,
                config
            );

            Cout << Endl;
        };

        if (node.GetType() != NProto::E_DIRECTORY_NODE) {
            printNode(node);
            return true;
        }

        TString cookie;

        while (true) {
            auto request = CreateRequest<NProto::TListNodesRequest>();
            request->SetNodeId(node.GetId());
            request->SetCookie(cookie);

            auto response = WaitFor(Client->ListNodes(
                PrepareCallContext(),
                std::move(request)));

            CheckResponse(response);

            const auto& names = response.GetNames();
            const auto& nodes = response.GetNodes();
            Y_ENSURE(
                names.size() == nodes.size(),
                "names/nodes sizes don't match"
            );

            for (int i = 0; i < names.size(); ++i) {
                Cout << names[i] << "\t";

                printNode(nodes[i]);
            }

            cookie = response.GetCookie();
            if (!cookie) {
                break;
            }
        }

        return true;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TCommandPtr NewLsCommand()
{
    return std::make_shared<TLsCommand>();
}

}   // namespace NCloud::NFileStore::NClient
