package Nfs_LocalTests_Trunk_LoadTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseUbSanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Load Tests (release, UB sanitizer)"
    paused = true

    params {
        param("env.TEST_TARGETS", "cloud/filestore/ci/loadtests ")
        param("env.SANITIZE", "undefined")
    }
})
