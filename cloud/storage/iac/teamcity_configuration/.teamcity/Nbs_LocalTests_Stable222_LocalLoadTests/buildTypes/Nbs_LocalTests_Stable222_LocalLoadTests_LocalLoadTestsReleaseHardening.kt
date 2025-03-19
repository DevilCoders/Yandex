package Nbs_LocalTests_Stable222_LocalLoadTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_LocalTests_Stable222_LocalLoadTests_LocalLoadTestsReleaseHardening : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaMake)
    name = "Local Load Tests (release, hardening)"
    paused = true

    params {
        param("env.DEFINITION_FLAGS", "-DSKIP_JUNK -DUSE_EAT_MY_DATA -DDEBUGINFO_LINES_ONLY -DHARDENING=yes")
    }
})
