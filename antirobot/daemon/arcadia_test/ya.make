PY3TEST()

OWNER(
    g:antirobot
)

IF (NOT SANITIZER_TYPE)
    SIZE(MEDIUM)
ELSE()
    SIZE(LARGE)
    TAG(ya:fat)
ENDIF()

REQUIREMENTS(
    cpu:2
    network:full
)

TEST_SRCS(
    TestAdminCommands.py
    TestAntiDDos.py
    TestAntiDDosStandalone.py
    TestAntiDDosTool.py
    TestAntirobotFactors.py
    TestBackCompatibility.py
    TestBadRequests.py
    TestBalancerHeaders.py
    TestBanSourceIpHeader.py
    TestBlockedByCaptcha.py
    TestBlockedPage.py
    TestBlockedResponses.py
    TestCacheSyncProtocol.py
    TestCaptchaApi.py
    TestCaptchaFury.py
    TestCaptchaGenErrorHandling.py
    TestCaptchaPage.py
    TestCaptchaRequest.py
    TestCbbFlags.py
    TestCheckpoints.py
    TestChinaRedirect.py
    TestCollapsingIPv6.py
    TestCommandLineArgs.py
    TestConfigs.py
    TestCustomRequestClassifierRules.py
    TestDegradationHeader.py
    TestDictionaries.py
    TestDisabling.py
    TestDiscovery.py
    TestExperimentFormulas.py
    TestFaultTolerance.py
    TestLogs.py
    TestMarkPassedRobotRequests.py
    TestMarkedResponses.py
    TestMatrixnetThresholds.py
    TestPingControl.py
    TestPreviewIdentType.py
    TestProjectIdWhitelist.py
    TestRandomFactorsLearn.py
    TestReq2Info.py
    TestReqTypes.py
    TestRobotnessHeader.py
    TestSaveFactors.py
    TestServerError.py
    TestSpravkaRobots.py
    TestSuspiciousnessHeader.py
    TestTrustedUsers.py
    TestUnistat.py
    TestVerochka.py
    TestVerticalsConsistency.py
    TestXmlSearch.py
    TestYqlRules.py
)

PEERDIR(
    antirobot/daemon/arcadia_test/util
    antirobot/idl
    antirobot/tools/yasm_stats/lib
    contrib/libs/brotli/python
    contrib/libs/grpc/src/python/grpcio_status
    contrib/python/cryptography
    contrib/python/PyJWT
    contrib/python/pytest-timeout
    contrib/python/requests
    infra/yp_service_discovery/api
)

DEPENDS(
    antirobot/daemon
    antirobot/scripts/gencfg
    antirobot/tools/access_service_mock
    antirobot/tools/antiddos
    antirobot/tools/api_captcha_mock
    antirobot/tools/cbb_api_mock
    antirobot/tools/check_spravka
    antirobot/tools/discovery_mock
    antirobot/tools/evlogdump
    antirobot/tools/fury_mock
    antirobot/tools/resource_manager_mock
    antirobot/tools/req2info
    antirobot/tools/wizard_mock
    antirobot/captcha_cloud_api/bin
    logbroker/unified_agent/bin
)

DATA(
    arcadia/antirobot/config/global_config.json
    arcadia/antirobot/config/header_hashes.json
    arcadia/antirobot/config/service_config.json
    arcadia/antirobot/config/service_identifier.json
    arcadia/antirobot/daemon_lib/factors_versions
    arcadia/antirobot/captcha/generated/js_print_mapping.json
    arcadia/antirobot/daemon/antirobot_daemon
    arcadia/antirobot/scripts/gencfg/antirobot_gencfg
    arcadia/antirobot/tools/antiddos/antiddos
    arcadia/antirobot/tools/api_captcha_mock/api_captcha_mock
    arcadia/antirobot/tools/cbb_api_mock/cbb_api_mock
    arcadia/antirobot/tools/check_spravka/check_spravka
    arcadia/antirobot/tools/discovery_mock/discovery_mock
    arcadia/antirobot/tools/evlogdump/antirobot_evlogdump
    arcadia/antirobot/tools/fury_mock/fury_mock
    arcadia/antirobot/tools/req2info/req2info
    arcadia/antirobot/tools/wizard_mock/wizard_mock
    sbr://3281797692 # antirobot data/formulas
    sbr://2410458084 # prev antirobot binary
    arcadia/logbroker/unified_agent/bin/unified_agent
)

FORK_TEST_FILES()

REQUIREMENTS(ram:32)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

END()
