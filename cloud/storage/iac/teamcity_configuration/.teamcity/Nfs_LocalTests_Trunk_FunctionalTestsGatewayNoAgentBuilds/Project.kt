package Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds

import Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds")
    name = "Functional Tests Gateway (No-agent builds)"

    buildType(Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseAddressSanitizer)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseUbSanitizer)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseHardening)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseMemorySanitizer)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsRelwithdebinfo)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseThreadSanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/blockstore/tests/functional")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_808"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsRelwithdebinfo, Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseHardening, Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseAddressSanitizer, Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseMemorySanitizer, Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseThreadSanitizer, Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds_FunctionalTestsReleaseUbSanitizer)
})
