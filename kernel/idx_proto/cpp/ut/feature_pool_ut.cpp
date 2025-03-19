#include <kernel/idx_proto/cpp/feature_pool.h>

#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/unknown_field_set.h>

#include <util/stream/buffer.h>
#include <util/generic/buffer.h>

using namespace NFeaturePool;

Y_UNIT_TEST_SUITE(TFeaturePoolTest) {
    Y_UNIT_TEST(TestFiltrationType)
    {
        {
            TLine line;
            line.SetFiltrationType(FT_NOT_FILTERED);
            UNIT_ASSERT(!IsPoolLineFiltered(line));
        }

        {
            TLine line;
            line.SetFiltrationType(FT_DUPLICATE);
            UNIT_ASSERT(IsPoolLineFiltered(line));
        }

        {
            TLine line;
            UNIT_ASSERT(IsPoolLineFiltered(line));
        }

        {
            using namespace ::google::protobuf;

            TLine line;
            // Set FiltratrionType to value that can't be interpreted
            // by protobuf parser
            const Reflection* reflection = line.GetReflection();
            UnknownFieldSet* ufs = reflection->MutableUnknownFields(&line);
            ufs->AddVarint(TLine::kFiltrationTypeFieldNumber, EFiltrationType_MAX + 1);

            // Emulate "unknown field from wire" case
            TBuffer buf;
            TBufferOutput out(buf);
            line.SerializeToArcadiaStream(&out);

            line.Clear();
            TBufferInput in(buf);
            line.ParseFromArcadiaStream(&in);

            UNIT_ASSERT(!reflection->GetUnknownFields(line).empty());
            UNIT_ASSERT_EQUAL(line.GetFiltrationType(), FT_UNKNOWN);
            UNIT_ASSERT(IsPoolLineFiltered(line));
        }
    }

    Y_UNIT_TEST(TestPoolLineFromToTSV)
    {
        NFeaturePool::TLine protoLine;
        TString tsvLine = "1" "\t" "0.3" "\t" "www.host.com/page" "\t" "15" "\t" "1" "\t" "0.5" "\t" "0";
        FeaturesTSVLineToProto(tsvLine, protoLine);

        UNIT_ASSERT_EQUAL(protoLine.GetRequestId(), 1);
        UNIT_ASSERT_EQUAL(protoLine.GetRating(), 0.3f);
        UNIT_ASSERT_EQUAL(protoLine.GetMainRobotUrl(), "www.host.com/page");
        UNIT_ASSERT_EQUAL(protoLine.GetGrouping(), "15");
        UNIT_ASSERT_EQUAL(protoLine.FeatureSize(), 3);
        UNIT_ASSERT_EQUAL(protoLine.GetFeature(0), 1.0f);
        UNIT_ASSERT_EQUAL(protoLine.GetFeature(1), 0.5f);
        UNIT_ASSERT_EQUAL(protoLine.GetFeature(2), 0.0f);
        UNIT_ASSERT(!IsPoolLineFiltered(protoLine));

        TString tsvLineFromProto;
        ProtoToFeaturesTSVLine(protoLine, tsvLineFromProto);

        UNIT_ASSERT_EQUAL(tsvLine, tsvLineFromProto);

        UNIT_ASSERT_EXCEPTION(FeaturesTSVLineToProto("", protoLine), yexception);
        UNIT_ASSERT_NO_EXCEPTION(ProtoToFeaturesTSVLine(NFeaturePool::TLine(), tsvLineFromProto));
    }
};
