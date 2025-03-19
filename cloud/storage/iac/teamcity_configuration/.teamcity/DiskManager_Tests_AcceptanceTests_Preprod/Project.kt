package DiskManager_Tests_AcceptanceTests_Preprod

import DiskManager_Tests_AcceptanceTests_Preprod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_AcceptanceTests_Preprod")
    name = "preprod"
    archived = true

    buildType(DiskManager_Tests_AcceptanceTests_Preprod_AcceptanceTestMedium)
    buildType(DiskManager_Tests_AcceptanceTests_Preprod_AcceptanceTestEnormous)
    buildType(DiskManager_Tests_AcceptanceTests_Preprod_AcceptanceTestBig)
    buildType(DiskManager_Tests_AcceptanceTests_Preprod_AcceptanceTestSmall)

    params {
        param("env.CLUSTER", "preprod")
        param("env.INSTANCE_RAM", "8")
        param("env.INSTANCE_CORES", "8")
    }
    buildTypesOrder = arrayListOf(DiskManager_Tests_AcceptanceTests_Preprod_AcceptanceTestSmall, DiskManager_Tests_AcceptanceTests_Preprod_AcceptanceTestMedium, DiskManager_Tests_AcceptanceTests_Preprod_AcceptanceTestBig, DiskManager_Tests_AcceptanceTests_Preprod_AcceptanceTestEnormous)
})
