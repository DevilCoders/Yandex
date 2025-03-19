package Nbs_LocalTests_Stable214_LocalLoadTests

import Nbs_LocalTests_Stable214_LocalLoadTests.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_LocalTests_Stable214_LocalLoadTests")
    name = "Local Load Tests"
    archived = true

    buildType(Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseHardening)
    buildType(Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseUbSanitizer)
    buildType(Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseThreadSanitizer)
    buildType(Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsRelwithdebinfo)
    buildType(Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseMemorySanitizer)
    buildType(Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseAddressSanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/blockstore/tests/loadtest")
        param("env.TIMEOUT", "25200")
    }
    buildTypesOrder = arrayListOf(Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsRelwithdebinfo, Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseHardening, Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseAddressSanitizer, Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseMemorySanitizer, Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseThreadSanitizer, Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseUbSanitizer)
})
