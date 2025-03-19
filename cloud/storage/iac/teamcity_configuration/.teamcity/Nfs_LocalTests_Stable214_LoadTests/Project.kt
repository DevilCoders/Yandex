package Nfs_LocalTests_Stable214_LoadTests

import Nfs_LocalTests_Stable214_LoadTests.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests_Stable214_LoadTests")
    name = "Load Tests"
    archived = true

    buildType(Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseMemorySanitizer)
    buildType(Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseUbSanitizer)
    buildType(Nfs_LocalTests_Stable214_LoadTests_LoadTestsRelwithdebinfo)
    buildType(Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseThreadSanitizer)
    buildType(Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseAddressSanitizer)
    buildType(Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseHardening)

    params {
        param("env.TIMEOUT", "25200")
    }
    buildTypesOrder = arrayListOf(Nfs_LocalTests_Stable214_LoadTests_LoadTestsRelwithdebinfo, Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseHardening, Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseAddressSanitizer, Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseMemorySanitizer, Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseThreadSanitizer, Nfs_LocalTests_Stable214_LoadTests_LoadTestsReleaseUbSanitizer)
})
