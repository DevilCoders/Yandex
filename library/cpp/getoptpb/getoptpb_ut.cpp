#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/text_format.h>
#include <util/system/tempfile.h>
#include <util/stream/file.h>

#include "getoptpb.h"
#include <library/cpp/getoptpb/ut/testconf.pb.h>
#include <library/cpp/protobuf/json/proto2json.h>

using namespace NGetoptPb;

class TGetoptPbTest: public TTestBase {
    UNIT_TEST_SUITE(TGetoptPbTest);
    UNIT_TEST(Test);
    UNIT_TEST(TestOnlyFile);
    UNIT_TEST(TestJsonConfig);
    UNIT_TEST(TestDefConfPath);
    UNIT_TEST(TestInvalidArgs);
    UNIT_TEST(TestTextConfig);
    UNIT_TEST(TestCheckRepeated);
    UNIT_TEST(TestMapFromJsonObject);
    UNIT_TEST(TestMapFromJsonList);
    UNIT_TEST(TestNoExceptionIfBadJson);
    UNIT_TEST(TestRequiredFieldsFromJson);
    UNIT_TEST(TestUnknownFieldsFromJson);
    UNIT_TEST(TestAllowUnknownOption);
    UNIT_TEST(TestSubcommandInheritSettings);
    UNIT_TEST(TestSeparateFreeArgs);
    UNIT_TEST(TestRequiredFreeArgs);
    UNIT_TEST(TestRepeatedFreeArgs);
    UNIT_TEST(TestMixedFreeArgs);
    UNIT_TEST_SUITE_END();

private:
    void Test();
    void TestOnlyFile();
    void TestJsonConfig();
    void TestDefConfPath();
    void TestInvalidArgs();
    void TestTextConfig();
    void TestCheckRepeated();
    void TestMapFromJsonObject();
    void TestMapFromJsonList();
    void TestNoExceptionIfBadJson();
    void TestRequiredFieldsFromJson();
    void TestUnknownFieldsFromJson();
    void TestAllowUnknownOption();
    void TestSubcommandInheritSettings();
    void TestSeparateFreeArgs();
    void TestRequiredFreeArgs();
    void TestRepeatedFreeArgs();
    void TestMixedFreeArgs();
};

UNIT_TEST_SUITE_REGISTRATION(TGetoptPbTest)

void TGetoptPbTest::Test() {
    const char* argv[] = {"testprog",
                          "--a-nthreads", "50",
                          "-g",
                          "--a-home", "../",
                          "--a-test-enum", "TwO",
                          "--a-max-size", "88.44",
                          "--a-calc-host", "odobenus",
                          "--a-port", "8080",
                          "--a-c-port", "9999",
                          "--a-c-port", "8081",
                          "--a-test-repeated-opt", "55",
                          "--a-test-repeated-opt", "77",
                          "a-cmd2", "--b", "131"};

    //    const char *argv[] = { "testprog", "--help" };

    int argc = sizeof(argv) / sizeof(argv[0]);

    TTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    settings.OptPrefix = "a-";
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);

    if (!ok) {
        fprintf(stderr, "GetoptPb failed: %s\n", errorStr.data());
    }
    UNIT_ASSERT_EQUAL(ok, true);

    const char* correctResult = ""
                                "HomeDir: \"../\"\n"
                                "NumThreads: 50\n"
                                "GodMode: true\n"
                                "MaxSize: 88.44\n" //FIXME: floating point comparison
                                "TestEnum: TE_TWO\n"
                                "Calc {\n"
                                "  Host: \"odobenus\"\n"
                                "}\n"
                                "TestRepeatedOpt: 55\n"
                                "TestRepeatedOpt: 77\n"
                                "Calc3 {\n"
                                "  Port: 8080\n"
                                "}\n"
                                "Calc4 {\n"
                                "  Port: 8081\n"
                                "}\n"
                                "Cmd2 {\n"
                                "  B: 131\n"
                                "}\n";

    if (correctResult != config.DebugString()) {
        fprintf(stderr, "Got incorrect config:\n%s\n", config.DebugString().data());
        UNIT_ASSERT(false);
    }
}

void TGetoptPbTest::TestOnlyFile() {
    TTempFile file("config.pb.txt");

    {
        TSmallTestConfig conf;
        conf.SetHost("localHost");
        conf.SetPort(8080);
        TString str;
        TFileOutput fout(file.Name());
        ::google::protobuf::TextFormat::PrintToString(conf, &str);
        fout << str;
    }

    const char *argv[] = {"testprog", "--config", file.Name().c_str() };
    int argc = sizeof(argv) / sizeof(argv[0]);

    TSmallTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    settings.DontRequireRequired = true;
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);

    UNIT_ASSERT_C(ok, errorStr);
    UNIT_ASSERT_VALUES_EQUAL(config.GetHost(), "localHost");
}

