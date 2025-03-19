package Nbs_LocalTests_Stable222_FunctionalTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseHardening : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaMake)
    name = "Functional Tests (release, hardening)"
    paused = true

    params {
        param("env.DEFINITION_FLAGS", "-DSKIP_JUNK -DUSE_EAT_MY_DATA -DDEBUGINFO_LINES_ONLY -DHARDENING=yes")
    }
})
