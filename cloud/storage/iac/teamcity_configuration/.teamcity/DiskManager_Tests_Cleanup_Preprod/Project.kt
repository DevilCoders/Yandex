package DiskManager_Tests_Cleanup_Preprod

import DiskManager_Tests_Cleanup_Preprod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_Cleanup_Preprod")
    name = "preprod"
    archived = true

    buildType(DiskManager_Tests_Cleanup_Preprod_AcceptanceTest)
    buildType(DiskManager_Tests_Cleanup_Preprod_EternalAcceptanceTest)
    buildType(DiskManager_Tests_Cleanup_Preprod_AcceptanceTestCommon)

    params {
        param("env.CLUSTER", "preprod")
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_Cleanup_Preprod_AcceptanceTestCommon, DiskManager_Tests_Cleanup_Preprod_AcceptanceTest, DiskManager_Tests_Cleanup_Preprod_EternalAcceptanceTest)
})
