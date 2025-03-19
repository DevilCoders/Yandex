package Nfs_LocalTests_Trunk_FunctionalTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Trunk_FunctionalTests_FunctionalTestsReleaseMemorySanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Functional Tests (release, memory sanitizer)"
    paused = true

    params {
        param("env.DISK_SPACE", "300")
        param("env.SANITIZE", "memory")
    }
})
