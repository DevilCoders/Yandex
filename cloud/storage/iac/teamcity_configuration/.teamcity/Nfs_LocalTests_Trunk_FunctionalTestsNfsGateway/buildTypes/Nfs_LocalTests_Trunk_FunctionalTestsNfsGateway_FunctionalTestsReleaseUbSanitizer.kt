package Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway_FunctionalTestsReleaseUbSanitizer : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Functional Tests (release, UB sanitizer)"
    paused = true

    params {
        param("env.SANITIZE", "undefined")
    }
})
