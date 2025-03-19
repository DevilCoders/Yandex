package DiskManager_Tests_Cleanup_Prod

import DiskManager_Tests_Cleanup_Prod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_Cleanup_Prod")
    name = "prod"
    archived = true

    buildType(DiskManager_Tests_Cleanup_Prod_AcceptanceTest)
    buildType(DiskManager_Tests_Cleanup_Prod_AcceptanceTestCommon)
    buildType(DiskManager_Tests_Cleanup_Prod_EternalAcceptanceTest)

    params {
        param("env.CLUSTER", "prod")
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_Cleanup_Prod_AcceptanceTestCommon, DiskManager_Tests_Cleanup_Prod_AcceptanceTest, DiskManager_Tests_Cleanup_Prod_EternalAcceptanceTest)
})
