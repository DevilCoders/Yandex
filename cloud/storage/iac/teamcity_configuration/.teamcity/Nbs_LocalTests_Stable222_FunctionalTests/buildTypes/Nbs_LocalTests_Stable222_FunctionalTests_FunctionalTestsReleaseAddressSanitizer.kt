package Nbs_LocalTests_Stable222_FunctionalTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_LocalTests_Stable222_FunctionalTests_FunctionalTestsReleaseAddressSanitizer : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaMake)
    name = "Functional Tests (release, address sanitizer)"
    paused = true

    params {
        param("env.DISK_SPACE", "300")
        param("env.SANITIZE", "address")
    }
})
