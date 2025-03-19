package DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds

import DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds")
    name = "hw-nbs-stable-lab (No-agent builds)"

    buildType(DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestMedium)
    buildType(DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestEnormous)
    buildType(DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestSmall)
    buildType(DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestBig)

    params {
        param("cluster", "hw_nbs_stable_lab")
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestSmall, DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestMedium, DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestBig, DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestEnormous)
})
