package Nfs_LocalTests_Stable222_FunctionalTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Stable222_FunctionalTests_FunctionalTestsRelwithdebinfo : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Functional Tests (relwithdebinfo)"
    paused = true

    params {
        param("env.BUILD_TYPE", "relwithdebinfo")
    }
})
