package DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds

import DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds")
    name = "preprod (No-agent builds)"

    buildType(DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds_AcceptanceTestEnormous)
    buildType(DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds_AcceptanceTestMedium)
    buildType(DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds_AcceptanceTestBig)
    buildType(DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds_AcceptanceTestSmall)

    params {
        param("cluster", "preprod")
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds_AcceptanceTestSmall, DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds_AcceptanceTestMedium, DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds_AcceptanceTestBig, DiskManager_Tests_AcceptanceTests_PreprodNoAgentBuilds_AcceptanceTestEnormous)
})
