#include <kernel/proto_fuzz/fuzzer.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/diff/diff.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/tokenizer.h>

#include <kernel/proto_fuzz/ut/test.pb.h>

using namespace NProtoFuzz;

const ui64 seed = 11;

TString ToString(const NProtoBuf::Message& msg) {
    return msg.ShortDebugString();
}

template <typename ProtoType>
ProtoType ToProto(const TString& text)
{
    using TF = ::google::protobuf::TextFormat;
    using TProto = ProtoType;

    TF::Parser parser;
    TProto proto;
    parser.ParseFromString(text, &proto);

    return proto;
}

void FuzzProb(NProtoBuf::Message& msg, ui64 seed, double prob = 1.0) {
    TFuzzer::TOptions opts(seed);
    opts.FuzzRatio = prob;
    opts.FuzzStructureRatio = prob;
    TFuzzer{opts}(msg);
}

size_t GetTreeDepth(const TBasicNode& node) {
    if (node.NodesSize() == 0) {
        return 0;
    }
    size_t depth = 0;
    for (const auto& subNode : node.GetNodes()) {
        depth = Max(depth, GetTreeDepth(subNode));
    }
    return depth + 1;
}

size_t GetTreeDepth(const TNode& node) {
    if (node.NodesSize() == 0 && !node.HasOneNode()) {
        return 0;
    }
    size_t depth = 0;
    for (const auto& subNode : node.GetNodes()) {
        depth = Max(depth, GetTreeDepth(subNode));
    }
    if (node.HasOneNode()) {
        depth = Max(depth, GetTreeDepth(node.GetOneNode()));
    }
    return depth + 1;
}

template <typename NodeType>
size_t GetTreeFanOut(const NodeType& node) {
    size_t fanOut = node.NodesSize();
    for (const auto& subNode : node.GetNodes()) {
        fanOut = Max(fanOut, GetTreeFanOut(subNode));
    }
    return fanOut;
}

TString GetInlineDiff(const TStringBuf& x, const TStringBuf& y) {
    TVector<NDiff::TChunk<char>> chunks;
    NDiff::InlineDiff(chunks, x, y);

    TStringBuilder out;

    bool hasDiff = false;

    for (const auto& chunk : chunks) {
        if (chunk.Left.empty() && chunk.Right.empty()) {
            out << TStringBuf(chunk.Common.data(), chunk.Common.size());
        } else {
            hasDiff = true;

            out << "((" << TStringBuf(chunk.Left.data(), chunk.Left.size())
                << "//" << TStringBuf(chunk.Right.data(), chunk.Right.size())
                << "))" << TStringBuf(chunk.Common.data(), chunk.Common.size());
        }
    }
    out << Endl;

    return hasDiff ? TString(out) : TString("no diffs");
}

