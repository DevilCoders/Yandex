package Nbs_LocalTests_Stable222_LocalLoadTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_LocalTests_Stable222_LocalLoadTests_LocalLoadTestsRelwithdebinfo : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaMake)
    name = "Local Load Tests (relwithdebinfo)"
    paused = true

    params {
        param("env.BUILD_TYPE", "relwithdebinfo")
    }
})
