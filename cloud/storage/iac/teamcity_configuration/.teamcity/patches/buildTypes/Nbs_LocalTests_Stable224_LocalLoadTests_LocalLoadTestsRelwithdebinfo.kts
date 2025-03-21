package patches.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.BuildType
import jetbrains.buildServer.configs.kotlin.v2019_2.ui.*

/*
This patch script was generated by TeamCity on settings change in UI.
To apply the patch, create a buildType with id = 'Nbs_LocalTests_Stable224_LocalLoadTests_LocalLoadTestsRelwithdebinfo'
in the project with id = 'Nbs_LocalTests_Stable224_LocalLoadTests', and delete the patch script.
*/
create(RelativeId("Nbs_LocalTests_Stable224_LocalLoadTests"), BuildType({
    templates(RelativeId("Nbs_YcNbsCiRunYaMake"))
    id("Nbs_LocalTests_Stable224_LocalLoadTests_LocalLoadTestsRelwithdebinfo")
    name = "Local Load Tests (relwithdebinfo)"
    paused = true

    params {
        param("env.BUILD_TYPE", "relwithdebinfo")
    }
}))

