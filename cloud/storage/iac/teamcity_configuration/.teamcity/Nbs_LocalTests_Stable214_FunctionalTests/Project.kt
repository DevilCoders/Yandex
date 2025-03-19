package Nbs_LocalTests_Stable214_FunctionalTests

import Nbs_LocalTests_Stable214_FunctionalTests.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_LocalTests_Stable214_FunctionalTests")
    name = "Functional Tests"
    archived = true

    buildType(Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsRelwithdebinfo)
    buildType(Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseThreadSanitizer)
    buildType(Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseHardening)
    buildType(Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseAddressSanitizer)
    buildType(Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseMemorySanitizer)
    buildType(Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseUbSanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/blockstore/tests/functional")
    }
    buildTypesOrder = arrayListOf(Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsRelwithdebinfo, Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseHardening, Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseAddressSanitizer, Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseMemorySanitizer, Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseThreadSanitizer, Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseUbSanitizer)
})
