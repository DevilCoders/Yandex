package Nfs_LocalTests_Stable212_FunctionalTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Stable212_FunctionalTests_FunctionalTestsReleaseAddressSanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Functional Tests (release, address sanitizer)"
    paused = true

    params {
        param("env.SANITIZE", "address")
    }
})
