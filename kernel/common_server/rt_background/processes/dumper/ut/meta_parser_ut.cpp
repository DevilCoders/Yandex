#include <kernel/common_server/rt_background/processes/dumper/meta_parser.h>
#include <fintech/risk/backend/src/proto/feature.pb.h>
#include <fintech/risk/backend/src/proto/info.pb.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/json/json_reader.h>
#include <kernel/common_server/api/common.h>
#include <fintech/risk/backend/src/proto/factor.pb.h>
#include <fintech/risk/backend/src/proto/context_tag.pb.h>

class TServiceInfoData: public TProtoBytesMetaParser<NFintechProto::TServiceInfo> {
private:
    static TFactory::TRegistrator<TServiceInfoData> Registrator;
};

class TContextTagData: public TProtoBytesMetaParser<NFintechProto::TContextTag> {
private:
    static TFactory::TRegistrator<TContextTagData> Registrator;
};

TServiceInfoData::TFactory::TRegistrator<TServiceInfoData> TServiceInfoData::Registrator("request_info");
TContextTagData::TFactory::TRegistrator<TContextTagData> TContextTagData::Registrator("tag_object_proto");

Y_UNIT_TEST_SUITE(MetaParser) {

    Y_UNIT_TEST(Simple) {
        {
            NCS::NStorage::TTableRecordWT record;
            NJson::TJsonValue json;
            record.Set("request_info", "0a07706172746965735a1a0a180a046261736512100a0e0a057970756964120508b6f7bd50");
            record.Set("tag_object_proto", "229f010a9c0112250a1966657463685f6d6f7261746f7269756d5f696e666f5f313064180a200028003001380012250a1966657463685f6d6f7261746f7269756d5f696e666f5f333064180a200028003001380012250a1966657463685f6d6f7261746f7269756d5f696e666f5f363064180a200028003001380012250a1966657463685f6d6f7261746f7269756d5f696e666f5f393064180a2000280030013800");
            for (auto&& i : record) {
                auto metaParser = IDumperMetaParser::TFactory::Construct(i.first);
                CHECK_WITH_LOG(metaParser != nullptr);
                json[i.first] = metaParser->ParseMeta(i.second);
            }
            CHECK_WITH_LOG(json["request_info"]["parties_info"].IsMap() == true) << json.GetStringRobust();
            CHECK_WITH_LOG(json["tag_object_proto"]["context_progress_tag"].IsMap() == true) << json.GetStringRobust();
        }
    };

}
