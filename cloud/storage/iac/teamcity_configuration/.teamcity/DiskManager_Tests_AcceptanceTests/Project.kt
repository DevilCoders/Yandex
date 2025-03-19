package DiskManager_Tests_AcceptanceTests

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_AcceptanceTests")
    name = "Acceptance Tests"

    params {
        param("env.TEST_SUITE", "default")
        param("env.TEST_TYPE", "acceptance")
        param("env.VERIFY_TEST", "/usr/bin/verify-test")
        param("env.DEBUG", "false")
    }

    subProject(DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds.Project)
    subProject(DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds.Project)
    subProject(DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds.Project)
    subProject(DiskManager_Tests_AcceptanceTests_Prod.Project)
    subProject(DiskManager_Tests_AcceptanceTests_Preprod.Project)
    subProject(DiskManager_Tests_AcceptanceTests_HwNbsStableLab.Project)
})
