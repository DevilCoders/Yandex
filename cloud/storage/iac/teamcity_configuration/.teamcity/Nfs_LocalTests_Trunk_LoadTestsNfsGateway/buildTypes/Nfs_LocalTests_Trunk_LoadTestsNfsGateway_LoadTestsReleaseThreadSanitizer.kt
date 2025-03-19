package Nfs_LocalTests_Trunk_LoadTestsNfsGateway.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseThreadSanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Load Tests (release, thread sanitizer)"
    paused = true

    params {
        param("env.SANDBOX_ENV_VARS", "TSAN_OPTIONS='report_atomic_races=0'")
        param("env.SANITIZE", "thread")
    }
})
