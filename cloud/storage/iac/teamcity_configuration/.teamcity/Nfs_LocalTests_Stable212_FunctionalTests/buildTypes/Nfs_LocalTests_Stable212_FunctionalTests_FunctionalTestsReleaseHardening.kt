package Nfs_LocalTests_Stable212_FunctionalTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Stable212_FunctionalTests_FunctionalTestsReleaseHardening : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Functional Tests (release, hardening)"
    paused = true

    params {
        param("env.DEFINITION_FLAGS", "-DSKIP_JUNK -DUSE_EAT_MY_DATA -DDEBUGINFO_LINES_ONLY -DHARDENING=yes")
    }
})
