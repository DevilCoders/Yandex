package Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds

import Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds")
    name = "Functional Tests (No-agent builds)"

    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseHardening)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsRelwithdebinfo)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseMemorySanitizer)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseUbSanitizer)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseThreadSanitizer)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseAddressSanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/blockstore/tests/functional")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_761"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsRelwithdebinfo, Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseHardening, Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseAddressSanitizer, Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseMemorySanitizer, Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseThreadSanitizer, Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseUbSanitizer)
})
