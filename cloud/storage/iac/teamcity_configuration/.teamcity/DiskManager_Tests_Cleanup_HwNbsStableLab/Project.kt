package DiskManager_Tests_Cleanup_HwNbsStableLab

import DiskManager_Tests_Cleanup_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_Cleanup_HwNbsStableLab")
    name = "hw-nbs-stable-lab"
    archived = true

    buildType(DiskManager_Tests_Cleanup_HwNbsStableLab_AcceptanceTestCommon)
    buildType(DiskManager_Tests_Cleanup_HwNbsStableLab_AcceptanceTest)
    buildType(DiskManager_Tests_Cleanup_HwNbsStableLab_EternalAcceptanceTest)

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_Cleanup_HwNbsStableLab_AcceptanceTestCommon, DiskManager_Tests_Cleanup_HwNbsStableLab_AcceptanceTest, DiskManager_Tests_Cleanup_HwNbsStableLab_EternalAcceptanceTest)
})
