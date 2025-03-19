package DiskManager_Tests_AcceptanceTests_HwNbsStableLab

import DiskManager_Tests_AcceptanceTests_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_AcceptanceTests_HwNbsStableLab")
    name = "hw-nbs-stable-lab"
    archived = true

    buildType(DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestMedium)
    buildType(DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestSmall)
    buildType(DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestBig)
    buildType(DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestEnormous)

    params {
        param("cluster", "hw_nbs_stable_lab")
        param("env.CLUSTER", "hw-nbs-stable-lab")
        param("env.INSTANCE_RAM", "8")
        param("env.INSTANCE_CORES", "8")
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestSmall, DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestMedium, DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestBig, DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestEnormous)
})
