package Nfs_LocalTests_Trunk_LoadTestsNfsGateway

import Nfs_LocalTests_Trunk_LoadTestsNfsGateway.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests_Trunk_LoadTestsNfsGateway")
    name = "Load Tests NFS Gateway"
    archived = true

    buildType(Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseHardening)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsRelwithdebinfo)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseAddressSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseThreadSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseUbSanitizer)
    buildType(Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseMemorySanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/filestore/ci/gateway-nfs-loadtests")
        param("env.TIMEOUT", "25200")
        param("env.CONTAINER_RESOURCE", "2273379275")
    }
    buildTypesOrder = arrayListOf(Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsRelwithdebinfo, Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseHardening, Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseAddressSanitizer, Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseMemorySanitizer, Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseThreadSanitizer, Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseUbSanitizer)
})
