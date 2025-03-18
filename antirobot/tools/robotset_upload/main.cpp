#include <antirobot/tools/robotset_upload/proto/args.pb.h>

#include <antirobot/daemon_lib/req_types.h>
#include <antirobot/daemon_lib/uid.h>

#include <antirobot/idl/cache_sync.pb.h>

#include <mapreduce/yt/interface/client.h>

#include <google/protobuf/messagext.h>

#include <library/cpp/getoptpb/getoptpb.h>

#include <util/stream/file.h>


int main(int argc, const char* argv[]) {
    NYT::JoblessInitialize();

    const NAntirobot::TRobotSetArgs args = NGetoptPb::GetoptPbOrAbort(
        argc, argv,
        {.DumpConfig = false}
    );

    TFileInput in(args.GetInput());
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);

    NAntiRobot::NCacheSyncProto::TBanAction action;

    const auto client = NYT::CreateClient(args.GetCluster());
    const auto writer = client->CreateTableWriter<NAntiRobot::NCacheSyncProto::TBanAction>(
        NYT::TRichYPath(args.GetOutput())
            .Schema(NYT::CreateTableSchema<NAntiRobot::NCacheSyncProto::TBanAction>())
    );

    while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&action, &adaptor)) {
        action.DiscardUnknownFields();
        writer->AddRow(action);
    }

    return 0;
}
