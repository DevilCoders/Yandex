package Nfs_LocalTests_Trunk_LoadTestsNfsGateway.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseAddressSanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Load Tests (release, address sanitizer)"
    paused = true

    params {
        param("env.SANITIZE", "address")
    }
})
