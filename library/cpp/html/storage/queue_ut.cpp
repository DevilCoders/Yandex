#include "queue.h"
#include "storage.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NHtml;

Y_UNIT_TEST_SUITE(TChunkQueueTest) {
    Y_UNIT_TEST(NextChunk) {
        THtmlChunk c1(PARSED_MARKUP);
        THtmlChunk c2(PARSED_TEXT);
        THtmlChunk c3(PARSED_MARKUP);

        TStorage storage;

        const THtmlChunk* first = storage.Push(c1);
        storage.Push(c2);
        storage.Push(c3);

        TSegmentedQueueIterator it = storage.Begin();
        UNIT_ASSERT(c1.flags.type == it->flags.type);
        ++it;
        UNIT_ASSERT(c2.flags.type == it->flags.type);
        ++it;
        UNIT_ASSERT(c3.flags.type == it->flags.type);
        ++it;
        UNIT_ASSERT(it == storage.End());

        UNIT_ASSERT(c1.flags.type == first->flags.type);
        first = GetNextChunk(first);
        UNIT_ASSERT(c2.flags.type == first->flags.type);
        first = GetNextChunk(first);
        UNIT_ASSERT(c3.flags.type == first->flags.type);
        first = GetNextChunk(first);
        UNIT_ASSERT(first == nullptr);
    }
}
