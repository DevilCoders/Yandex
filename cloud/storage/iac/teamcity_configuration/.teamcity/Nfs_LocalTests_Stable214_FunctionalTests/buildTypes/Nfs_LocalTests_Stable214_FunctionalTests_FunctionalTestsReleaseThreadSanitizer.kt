package Nfs_LocalTests_Stable214_FunctionalTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseThreadSanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Functional Tests (release, thread sanitizer)"
    paused = true

    params {
        param("env.SANITIZE", "thread")
    }
})
