#include <kernel/daemon/config/patcher.h>
#include <kernel/daemon/config/daemon_config.h>
#include <library/cpp/testing/unittest/gtest.h>
#include <library/cpp/json/json_reader.h>

#include <util/stream/output.h>
#include <util/stream/file.h>

namespace {
    const TString ItsFilePath("its_config.json");
    const TString YconfFolderPath(".");
    const TString YconfConfigFilePath(YconfFolderPath + "/config_file.cfg");

    const TString ItsFilePath2("its_config2.json");

    void PrepareYconfConfigFile(const TString &text) {
        TFileOutput file(YconfConfigFilePath);
        file << text;
    }

    void PrepareItsConfigFile(const TString &text, const TString &path) {
        TFileOutput file(path);
        file << text;
    }

    TString GetConfigText(const TString &yconf, const TString &itsConfig) {
        TConfigPatcher ConfigPatcher;
        ConfigPatcher.SetItsConfigPath(ItsFilePath);
        ConfigPatcher.GetPreprocessor()->SetStrict(true);
        PrepareYconfConfigFile(yconf);
        PrepareItsConfigFile(itsConfig, ItsFilePath);
        return ConfigPatcher.ReadAndProcess(YconfConfigFilePath);
    }

    TString GetConfigTextTwoFiles(const TString &yconf, const TString &itsConfig1, const TString &itsConfig2) {
        TConfigPatcher ConfigPatcher;
        ConfigPatcher.SetItsConfigPath(ItsFilePath);
        ConfigPatcher.SetItsConfigPath(ItsFilePath2);
        ConfigPatcher.GetPreprocessor()->SetStrict(true);
        PrepareYconfConfigFile(yconf);
        PrepareItsConfigFile(itsConfig1, ItsFilePath);
        PrepareItsConfigFile(itsConfig2, ItsFilePath2);
        return ConfigPatcher.ReadAndProcess(YconfConfigFilePath);
    }
}

TEST(ItsConfig, CommonTest) {
    TString itsConf = R"(
{
  "files": [
    {
      "name": "include_file.cfg",
      "value": {
        "Field1": {
          "ValueInt": 10,
          "ValueBool": true
        },
        "Field2": [{
          "ValueStr": "1h"
        }, {
          "ValueDouble2": 2.2
        }],
        "Field3": {
          "ValueDouble3": 3.2
        }
      }
    }
  ],
  "patches": {
    "State.PatchState.Value1": "4000",
    "State.PatchState.Value2": "__remove__",
    "State.PatchState1.Value1": "new node",
    "NewState.PatchState1.Value1": "new root",
    "State.FileState.Field3.ValueDouble3": "4.2"
  }
})";

    TString yconf = R"(
<State>
  <FileState>
    #include include_file.cfg
  </FileState>
  <PatchState>
    Value1: 1
    Value2: 2
  </PatchState>
</State>)";

    auto text = GetConfigText(yconf, itsConf);
    Cout << "Config text: " << Endl << text << Endl;

    TAnyYandexConfig Config;
    EXPECT_TRUE(Config.ParseMemory(text));
    const auto& rootChilds = Config.GetRootSection()->GetAllChildren();

    EXPECT_TRUE(rootChilds.contains("State"));
    const auto& stateChilds = rootChilds.find("State")->second->GetAllChildren();
    EXPECT_FALSE(stateChilds.find("PatchState")->second->GetDirectives().contains("Value2"));
    EXPECT_EQ(stateChilds.find("PatchState")->second->GetDirectives().Value<int>("Value1"), 4000);
    EXPECT_EQ(stateChilds.find("PatchState1")->second->GetDirectives().Value<TString>("Value1"), "new node");

    EXPECT_TRUE(rootChilds.contains("NewState"));
    const auto& newStateChilds = rootChilds.find("NewState")->second->GetAllChildren();
    EXPECT_EQ(newStateChilds.find("PatchState1")->second->GetDirectives()["Value1"], "new root");

    EXPECT_TRUE(stateChilds.contains("FileState"));
    const auto& fileStateChilds = stateChilds.find("FileState")->second->GetAllChildren();
    EXPECT_EQ(fileStateChilds.find("Field1")->second->GetDirectives().Value<int>("ValueInt"), 10);
    EXPECT_EQ(fileStateChilds.find("Field1")->second->GetDirectives().Value<bool>("ValueBool"), true);
    EXPECT_EQ(fileStateChilds.find("Field3")->second->GetDirectives().Value<double >("ValueDouble3"), 4.2);

    int i = 0;
    auto field2Nodes = fileStateChilds.equal_range("Field2");
    for (auto field2 = field2Nodes.first; field2 != field2Nodes.second; ++field2) {
        const auto& directives = field2->second->GetDirectives();
        if (directives.contains("ValueStr")) {
            EXPECT_EQ(directives.Value<TString>("ValueStr"), "1h");
        } else {
            EXPECT_EQ(directives.Value<double>("ValueDouble2"), 2.2);
        }
        EXPECT_TRUE(i < 2);
        i++;
    }
}

