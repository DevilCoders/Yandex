#include <cloud/blockstore/libs/diagnostics/events/profile_events.ev.pb.h>
#include <cloud/blockstore/tools/analytics/libs/event-log/dump.h>

#include <library/cpp/eventlog/dumper/evlogdump.h>

using namespace NCloud::NBlockStore;

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char** argv)
{
    struct TEventProcessor
        : TProtobufEventProcessor
    {
        void DoProcessEvent(const TEvent* ev, IOutputStream* out) override
        {
            auto* message =
                dynamic_cast<const NProto::TProfileLogRecord*>(ev->GetProto());
            if (message) {
                auto order = GetItemOrder(*message);

                for (const auto& i: order) {
                    switch (i.Type) {
                        case EItemType::Request: {
                            DumpRequest(*message, i.Index, out);
                            break;
                        }
                        case EItemType::BlockInfo: {
                            DumpBlockInfoList(*message, i.Index, out);
                            break;
                        }
                        case EItemType::BlockCommitId: {
                            DumpBlockCommitIdList(*message, i.Index, out);
                            break;
                        }
                        case EItemType::BlobUpdate: {
                            DumpBlobUpdateList(*message, i.Index, out);
                            break;
                        }
                        default: {
                            Y_FAIL("unknown item");
                        }
                    }
                }
            }
        }
    } processor;

    return IterateEventLog(
        NEvClass::Factory(),
        &processor,
        argc,
        argv
    );
}
