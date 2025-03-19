package Nbs_LocalTests_Stable222_FunctionalTests

import Nbs_LocalTests_Stable222_FunctionalTests.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart
import jetbrains.buildServer.configs.kotlin.v2019_2.CustomChart.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.buildTypeCustomChart

object Project : Project({
    id("Nbs_LocalTests_Stable222_FunctionalTests")
    name = "Functional Tests"
    archived = true

    buildType(Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseMemorySanitizer)
    buildType(Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsRelwithdebinfo)
    buildType(Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseHardening)
    buildType(Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseAddressSanitizer)
    buildType(Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseThreadSanitizer)
    buildType(Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseUbSanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/blockstore/tests/functional")
    }

    features {
        buildTypeCustomChart {
            id = "PROJECT_EXT_363"
            title = "Total build time"
            seriesTitle = "Serie"
            format = CustomChart.Format.DURATION
            series = listOf(
                Serie(title = "Build Duration (all stages)", key = SeriesKey.BUILD_DURATION)
            )
        }
    }
    buildTypesOrder = arrayListOf(Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsRelwithdebinfo, Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseHardening, Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseAddressSanitizer, Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseMemorySanitizer, Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseThreadSanitizer, Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseUbSanitizer)
})
