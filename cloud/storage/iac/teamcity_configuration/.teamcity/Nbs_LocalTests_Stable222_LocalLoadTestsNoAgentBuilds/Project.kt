package Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds

import Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds")
    name = "Local Load Tests (No-agent builds)"

    buildType(Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseAddressSanitizer)
    buildType(Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseMemorySanitizer)
    buildType(Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseThreadSanitizer)
    buildType(Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseUbSanitizer)
    buildType(Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsRelwithdebinfo)
    buildType(Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseHardening)

    params {
        param("env.TEST_TARGETS", "cloud/blockstore/tests/loadtest")
        param("env.TIMEOUT", "25200")
    }
    buildTypesOrder = arrayListOf(Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsRelwithdebinfo, Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseHardening, Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseAddressSanitizer, Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseMemorySanitizer, Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseThreadSanitizer, Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsReleaseUbSanitizer)
})
