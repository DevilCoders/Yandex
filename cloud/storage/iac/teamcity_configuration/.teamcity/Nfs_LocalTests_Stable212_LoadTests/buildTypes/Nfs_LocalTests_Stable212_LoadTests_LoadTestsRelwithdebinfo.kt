package Nfs_LocalTests_Stable212_LoadTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Stable212_LoadTests_LoadTestsRelwithdebinfo : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Load Tests (relwithdebinfo)"
    paused = true

    params {
        param("env.TEST_TARGETS", "cloud/filestore/ci/loadtests ")
        param("env.BUILD_TYPE", "relwithdebinfo")
    }
})
