package DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds

import DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds")
    name = "prod (No-agent builds)"

    buildType(DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds_AcceptanceTestBig)
    buildType(DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds_AcceptanceTestSmall)
    buildType(DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds_AcceptanceTestMedium)
    buildType(DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds_AcceptanceTestEnormous)

    params {
        param("cluster", "prod")
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds_AcceptanceTestSmall, DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds_AcceptanceTestMedium, DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds_AcceptanceTestBig, DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds_AcceptanceTestEnormous)
})
