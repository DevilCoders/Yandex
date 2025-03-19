package Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway

import Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway")
    name = "Functional Tests NFS Gateway"
    archived = true

    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseUbSanitizer)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsRelwithdebinfo)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseAddressSanitizer)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseThreadSanitizer)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseHardening)
    buildType(Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseMemorySanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/filestore/ci/gateway-nfs-tests")
        param("env.CONTAINER_RESOURCE", "2273379275")
    }
    buildTypesOrder = arrayListOf(Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsRelwithdebinfo, Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseHardening, Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseAddressSanitizer, Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseMemorySanitizer, Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseThreadSanitizer, Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseUbSanitizer)
})
