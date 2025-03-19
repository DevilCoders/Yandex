package Nbs_LocalTests_Stable222_LocalLoadTests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_LocalTests_Stable222_LocalLoadTests_LocalLoadTestsReleaseThreadSanitizer : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaMake)
    name = "Local Load Tests (release, thread sanitizer)"
    paused = true

    params {
        param("env.SANDBOX_ENV_VARS", "TSAN_OPTIONS='report_atomic_races=0'")
        param("env.SANITIZE", "thread")
    }
})
