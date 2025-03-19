package Nbs_LocalTests_Stable222_FunctionalTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsRelwithdebinfo : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaMake)
    name = "Functional Tests (relwithdebinfo)"
    paused = true

    params {
        param("env.BUILD_TYPE", "relwithdebinfo")
    }
})
