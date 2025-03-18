#include <library/cpp/eventlog/ut/test_events.ev.pb.h>

#include <library/cpp/threading/future/async.h>
#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/eventlog/evdecoder.h>
#include <library/cpp/eventlog/iterator.h>
#include <library/cpp/eventlog/threaded_eventlog.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/thread/pool.h>
#include <util/string/cast.h>
#include <util/stream/file.h>
#include <util/system/fs.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/tempfile.h>

const size_t WORKER_COUNT = 32;
const size_t THREAD_COUNT = WORKER_COUNT;
const size_t EVENT_COUNT = 1000;

using namespace NEventLog;

Y_UNIT_TEST_SUITE(ReopenEventlog) {
    Y_UNIT_TEST(SimpleTest) {
        auto tempFile = TTempFile("event.log");
        auto tempFileName = tempFile.Name();

        {
            TEventLog eventLog(tempFileName, NEvClass::Factory()->CurrentFormat());
            TThreadedEventLog threadedEventLog(eventLog);

            TThreadPool threadPool;
            threadPool.Start(THREAD_COUNT);

            for (size_t iter = 0; iter < WORKER_COUNT; ++iter) {
                auto worker = [&, iter]{
                    for (size_t i = iter * EVENT_COUNT; i < (iter + 1) * EVENT_COUNT; ++i) {
                        {
                            TSelfFlushLogFrame frame(threadedEventLog);
                            frame.LogEvent(TOneField(ToString(i)));
                        }

                        if (i % 100 == 0) {
                            threadedEventLog.ReopenLog();
                        }
                    }
                };

                NThreading::Async(std::move(worker), threadPool);
            }

            threadPool.Stop();
        }

        size_t writtenCounter = 0;
        TAutoPtr<IIterator> eventlog = CreateIterator(TOptions().SetFileName(tempFileName));
        for (IIterator::TIterator event = eventlog->begin(); event != eventlog->end(); ++event) {
            if(event->Class == TOneField::ID) {
                writtenCounter++;
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(WORKER_COUNT * EVENT_COUNT, writtenCounter);
    }
}
