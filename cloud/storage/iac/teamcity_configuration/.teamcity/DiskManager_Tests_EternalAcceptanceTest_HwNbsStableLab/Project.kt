package DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab

import DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab")
    name = "hw-nbs-stable-lab"
    archived = true

    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest64GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest16GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest2GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest4GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest128GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest256GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest8GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest512GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest32GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest8TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest4TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest1TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest2TiB)

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
        param("env.INSTANCE_RAM", "8")
        param("env.INSTANCE_CORES", "8")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_386"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest2GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest4GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest8GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest16GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest32GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest64GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest128GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest256GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest512GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest1TiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest2TiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest4TiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest8TiB)
})
