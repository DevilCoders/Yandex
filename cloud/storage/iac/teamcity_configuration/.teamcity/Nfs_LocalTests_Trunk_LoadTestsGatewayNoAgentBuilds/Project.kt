package Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds

import Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds")
    name = "Load Tests Gateway (No-agent builds)"

    buildType(Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseAddressSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsRelwithdebinfo)
    buildType(Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseMemorySanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseThreadSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseUbSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseHardening)

    params {
        param("env.TEST_TARGETS", "cloud/blockstore/tests/functional")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_942"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsRelwithdebinfo, Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseHardening, Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseAddressSanitizer, Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseMemorySanitizer, Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseThreadSanitizer, Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds_LoadTestsReleaseUbSanitizer)
})
