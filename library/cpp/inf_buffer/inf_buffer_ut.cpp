#include "inf_buffer.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/stream/str.h>
#include <util/string/split.h>
#include <util/thread/pool.h>


Y_UNIT_TEST_SUITE(InfBuffer) {
    Y_UNIT_TEST(MT) {
        TString data;

        static const size_t n = 19; // threads
        static const size_t length = 1'000'000;

        char alpha[n];
        Iota(std::begin(alpha), std::end(alpha), 'a');

        {
            THolder<IThreadPool> queue = CreateThreadPool(n, n, IThreadPool::TParams().SetBlocking(false).SetCatching(false));

            NInfBuffer::TInfBuffer buffer{MakeHolder<TStringOutput>(data)};

            for (size_t i = 0; i < n; ++i) {
                auto fn = [i, &buffer, &alpha]() {
                    for (size_t j = i; j < length; j += n) {
                        const TString sj = ToString(j);
                        const IOutputStream::TPart parts[4]{
                            IOutputStream::TPart(&alpha[i], 1),
                            IOutputStream::TPart(":", 1),
                            IOutputStream::TPart(sj.data(), sj.size()),
                            IOutputStream::TPart("\n", 1),
                        };
                        buffer.Write(parts, std::size(parts));
                    }
                };
                queue->SafeAddFunc(fn);
            }
            queue->Stop();

            buffer.Finish();
        }

        if (length < 100) {
           Cerr << data << Endl;
        }

        TVector<TString> bufferLines = StringSplitter(data).Split('\n').SkipEmpty();
        Sort(bufferLines);

        TVector<TString> refLines(Reserve(length));
        for (size_t i = 0; i < length; ++i) {
            refLines.push_back(TString::Join(alpha[i % n], ':', ToString(i)));
        }
        Sort(refLines);

        UNIT_ASSERT_VALUES_EQUAL(bufferLines.size(), refLines.size());
        for (size_t i = 0; i < length; ++i) {
            UNIT_ASSERT_VALUES_EQUAL_C(bufferLines[i], refLines[i], i);
        }
    }
}
