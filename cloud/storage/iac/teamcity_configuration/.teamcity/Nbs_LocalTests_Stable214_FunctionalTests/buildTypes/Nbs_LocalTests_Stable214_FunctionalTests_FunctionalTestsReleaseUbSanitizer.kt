package Nbs_LocalTests_Stable214_FunctionalTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_LocalTests_Stable214_FunctionalTests_FunctionalTestsReleaseUbSanitizer : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaMake)
    name = "Functional Tests (release, UB sanitizer)"
    paused = true

    params {
        param("env.DISK_SPACE", "300")
        param("env.SANITIZE", "undefined")
    }
})
