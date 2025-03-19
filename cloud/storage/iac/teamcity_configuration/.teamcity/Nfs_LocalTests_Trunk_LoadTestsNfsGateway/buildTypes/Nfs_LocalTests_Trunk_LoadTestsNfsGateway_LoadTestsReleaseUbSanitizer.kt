package Nfs_LocalTests_Trunk_LoadTestsNfsGateway.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseUbSanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Load Tests (release, UB sanitizer)"
    paused = true

    params {
        param("env.SANITIZE", "undefined")
    }
})
