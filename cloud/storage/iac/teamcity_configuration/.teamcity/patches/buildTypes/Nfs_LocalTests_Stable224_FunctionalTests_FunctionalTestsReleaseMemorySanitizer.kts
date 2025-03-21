package patches.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.BuildType
import jetbrains.buildServer.configs.kotlin.v2019_2.ui.*

/*
This patch script was generated by TeamCity on settings change in UI.
To apply the patch, create a buildType with id = 'Nfs_LocalTests_Stable224_FunctionalTests_FunctionalTestsReleaseMemorySanitizer'
in the project with id = 'Nfs_LocalTests_Stable224_FunctionalTests', and delete the patch script.
*/
create(RelativeId("Nfs_LocalTests_Stable224_FunctionalTests"), BuildType({
    templates(RelativeId("Nfs_YcNbsCiRunYaMake"))
    id("Nfs_LocalTests_Stable224_FunctionalTests_FunctionalTestsReleaseMemorySanitizer")
    name = "Functional Tests (release, memory sanitizer)"
    paused = true

    params {
        param("env.DISK_SPACE", "300")
        param("env.SANITIZE", "memory")
    }
}))