TEST(ItsConfig, RobotPatchesTest) {
    TString itsConf = R"(
{
  "robotPatches": {
    "State.PatchState.Value1": "4000",
    "State.PatchState.Value2": "__remove__",
    "NewState.PatchState1.Value1": "new root"
  },
  "patches": {
    "State.PatchState.Value1": "5000",
    "State.PatchState1.Value1": "new node"
  }
})";

    TString yconf = R"(
<State>
  <PatchState>
    Value1: 1
    Value2: 2
  </PatchState>
</State>)";

    auto text = GetConfigText(yconf, itsConf);
    Cout << "Config text: " << Endl << text << Endl;

    TAnyYandexConfig Config;
    EXPECT_TRUE(Config.ParseMemory(text));
    const auto& rootChilds = Config.GetRootSection()->GetAllChildren();

    EXPECT_TRUE(rootChilds.contains("State"));
    const auto& stateChilds = rootChilds.find("State")->second->GetAllChildren();
    EXPECT_FALSE(stateChilds.find("PatchState")->second->GetDirectives().contains("Value2"));
    EXPECT_EQ(stateChilds.find("PatchState")->second->GetDirectives().Value<int>("Value1"), 5000);
    EXPECT_EQ(stateChilds.find("PatchState1")->second->GetDirectives().Value<TString>("Value1"), "new node");

    EXPECT_TRUE(rootChilds.contains("NewState"));
    const auto& newStateChilds = rootChilds.find("NewState")->second->GetAllChildren();
    EXPECT_EQ(newStateChilds.find("PatchState1")->second->GetDirectives()["Value1"], "new root");
}


TEST(ItsConfig, TwoFilesTest) {
    TString itsConf1 = R"(
{
  "robotPatches": {
    "State.PatchState.Value1": "4000",
    "State.PatchState.Value2": "__remove__",
    "NewState.PatchState1.Value1": "new root"
  },
  "patches": {
    "State.PatchState2.Value1": "value to overwrite"
  }
})";

    TString itsConf2 = R"(
{
  "patches": {
    "State.PatchState.Value1": "5000",
    "State.PatchState1.Value1": "new node",
    "State.PatchState2.Value1": "overwritten value"
  }
})";

    TString yconf = R"(
<State>
  <PatchState>
    Value1: 1
    Value2: 2
  </PatchState>
</State>)";


    auto text = GetConfigTextTwoFiles(yconf, itsConf1, itsConf2);
    Cout << "Config text: " << Endl << text << Endl;

    TAnyYandexConfig Config;
    EXPECT_TRUE(Config.ParseMemory(text));
    const auto& rootChilds = Config.GetRootSection()->GetAllChildren();

    EXPECT_TRUE(rootChilds.contains("State"));
    const auto& stateChilds = rootChilds.find("State")->second->GetAllChildren();
    EXPECT_FALSE(stateChilds.find("PatchState")->second->GetDirectives().contains("Value2"));
    EXPECT_EQ(stateChilds.find("PatchState")->second->GetDirectives().Value<int>("Value1"), 5000);
    EXPECT_EQ(stateChilds.find("PatchState1")->second->GetDirectives().Value<TString>("Value1"), "new node");
    EXPECT_EQ(stateChilds.find("PatchState2")->second->GetDirectives().Value<TString>("Value1"), "overwritten value");

    EXPECT_TRUE(rootChilds.contains("NewState"));
    const auto& newStateChilds = rootChilds.find("NewState")->second->GetAllChildren();
    EXPECT_EQ(newStateChilds.find("PatchState1")->second->GetDirectives()["Value1"], "new root");
}
