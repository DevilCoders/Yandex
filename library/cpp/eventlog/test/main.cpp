#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/eventlog/iterator.h>

#include <library/cpp/eventlog/test/events.ev.pb.h>

using namespace NTest;

static inline void CreateLog(const TString& path) {
    TEventLog log(path, NEvClass::Factory()->CurrentFormat());

    for (size_t i = 0; i < 100; ++i) {
        TEventLogFrame frame(log);

        {
            TEvent1 ev;

            ev.SetIter(i);
            frame.LogEvent(ev);
        }

        for (size_t j = 0; j < 100; ++j) {
            TEvent2 ev;

            ev.SetIter(i);
            ev.SetSubIter(j);

            frame.LogEvent(ev);
        }

        frame.Flush();
    }

    log.CloseLog();
}

static inline void DumpEventLog(const TString& path) {
    using namespace NEventLog;

    TAutoPtr<IIterator> eventlog = CreateIterator(TOptions().SetFileName(path));

    for (IIterator::TIterator event = eventlog->begin(); event != eventlog->end(); ++event) {
        Cout << "timestamp = " << event->Timestamp << " class = " << event->Class << " typename = " << event->GetProto()->GetTypeName() << "\n"
             << event->GetProto()->DebugString() << Endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        Cerr << "usage: " << argv[0] << " logfile" << Endl;

        return 1;
    }

    CreateLog(argv[1]);
    DumpEventLog(argv[1]);
}
