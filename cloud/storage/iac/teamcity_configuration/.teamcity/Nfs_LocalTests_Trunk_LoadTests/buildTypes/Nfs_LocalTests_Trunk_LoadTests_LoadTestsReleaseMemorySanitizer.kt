package Nfs_LocalTests_Trunk_LoadTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Trunk_LoadTests_LoadTestsReleaseMemorySanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Load Tests (release, memory sanitizer)"
    paused = true

    params {
        param("env.TEST_TARGETS", "cloud/filestore/ci/loadtests ")
        param("env.SANITIZE", "memory")
    }
})
