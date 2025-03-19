#include "protoconf.h"
#include <kernel/gazetteer/config/test.pb.h>


#include <util/system/tempfile.h>
#include <util/stream/file.h>

#include <library/cpp/testing/unittest/registar.h>


class TProtoConfTest: public TTestBase {
    UNIT_TEST_SUITE(TProtoConfTest)
        UNIT_TEST(SimpleTest);
    UNIT_TEST_SUITE_END();

public:

    TAutoPtr<TTempFile> MakeTestConfig() const {
        const TString confData(
            "NProtoConfigUt.TTestConfig {\n"
            "   DataFile = [\"mydata.txt\", \"abc.def\"] // comments\n"
            "   Mode = READ\n"
            "   Network = { Host = 'palantir.yandex.ru' Port = 1234 } \n"
            "}"
        );

        TString tmpFileName = "test-2kcnakj28.tmp.cfg";
        {
            TOFStream writer(tmpFileName);
            writer << confData;
        }

        return new TTempFile(tmpFileName);
    }


    void SimpleTest() {
        THolder<TTempFile> tmpFile(MakeTestConfig());

        NProtoConfigUt::TTestConfig config;
        UNIT_ASSERT(NProtoConf::LoadFromFile(tmpFile->Name(), config));

        UNIT_ASSERT_EQUAL(config.DataFileSize(), 2);
        UNIT_ASSERT_EQUAL(config.GetDataFile(0), "mydata.txt");
        UNIT_ASSERT_EQUAL(config.GetDataFile(1), "abc.def");
        UNIT_ASSERT_EQUAL(config.GetMode(), NProtoConfigUt::TTestConfig::READ);
        UNIT_ASSERT(config.HasNetwork());
        UNIT_ASSERT_EQUAL(config.GetNetwork().GetHost(), "palantir.yandex.ru");
        UNIT_ASSERT_EQUAL(config.GetNetwork().GetPort(), 1234);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TProtoConfTest);