void TGetoptPbTest::TestJsonConfig() {
    TTempFile file("config.json");

    TTestConfig srcConfig;
    {
        TFileOutput fout(file.Name());
        srcConfig.SetMaxSize(123);
        srcConfig.AddTestRepeatedOpt(1);
        srcConfig.AddTestRepeatedOpt(2);
        NProtobufJson::Proto2Json(srcConfig, fout);
    }

    TVector<const char*> argvLong = {"testprog", "--config-json", "config.json", "--max-size", "456", "cmd2", "--b", "1"};
    TVector<const char*> argvShort = {"testprog", "-c", "config.json", "--max-size", "456", "cmd2", "--b", "1"};

    TGetoptPbSettings settingsLong;
    TGetoptPbSettings settingsShort;
    settingsShort.ConfPathShort = '\0';
    settingsShort.ConfPathJsonShort = 'c';

    TVector<std::pair<TVector<const char*>, TGetoptPbSettings>> tests = {
        {argvLong, settingsLong},
        {argvShort, settingsShort},
    };

    for (auto [argv, settings] : tests) {
        TString errorStr;
        TTestConfig config;
        bool ok = GetoptPb(argv.size(), argv.data(), config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_VALUES_EQUAL(config.GetMaxSize(), 456);
        UNIT_ASSERT_VALUES_EQUAL(config.GetTestRepeatedOpt(0), srcConfig.GetTestRepeatedOpt(0));
        UNIT_ASSERT_VALUES_EQUAL(config.GetTestRepeatedOpt(1), srcConfig.GetTestRepeatedOpt(1));
    }
}

void TGetoptPbTest::TestTextConfig() {
    TSmallTestConfig srcConfig;
    TString srcConfigText;

    srcConfig.SetHost("ya.ru");
    srcConfig.SetPort(1337);
    srcConfig.SetPath("/path");

    ::google::protobuf::TextFormat::PrintToString(srcConfig, &srcConfigText);

    const char* argv[] = {"testprog", "--config-text", srcConfigText.data(), "--host", "yandex.ru"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    TSmallTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);

    UNIT_ASSERT_C(ok, errorStr);
    UNIT_ASSERT_VALUES_EQUAL(config.GetHost(), "yandex.ru");
    UNIT_ASSERT_VALUES_EQUAL(config.GetPort(), 1337);
    UNIT_ASSERT_VALUES_EQUAL(config.GetPath(), "/path");
}


void TGetoptPbTest::TestDefConfPath() {
    const char *argv[] = {"testprog", "--max-size", "456", "cmd2", "--b", "1"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    TString absentName = ".394968ss";
    TString presentName = ".dkeke44";

    {
        TFileOutput fout(presentName);
        fout << "Cmd2 { A: 0.5 }";
    }

    TTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    settings.DefaultConfPath = absentName;
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT(!ok);

    settings.IgnoreConfFileReadErrors = true;
    ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);
    UNIT_ASSERT_EQUAL(config.GetCmd2().GetB(), 1);
    UNIT_ASSERT_EQUAL(config.GetCmd2().HasA(), false);

    settings.DefaultConfPath = presentName;
    ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);
    UNIT_ASSERT_EQUAL(config.GetCmd2().GetB(), 1);
    UNIT_ASSERT_EQUAL(config.GetCmd2().GetA(), 0.5);

}

void TGetoptPbTest::TestInvalidArgs() {
    const char* argv[] = {"testprog", "invalid-arg"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    TTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);

    UNIT_ASSERT(!ok);
}

