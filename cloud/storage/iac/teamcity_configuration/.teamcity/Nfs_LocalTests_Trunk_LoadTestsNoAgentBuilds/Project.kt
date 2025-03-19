package Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds

import Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds")
    name = "Load Tests (No-agent builds)"

    buildType(Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseAddressSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseThreadSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseHardening)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseUbSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsRelwithdebinfo)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseMemorySanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/blockstore/tests/functional")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_809"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsRelwithdebinfo, Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseHardening, Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseAddressSanitizer, Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseMemorySanitizer, Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseThreadSanitizer, Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds_LoadTestsReleaseUbSanitizer)
})
