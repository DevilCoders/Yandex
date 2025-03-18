#include <library/cpp/protobuf/from_xml/pbfromxml.h>
#include <library/cpp/protobuf/from_xml/ut/proto/data_types.pb.h>
#include <library/cpp/protobuf/from_xml/ut/proto/recursive.pb.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/text_format.h>
#include <contrib/libs/pugixml/pugixml.hpp>

#include <util/generic/map.h>
#include <util/stream/file.h>
#include <util/string/vector.h>

using namespace NProtobufFromXml;

namespace {
    void Pair(PbMessage& msg, const pugi::xml_node& node) {
        Fields(msg, node, {{"first", "first"}, {"second", "second"}});
    }

    void Recursive(PbMessage& msg, const pugi::xml_node& node) {
        Fields(msg, node, {{"name", "name"}, {"count", "@count"}, {"pair", "pair", &Pair}, {"child", "recursive", &Recursive}});
    }

    TMaybe<TEnum> Type(const TString& value) {
        static const TMap<TString, TEnum> types{
            {"t1", {"T1"}},
            {"t2", {"T2"}},
            {"t3", {"T3"}}};
        return types.contains(value) ? types.at(value) : TMaybe<TEnum>();
    }

    void Data(PbMessage& msg, const pugi::xml_node& node) {
        Fields(msg, node, {{"s", "string"}, {"i", "int32"}, {"si", "sint32"}, {"ui", "uint32"}, {"i64", "int64"}, {"si64", "sint64"}, {"ui64", "uint64"}, {"f", "float"}, {"d", "double"}, {"b", "bool"}, {"t", "type", &Type}});
    }

    template <typename Message>
    void Check(const TString& xmlName, const TString& pbName, void (*Root)(PbMessage&, const pugi::xml_node&)) {
        pugi::xml_document doc;
        Y_VERIFY(doc.load_file(xmlName.c_str()));
        Message pbRoot;
        Root(pbRoot, doc.select_node("//Root").node());

        TString convertResult;
        google::protobuf::TextFormat::PrintToString(pbRoot, &convertResult);

        if (!pbRoot.IsInitialized()) {
            TVector<TString> errors;
            pbRoot.FindInitializationErrors(&errors);
            for (const auto& error : errors) {
                Cerr << error << Endl;
            }
            throw yexception() << "not all required fields are filled";
        }

        UNIT_ASSERT_EQUAL(convertResult, TUnbufferedFileInput(pbName).ReadAll());
    }

    TString TestDataPath(const TString& fileName) {
        return ArcadiaSourceRoot() + "/library/cpp/protobuf/from_xml/ut/data/" + fileName;
    }

}

Y_UNIT_TEST_SUITE(FieldsTests) {
    Y_UNIT_TEST(Fields_givenRecursiveMessage_parsesItCorrectly) {
        Check<NRecursive::Root>(
            TestDataPath("recursive.xml"),
            TestDataPath("recursive.pb"),
            [](PbMessage& msg, const pugi::xml_node& node) {
                Fields(msg, node, {{"s", "string"}, {"rec", "recursive", &Recursive}});
            });
    }

    Y_UNIT_TEST(Fields_givenMessageWithDifferentFieldTypes_parsesItCorrectly) {
        Check<NDataTypes::Root>(
            TestDataPath("data_types.xml"),
            TestDataPath("data_types.pb"),
            [](PbMessage& msg, const pugi::xml_node& node) {
                Fields(msg, node, {{"data", "Data", &Data}});
            });
    }
}