void TGetoptPbTest::TestCheckRepeated() {
    {
        const char* argv[] = {"testprog",
                              "--max-size", "88.44",
                              "--max-size", "44.88",
                              "--test-repeated-opt", "55",
                              "--test-repeated-opt", "77",
                              "cmd2", "--b", "131"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_EQUAL(config.GetMaxSize(), 44.88);
        UNIT_ASSERT_EQUAL(config.GetTestRepeatedOpt(0), 55);
        UNIT_ASSERT_EQUAL(config.GetTestRepeatedOpt(1), 77);

        settings.CheckRepeated = true;
        ok = GetoptPb(argc, argv, config, errorStr, settings);
        UNIT_ASSERT(!ok); // max-size is not repeated field, but has been repeated
    }

    {
        // with check
        const char* argv[] = {"testprog",
                              "--max-size", "44.88",
                              "--test-repeated-opt", "55",
                              "--test-repeated-opt", "77",
                              "cmd2", "--b", "131"};
        int argc = sizeof(argv) / sizeof(argv[0]);
        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        settings.CheckRepeated = true;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_EQUAL(config.GetTestRepeatedOpt(0), 55);
        UNIT_ASSERT_EQUAL(config.GetTestRepeatedOpt(1), 77);
    }

}

void TGetoptPbTest::TestMapFromJsonObject() {
    TTempFile file("config.json");
    {
        TFileOutput fout(file.Name());
        fout.Write(R"({"KeyValue": {"a": "b", "c": "d"}})");
    }
    const char* argv[] = {"testprog", "--config-json", "config.json"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    TMapTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    settings.UseMapAsObject = true;
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);
    UNIT_ASSERT_VALUES_EQUAL(2, config.GetKeyValue().size());
    UNIT_ASSERT_VALUES_EQUAL("b", config.GetKeyValue().at("a"));
    UNIT_ASSERT_VALUES_EQUAL("d", config.GetKeyValue().at("c"));
}

void TGetoptPbTest::TestMapFromJsonList() {
    TTempFile file("config.json");
    {
        TFileOutput fout(file.Name());
        fout.Write(R"({"KeyValue": [{"key": "a", "value": "b"}]})");
    }
    const char* argv[] = {"testprog", "--config-json", "config.json"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    TMapTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);
    UNIT_ASSERT_VALUES_EQUAL(1, config.GetKeyValue().size());
    UNIT_ASSERT_VALUES_EQUAL("b", config.GetKeyValue().at("a"));
}

void TGetoptPbTest::TestNoExceptionIfBadJson() {
    TTempFile file("config.json");
    {
        TFileOutput fout(file.Name());
        fout.Write(R"({"Host": "localhost",})");
    }
    const char* argv[] = {"testprog", "--config-json", "config.json", "--host", "localhost"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    TSmallTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT(!ok);
    UNIT_ASSERT_STRING_CONTAINS(errorStr, "Couldn't parse config from --config.json parameter");
    settings.IgnoreConfFileReadErrors = true;
    ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);
}

void TGetoptPbTest::TestRequiredFieldsFromJson() {
    TTempFile file("config.json");
    {
        TFileOutput fout(file.Name());
        fout.Write(R"({"Port": 80})");
    }
    const char* argv[] = {"testprog", "--config-json", "config.json"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    TSmallTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT(!ok);
    settings.DontRequireRequired = true;
    ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);
}

void TGetoptPbTest::TestUnknownFieldsFromJson() {
    TTempFile file("config.json");
    {
        TFileOutput fout(file.Name());
        fout.Write(R"({"Host": "localhost", "SomeOtherField": ""})");
    }
    const char* argv[] = {"testprog", "--config-json", "config.json"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    TSmallTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT(!ok);
    settings.AllowUnknownFieldsInConfFile = true;
    ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);
}

void TGetoptPbTest::TestAllowUnknownOption() {
    TTempFile file("config.json");
    {
        TFileOutput fout(file.Name());
        fout.Write(R"({"Host": "localhost", "SomeOtherField": ""})");
    }
    const char* argv[] = {"testprog", "--allow-unknown-fields", "--config-json", "config.json"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    TSmallTestConfig config;
    TString errorStr;
    TGetoptPbSettings settings;
    settings.AllowUnknownFieldsInConfFileLong = "allow-unknown-fields";
    bool ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);

    argv[1] = "--allow-unknown-fields=N";
    settings.AllowUnknownFieldsInConfFile = true;
    ok = GetoptPb(argc, argv, config, errorStr, settings);
    UNIT_ASSERT_C(!ok, errorStr);

    const char* argvDef[] = {"testprog", "--config-json", "config.json"};
    settings.AllowUnknownFieldsInConfFile = false;
    ok = GetoptPb(Y_ARRAY_SIZE(argvDef), argvDef, config, errorStr, settings);
    UNIT_ASSERT_C(!ok, errorStr);

    settings.AllowUnknownFieldsInConfFile = true;
    ok = GetoptPb(Y_ARRAY_SIZE(argvDef), argvDef, config, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);
}

void TGetoptPbTest::TestSubcommandInheritSettings() {
    const char* argv[] = {"testprog", "sub", "--config", "config.json"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    TTestInheritMain params;
    TGetoptPbSettings settings;
    settings.ConfPathLong = "";
    TString errorStr;

    // TTestInheritMain contains TTestInheritSubcommand with Config field which conflicts with default --config option.
    // Although ConfPathLong is empty, it's still unparseable because subcommands do not inherit all parser settings.
    // Therefore the GetoptPb call below should fail
    bool ok = GetoptPb(argc, argv, params, errorStr, settings);
    UNIT_ASSERT(!ok);

    settings.SubcommandsInheritSettings = true;
    // This time we enabled settings inheritance so that GetoptPb should not fail.
    ok = GetoptPb(argc, argv, params, errorStr, settings);
    UNIT_ASSERT_C(ok, errorStr);
}

void TGetoptPbTest::TestSeparateFreeArgs() {
    {
        const char* argv[] = {"testprog", "--max-size", "88.44",
                              "cmd2", "--b", "131",
                              "text", "1", "da", "44.88", "optional-string"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_VALUES_EQUAL(config.GetMaxSize(), 88.44);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeText(), "text");
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeInt(), 1);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeBool(), true);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeDouble(), 44.88);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeText2(), "optional-string");
    }

    {
        const char* argv[] = {"testprog", "--max-size", "88.44",
                              "cmd2", "--b", "131",
                              "text", "1"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_VALUES_EQUAL(config.GetMaxSize(), 88.44);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeText(), "text");
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeInt(), 1);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeBool(), false);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeDouble(), 0.01);
        UNIT_ASSERT(!config.GetCmd2().HasFreeText2());
    }

    {
        const char* argv[] = {"testprog", "--max-size", "88.44",
                              "cmd2", "--b", "131"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_VALUES_EQUAL(config.GetMaxSize(), 88.44);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeText(), "aa");
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeInt(), 1000);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeBool(), false);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd2().GetFreeDouble(), 0.01);
        UNIT_ASSERT(!config.GetCmd2().HasFreeText2());
    }

    {
        const char* argv[] = {"testprog", "--max-size", "88.44",
                              "cmd2", "--b", "131",
                              "text", "1", "da", "44.88", "optional-string",
                              "too-much"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT(!ok);
    }
}

