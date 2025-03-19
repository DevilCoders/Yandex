#include <cloud/filestore/libs/diagnostics/events/profile_events.ev.pb.h>
#include <cloud/filestore/tools/analytics/libs/event-log/dump.h>

#include <library/cpp/eventlog/dumper/evlogdump.h>

using namespace NCloud::NFileStore;

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

                for (const auto i: order) {
                    DumpRequest(*message, i, out);
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