Y_UNIT_TEST_SUITE(FuzzerTest) {
    template <typename TMsg>
    void CheckFuzzSimple(size_t count, bool printDiffs = false) {
        TMsg msg;
        Cdbg << "BEFORE = (" << ToString(msg) << ")" << Endl;

        TFastRng64 rng(seed);
        for (size_t i : xrange(count)) {
            TMsg prevMsg = msg;
            FuzzProb(msg, rng.GenRand64());

            if (printDiffs) {
                Cdbg << "DIFF[" << i << "] = (" << GetInlineDiff(ToString(prevMsg), ToString(msg)) << ")" << Endl;
            } else {
                Cdbg << "AFTER[" << i << "] = (" << ToString(msg) << ")" << Endl;
            }

            UNIT_ASSERT_UNEQUAL(ToString(prevMsg), ToString(msg));
        }

        size_t fuzzedCount = 0;
        for (size_t i : xrange(count)) {
            Y_UNUSED(i);

            TMsg prevMsg = msg;
            FuzzProb(msg, rng.GenRand64(), 0.1);

            if (printDiffs) {
                Cdbg << "DIFF[" << i << "] = (" << GetInlineDiff(ToString(prevMsg), ToString(msg)) << ")" << Endl;
            } else {
                Cdbg << "AFTER[" << i << "] = (" << ToString(msg) << ")" << Endl;
            }

            if (ToString(prevMsg) != ToString(msg)) {
                fuzzedCount += 1;
            }
        }

        Cdbg << "FUZZED_COUNT = " << fuzzedCount << Endl;
        UNIT_ASSERT(fuzzedCount >= count / 20);
    }

    Y_UNIT_TEST(TestEmpty) {
        TEmpty msg;
        Cdbg << "BEFORE = (" << ToString(msg) << ")" << Endl;
        FuzzProb(msg, seed);
        Cdbg << "AFTER = (" << ToString(msg) << ")" << Endl;
        UNIT_ASSERT_EQUAL(ToString(msg), ToString(TEmpty{}));
    }

    Y_UNIT_TEST(TestNoFuzz) {
        TNode msg;
        FuzzProb(msg, seed, 1.0);

        Cdbg << "BEFORE = (" << ToString(msg) << ")" << Endl;

        TNode prevMsg = msg;
        FuzzProb(msg, seed, 0.0);

        Cdbg << "AFTER = (" << ToString(msg) << ")" << Endl;

        UNIT_ASSERT_EQUAL(ToString(prevMsg), ToString(msg));
    }

    Y_UNIT_TEST(TestOneInt64) {
        CheckFuzzSimple<TOneInt64>(100);
    }

    Y_UNIT_TEST(TestOneInt32) {
        CheckFuzzSimple<TOneInt32>(100);
    }

    Y_UNIT_TEST(TestOneUInt64) {
        CheckFuzzSimple<TOneUInt64>(100);
    }

    Y_UNIT_TEST(TestOneUInt32) {
        CheckFuzzSimple<TOneUInt32>(100);
    }

    Y_UNIT_TEST(TestOneDouble) {
        CheckFuzzSimple<TOneDouble>(100);
    }

    Y_UNIT_TEST(TestOneFloat) {
        CheckFuzzSimple<TOneFloat>(100);
    }

    Y_UNIT_TEST(TestOneString) {
        CheckFuzzSimple<TOneString>(100);
    }

    Y_UNIT_TEST(TestOneEnum) {
        CheckFuzzSimple<TOneEnum>(100);
    }

    Y_UNIT_TEST(TestOneLevel) {
        CheckFuzzSimple<TValues>(100, true);
    }

    Y_UNIT_TEST(TestBasicMultiLevel) {
        TBasicNode msg;

        Cdbg << "BEFORE = (" << ToString(msg) << ")" << Endl;

        size_t maxDepth = 0;
        size_t maxFanOut = 0;

        TFastRng64 rng(seed);
        for (size_t i : xrange(200)) {
            TBasicNode prevMsg = msg;
            FuzzProb(msg, rng.GenRand64(), 1.0);

            Cdbg << "AFTER[" << i << "] = (" << ToString(msg) << ")" << Endl;

            const size_t depth = GetTreeDepth(msg);
            const size_t fanOut = GetTreeFanOut(msg);

            Cdbg << "DEPTH[" << i << "] = " << depth << Endl;
            Cdbg << "FANOUT[" << i << "] = " << fanOut << Endl;

            UNIT_ASSERT_UNEQUAL(ToString(prevMsg), ToString(msg));

            maxDepth = Max(depth, maxDepth);
            maxFanOut = Max(fanOut, maxFanOut);
        }

        Cdbg << "MAX_DEPTH = " << maxDepth << Endl;
        Cdbg << "MAX_FANOUT = " << maxFanOut << Endl;


        UNIT_ASSERT(maxDepth < 20); // structure does't grow too much
        UNIT_ASSERT(maxFanOut > 1);
    }

    Y_UNIT_TEST(TestMultiLevel) {
        TNode msg;

        Cdbg << "BEFORE = (" << ToString(msg) << ")" << Endl;

        size_t maxDepth = 0;

        TFastRng64 rng(seed);
        for (size_t i : xrange(100)) {
            TNode prevMsg = msg;
            FuzzProb(msg, rng.GenRand64(), 1.0);

            Cdbg << "AFTER[" << i << "] = (" << ToString(msg) << ")" << Endl;

            const size_t depth = GetTreeDepth(msg);

            Cdbg << "DEPTH[" << i << "] = " << depth << Endl;

            UNIT_ASSERT_UNEQUAL(ToString(prevMsg), ToString(msg));

            if (i == 0) {
                UNIT_ASSERT(depth > 0);
            }

            maxDepth = Max(depth, maxDepth);
        }

        Cdbg << "MAX_DEPTH = " << maxDepth << Endl;

        UNIT_ASSERT(maxDepth < 20); // structure does't grow too much
    }
}
