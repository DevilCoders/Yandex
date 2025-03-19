#include <kernel/prs_log/serializer/serialize.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {
    NPrsLogProto::TLog CreateLog() {
        NPrsLogProto::TDocument doc;
        doc.SetUrl("some.url.here");
        doc.SetMetaRelevance(0xDEADBEEF);

        NPrsLogProto::TGroup group;
        *group.AddDocuments() = std::move(doc);
        group.SetCategoryName("some.category.name");

        NPrsLogProto::TGrouping grouping;
        *grouping.AddGroups() = std::move(group);

        NPrsLogProto::TLog log;
        *log.MutableGrouping() = std::move(grouping);

        return log;
    }

    void TestLog(const NPrsLogProto::TLog& log) {
        UNIT_ASSERT(log.HasGrouping());
        const NPrsLogProto::TGrouping& grouping = log.GetGrouping();
        UNIT_ASSERT_EQUAL(grouping.GroupsSize(), 1);
        const NPrsLogProto::TGroup& group = grouping.GetGroups(0);
        UNIT_ASSERT_EQUAL(group.GetCategoryName(), "some.category.name");
        UNIT_ASSERT_EQUAL(group.DocumentsSize(), 1);
        const NPrsLogProto::TDocument& doc = group.GetDocuments(0);
        UNIT_ASSERT(doc.HasUrl());
        UNIT_ASSERT_EQUAL(doc.GetUrl(), "some.url.here");
        UNIT_ASSERT_EQUAL(doc.GetMetaRelevance(), 0xDEADBEEF);
    }

    void TestCompressionMethod(NPrsLog::ECompressionMethod compressionMethod) {
        const TString serialized = NPrsLog::SerializeToProtoBase64(CreateLog(), compressionMethod);
        TestLog(NPrsLog::DeserializeFromProtoBase64(serialized));
    }
}

Y_UNIT_TEST_SUITE(TestProtoSerialization) {

    Y_UNIT_TEST(CompressionMethodNone) {
        TestCompressionMethod(NPrsLog::ECompressionMethod::CM_NONE);
    }

    Y_UNIT_TEST(CompressionMethodLz4) {
        TestCompressionMethod(NPrsLog::ECompressionMethod::CM_LZ4);
    }
}