void TGetoptPbTest::TestRequiredFreeArgs() {
    {
        const char* argv[] = {"testprog", "--max-size", "88.44",
                              "cmd3", "required-string"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_VALUES_EQUAL(config.GetMaxSize(), 88.44);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd3().GetFree(), "required-string");
    }

    {
        const char* argv[] = {"testprog", "--max-size", "88.44",
                              "cmd3"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT(!ok);
    }

    {
        const char* argv[] = {"testprog", "--max-size", "88.44",
                              "cmd3", "required-string", "too-much"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT(!ok);
    }
}

void TGetoptPbTest::TestRepeatedFreeArgs() {
    {
        const char* argv[] = {"testprog", "--max-size", "88.44",
                              "cmd1", "--a", "1"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_VALUES_EQUAL(config.GetMaxSize(), 88.44);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd1().GetA(), 1);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd1().FreeSize(), 0);
    }

    {
        const char* argv[] = {"testprog", "--max-size", "88.44",
                              "cmd1", "--a", "1",
                              "free1", "free2"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestConfig config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_VALUES_EQUAL(config.GetMaxSize(), 88.44);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd1().GetA(), 1);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd1().FreeSize(), 2);
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd1().GetFree(0), "free1");
        UNIT_ASSERT_VALUES_EQUAL(config.GetCmd1().GetFree(1), "free2");
    }
}

void TGetoptPbTest::TestMixedFreeArgs() {
    {
        const char* argv[] = {"testprog", "required"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestMixedFreeArgs config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_VALUES_EQUAL(config.GetA(), "required");
        UNIT_ASSERT_VALUES_EQUAL(config.GetB(), "default");
        UNIT_ASSERT(!config.HasC());
        UNIT_ASSERT_VALUES_EQUAL(config.DSize(), 0);
    }

    {
        const char* argv[] = {"testprog"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestMixedFreeArgs config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT(!ok);
    }

    {
        const char* argv[] = {"testprog", "required", "non-default", "optional", "repeat1", "repeat2"};
        int argc = sizeof(argv) / sizeof(argv[0]);

        TTestMixedFreeArgs config;
        TString errorStr;
        TGetoptPbSettings settings;
        bool ok = GetoptPb(argc, argv, config, errorStr, settings);

        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_C(ok, errorStr);
        UNIT_ASSERT_VALUES_EQUAL(config.GetA(), "required");
        UNIT_ASSERT_VALUES_EQUAL(config.GetB(), "non-default");
        UNIT_ASSERT_VALUES_EQUAL(config.GetC(), "optional");
        UNIT_ASSERT_VALUES_EQUAL(config.DSize(), 2);
        UNIT_ASSERT_VALUES_EQUAL(config.GetD(0), "repeat1");
        UNIT_ASSERT_VALUES_EQUAL(config.GetD(1), "repeat2");
    }
}
