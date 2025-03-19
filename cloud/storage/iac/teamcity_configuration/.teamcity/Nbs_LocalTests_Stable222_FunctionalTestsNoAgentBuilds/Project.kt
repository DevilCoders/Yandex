package Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds

import Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds")
    name = "Functional Tests (No-agent builds)"

    buildType(Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseUbSanitizer)
    buildType(Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsRelwithdebinfo)
    buildType(Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseHardening)
    buildType(Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseThreadSanitizer)
    buildType(Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseMemorySanitizer)
    buildType(Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseAddressSanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/blockstore/tests/functional")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_503"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsRelwithdebinfo, Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseHardening, Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseAddressSanitizer, Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseMemorySanitizer, Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseThreadSanitizer, Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseUbSanitizer)
})
