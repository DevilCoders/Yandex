package Nfs_LocalTests_Stable222_FunctionalTests

import Nfs_LocalTests_Stable222_FunctionalTests.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests_Stable222_FunctionalTests")
    name = "Functional Tests"
    archived = true

    buildType(Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseThreadSanitizer)
    buildType(Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseAddressSanitizer)
    buildType(Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseHardening)
    buildType(Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsRelwithdebinfo)
    buildType(Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseMemorySanitizer)
    buildType(Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseUbSanitizer)

    params {
        param("env.TEST_TARGETS", "cloud/filestore/ci/tests")
    }
    buildTypesOrder = arrayListOf(Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsRelwithdebinfo, Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseHardening, Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseAddressSanitizer, Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseMemorySanitizer, Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseThreadSanitizer, Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseUbSanitizer)
})
