package Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseThreadSanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Functional Tests (release, thread sanitizer)"
    paused = true

    params {
        param("env.SANDBOX_ENV_VARS", "TSAN_OPTIONS='report_atomic_races=0'")
        param("env.SANITIZE", "thread")
    }
})
