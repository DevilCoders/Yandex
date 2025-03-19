package DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds

import DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds")
    name = "preprod (No-agent builds)"

    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest2TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest2GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest8GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest256GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest4GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest512GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest64GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest128GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest8TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest4TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest1TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest16GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest32GiB)

    params {
        param("cluster", "preprod")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_1245"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest2GiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest4GiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest8GiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest16GiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest32GiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest64GiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest128GiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest256GiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest512GiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest1TiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest2TiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest4TiB, DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest8TiB)
})
