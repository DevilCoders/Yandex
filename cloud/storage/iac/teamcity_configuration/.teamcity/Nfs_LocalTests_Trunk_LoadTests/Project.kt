package Nfs_LocalTests_Trunk_LoadTests

import Nfs_LocalTests_Trunk_LoadTests.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests_Trunk_LoadTests")
    name = "Load Tests"
    archived = true

    buildType(Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseThreadSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseAddressSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseMemorySanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseUbSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseHardening)
    buildType(Nfs_LocalTests_Trunk_LoadTests_LoadTestsRelwithdebinfo)

    params {
        param("env.TEST_TARGETS", "cloud/filestore/ci/loadtests ")
        param("env.TIMEOUT", "25200")
    }
    buildTypesOrder = arrayListOf(Nfs_LocalTests_Trunk_LoadTests_LoadTestsRelwithdebinfo, Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseHardening, Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseAddressSanitizer, Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseMemorySanitizer, Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseThreadSanitizer, Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseUbSanitizer)
})
