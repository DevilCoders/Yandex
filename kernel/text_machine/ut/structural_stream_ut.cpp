#include <kernel/text_machine/parts/common/structural_stream.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/xrange.h>

using namespace NTextMachine;
using namespace NTextMachine::NCore;

using TIntStream = TStructuralStream<int>;

struct TInt {
    int Value;

    TInt() = delete;
    explicit TInt(int value)
        : Value(value)
    {}
};

struct TIntStreamNode {
     TVector<TIntStreamNode> Children;
     int Handle = -1;
     int Value = 0;

     TIntStreamNode(TInt value)
         : Value(value.Value)
     {}
     TIntStreamNode(int handle, std::initializer_list<TIntStreamNode> list)
         : Children(list.begin(), list.end())
         , Handle(handle)
     {
     }
};

struct TRootIntStreamNode {
     TVector<TIntStreamNode> Children;

     TRootIntStreamNode(std::initializer_list<TIntStreamNode> list)
         : Children(list.begin(), list.end())
     {
     }
};

static void CreateStreamHelper(TIntStream::TWriter& writer, const TIntStreamNode& node) {
    if (node.Handle < 0) {
        Cdbg << "[" << node.Value << "] ";
        writer.Next() = node.Value;
        return;
    }

    Cdbg << "(" << node.Handle << " ";
    auto guard = writer.Guard(node.Handle);
    for (const auto& child : node.Children) {
        CreateStreamHelper(writer, child);
    }
    Cdbg << ") ";
}

static void CreateStream(TIntStream& stream, const TRootIntStreamNode& node) {
    Cdbg << "WRITE ";
    auto writer = stream.CreateWriter();
    for (const auto& child : node.Children) {
        CreateStreamHelper(writer, child);
    }
    Cdbg << Endl;
}

static void VerifyStreamHelper(TIntStream::TReader& reader, const TIntStreamNode& node) {
    if (node.Handle < 0) {
        Cdbg << "[" << node.Value << "] ";
        UNIT_ASSERT_EQUAL(reader.Next(), node.Value);
        return;
    }

    Cdbg << "(" << node.Handle << " ";
    auto guard = reader.Guard(node.Handle);
    for (const auto& child : node.Children) {
        VerifyStreamHelper(reader, child);
    }
    Cdbg << ")";
}

static void VerifyStream(const TIntStream& stream, const TRootIntStreamNode& node) {
    Cdbg << "READ ";
    auto reader = stream.CreateReader();
    for (const auto& child : node.Children) {
        VerifyStreamHelper(reader, child);
    }
    Cdbg << Endl;
}

static void TestStream(const TRootIntStreamNode& node) {
    TIntStream stream;
    CreateStream(stream, node);
    VerifyStream(stream, node);

    TIntStream streamCopy;
    auto writer = streamCopy.CreateWriter();
    stream.Copy(writer);

    VerifyStream(streamCopy, node);
}

Y_UNIT_TEST_SUITE(TStructuralStreamTest) {
    Y_UNIT_TEST(TestEmpty) {
        TIntStream stream;
        UNIT_ASSERT_EQUAL(stream.NumItems(), 0);

        size_t count = 0;
        for (int value : stream) { count += 1; Y_UNUSED(value); }
        UNIT_ASSERT_EQUAL(count, 0);
    }

    Y_UNIT_TEST(TestOneItem) {
        TIntStream stream;

        auto writer = stream.CreateWriter();
        writer.Next() = 42;

        UNIT_ASSERT_EQUAL(stream.NumItems(), 1);

        auto reader = stream.CreateReader();
        UNIT_ASSERT_EQUAL(reader.Next(), 42);

        size_t count = 0;
        size_t sum = 0;
        for (int value : stream) { count += 1; sum += value; }
        UNIT_ASSERT_EQUAL(count, 1);
        UNIT_ASSERT_EQUAL(sum, 42);
    }

    Y_UNIT_TEST(TestChildren) {
        TIntStream stream;

        CreateStream(stream, {{1, {TInt(11), {2, {TInt(31)}}}}});

        UNIT_ASSERT_EQUAL(stream.NumItems(), 2);

        VerifyStream(stream, {{1, {TInt(11), {2, {TInt(31)}}}}});

        size_t count = 0;
        size_t sum = 0;
        for (int value : stream) { count += 1; sum += value; }
        UNIT_ASSERT_EQUAL(count, 2);
        UNIT_ASSERT_EQUAL(sum, 42);
    }

    Y_UNIT_TEST(TestSmallStreams) {
        TestStream({});
        TestStream({TInt(0)});
        TestStream({{1, {}}});
        TestStream({TInt(0), {1, {}}});
        TestStream({{1, {}}, TInt(0)});
        TestStream({{1, {}}, {2, {}}});
        TestStream({TInt(0), TInt(1)});
        TestStream({{1, {{2, {{3, {}}}}}}});
    }

    Y_UNIT_TEST(TestDeepStream) {
        TRootIntStreamNode node = {{0, {}}};
        TIntStreamNode *cur = &node.Children[0];
        for (int i : xrange<int>(1, 1024)) {
            cur->Children.push_back({i, {}});
            cur = &cur->Children[0];
        }

        TestStream(node);
    }

    Y_UNIT_TEST(TestTraversal) {
        TRootIntStreamNode node = {
            {0, {TInt(3), TInt(5)}},
            {1, {TInt(7), {2, {TInt(11)}}, TInt(13)}},
            {3, {TInt(17), TInt(19)}}
        };

        TIntStream stream;
        CreateStream(stream, node);

        auto reader = stream.CreateReader();

        {
            auto guard0 = reader.Guard(0);

            UNIT_ASSERT_EQUAL(reader.Next(), 3);
            UNIT_ASSERT_EQUAL(reader.Next(), 5);
        }

        {
            auto guard1 = reader.Guard(1);
        }

        {
            auto guard2 = reader.Guard(3);

            UNIT_ASSERT_EQUAL(reader.Next(), 17);
            UNIT_ASSERT_EQUAL(reader.Next(), 19);

            guard2.VisitChild();

            UNIT_ASSERT_EQUAL(reader.Next(), 17);
            UNIT_ASSERT_EQUAL(reader.Next(), 19);
        }
    }
};
