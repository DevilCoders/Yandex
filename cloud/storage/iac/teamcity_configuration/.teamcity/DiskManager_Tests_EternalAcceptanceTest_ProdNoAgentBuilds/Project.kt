package DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds

import DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds")
    name = "prod (No-agent builds)"

    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest2GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest16GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest4GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest64GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest8GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest256GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest4TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest32GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest512GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest1TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest2TiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest128GiB)
    buildType(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest8TiB)

    params {
        param("cluster", "prod")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_1246"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest2GiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest4GiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest8GiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest16GiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest32GiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest64GiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest128GiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest256GiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest512GiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest1TiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest2TiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest4TiB, DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest8TiB)
})
