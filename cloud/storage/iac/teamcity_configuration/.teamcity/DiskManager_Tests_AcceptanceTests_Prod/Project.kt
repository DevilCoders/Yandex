package DiskManager_Tests_AcceptanceTests_Prod

import DiskManager_Tests_AcceptanceTests_Prod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_AcceptanceTests_Prod")
    name = "prod"
    archived = true

    buildType(DiskManager_Tests_AcceptanceTests_Prod_AcceptanceTestBig)
    buildType(DiskManager_Tests_AcceptanceTests_Prod_AcceptanceTestSmall)
    buildType(DiskManager_Tests_AcceptanceTests_Prod_AcceptanceTestEnormous)
    buildType(DiskManager_Tests_AcceptanceTests_Prod_AcceptanceTestMedium)

    params {
        param("env.CLUSTER", "prod")
        param("env.INSTANCE_RAM", "8")
        param("env.INSTANCE_CORES", "8")
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_AcceptanceTests_Prod_AcceptanceTestSmall, DiskManager_Tests_AcceptanceTests_Prod_AcceptanceTestMedium, DiskManager_Tests_AcceptanceTests_Prod_AcceptanceTestBig, DiskManager_Tests_AcceptanceTests_Prod_AcceptanceTestEnormous)
})
