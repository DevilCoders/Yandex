package Nfs_LocalTests_Stable212_LoadTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Stable212_LoadTests_LoadTestsReleaseHardening : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Load Tests (release, hardening)"
    paused = true

    params {
        param("env.DEFINITION_FLAGS", "-DSKIP_JUNK -DUSE_EAT_MY_DATA -DDEBUGINFO_LINES_ONLY -DHARDENING=yes")
        param("env.TEST_TARGETS", "cloud/filestore/ci/loadtests ")
    }
})
