package Nbs_LocalTests_Stable214_LocalLoadTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_LocalTests_Stable214_LocalLoadTests_LocalLoadTestsReleaseAddressSanitizer : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaMake)
    name = "Local Load Tests (release, address sanitizer)"
    paused = true

    params {
        param("env.SANITIZE", "address")
    }
})
