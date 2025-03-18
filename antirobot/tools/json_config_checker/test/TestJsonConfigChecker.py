from devtools.fleur import util
from devtools.fleur.ytest import suite, test, generator
from devtools.fleur.ytest.tools.asserts import *

from antirobot.daemon.test.AntirobotTestSuite import AntirobotTestSuite

import os
import shutil
import json

ANTIROBOT_TEST_DATA = 'Antirobot.Daemon.Data'
CONFIG_DIR = 'test_file'
CONFIG_FIELDS = [
    'service', 're_queries', 'enabled', 'bans_enabled', 'blocks_enabled',
    'formula', 'threshold',
    'cbb_ip_flag', 'cbb_re_flag', 'cbb_re_mark_flag', 'cbb_re_mark_log_only_flag', 'cbb_re_user_mark_flag',
    'cbb_captcha_re_flag', 'cbb_farmable_ban_flag', 're_groups', 'random_events_fraction',
    'additional_factors_fraction', 'random_factors_fraction', 'cgi_secrets'
]
CONFIG_VALUES = [
    'other', [], True, True, True, True, 0.1, 'matrixnet.info', 2.0, 162, 183,
    185, 702, [], 262, 226, [], 0, [],
]

@suite(package="antirobot.tools")
class JsonConfigChecker(AntirobotTestSuite):

    def GenerateGoodConfigData(self):
        dataForService = {}
        for i, field in enumerate(CONFIG_FIELDS):
            dataForService[field] = CONFIG_VALUES[i]
        return [dataForService]

    def GenerateBadConfigDataWithBadFieldValue(self, field):
        jsonData = self.GenerateGoodConfigData()
        jsonData[0][field] = 'some strange text'
        return jsonData

    def GenerateBadConfigDataWithUnknownField(self):
        jsonData = self.GenerateGoodConfigData()
        jsonData[0]['some strange field'] = 'some strange text'
        return jsonData

    def GenerateBadConfigDataWithoutOneField(self, field):
        jsonData = self.GenerateGoodConfigData()
        del jsonData[0][field]
        return jsonData

    def CreateConfigFile(self, filePath, jsonData):
        with open(filePath, 'w') as jsonFile:
            json.dump(jsonData, jsonFile)

    def SetupSuite(self):
        super(JsonConfigChecker, self).SetupSuite()
        self.BinPath = self.GetBinPath("json_config_checker")
        self.TestDataPath = self.GetTestData(owner=ANTIROBOT_TEST_DATA, type='input')
        self.fullRegExpPath = os.path.join(CONFIG_DIR, 'service_identifier.json')
        self.fullJsonConfigPath = os.path.join(CONFIG_DIR, 'service_config.json')

        shutil.copytree(os.path.join(self.TestDataPath, 'formulas'), 'formulas')
        os.mkdir(CONFIG_DIR)
        shutil.copy(os.path.join(self.TestDataPath, 'data', 'service_identifier.json'), CONFIG_DIR)

    def RunChecker(self):
        return util.process.Execute(
            [
                self.BinPath,
                "--service-config", self.fullJsonConfigPath,
                "--service-identifier", self.fullRegExpPath,
            ],
            raiseOnError=True,
            shell=False,
        )

    @test
    @generator(CONFIG_FIELDS)
    def FailOnBadConfigFileWithBadFieldValue(self, fieldToScratch):

        self.CreateConfigFile(self.fullJsonConfigPath, self.GenerateBadConfigDataWithBadFieldValue(fieldToScratch))

        AssertRaises(lambda : self.RunChecker(), util.process.CommandExecutionException)

    @test
    def PassOnCorrectJsonFile(self):
        self.CreateConfigFile(self.fullJsonConfigPath, self.GenerateGoodConfigData())
        process = self.RunChecker()

        AssertEqual(process.GetReturnCode(), 0)

    @test
    def FailOnBadConfigFileWithUnknownField(self):
        self.CreateConfigFile(self.fullJsonConfigPath, self.GenerateBadConfigDataWithUnknownField())

        AssertRaises(lambda : self.RunChecker(), util.process.CommandExecutionException)

    @test
    @generator(CONFIG_FIELDS)
    def FailOnBadConfigFileWithoutOneField(self, field):
        self.CreateConfigFile(self.fullJsonConfigPath, self.GenerateBadConfigDataWithoutOneField(field))

        AssertRaises(lambda : self.RunChecker(), util.process.CommandExecutionException)
