package DiskManager_Tests

import DiskManager_Tests.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests")
    name = "Tests"
    description = "Disk Manager Tests"

    buildType(DiskManager_Tests_HwNbsStableLabRemoteTest)
    buildType(DiskManager_Tests_HwNbsStableLabRemoteTestNoAgentBuild)

    params {
        param("env.branch_path", "trunk")
    }

    subProject(DiskManager_Tests_Cleanup.Project)
    subProject(DiskManager_Tests_AcceptanceTests.Project)
    subProject(DiskManager_Tests_EternalAcceptanceTest.Project)
})
