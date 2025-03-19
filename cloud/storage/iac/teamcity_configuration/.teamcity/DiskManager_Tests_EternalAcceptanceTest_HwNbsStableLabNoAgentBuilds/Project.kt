package DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds

import DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds")
    name = "hw-nbs-stable-lab (No-agent builds)"

    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest4GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest2GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest256GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest128GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest8GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest16GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest32GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest1TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest4TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest2TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest64GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest512GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest8TiB)

    params {
        param("cluster", "hw_nbs_stable_lab")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_1244"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest2GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest4GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest8GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest16GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest32GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest64GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest128GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest256GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest512GiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest1TiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest2TiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest4TiB, DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds_EternalAcceptanceTest8TiB)
})
