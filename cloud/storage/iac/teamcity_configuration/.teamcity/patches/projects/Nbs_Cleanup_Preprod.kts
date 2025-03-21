package patches.projects

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.ui.*

/*
This patch script was generated by TeamCity on settings change in UI.
To apply the patch, create a project with id = 'Nbs_Cleanup_Preprod'
in the project with id = 'Nbs_Cleanup', and delete the patch script.
*/
create(RelativeId("Nbs_Cleanup"), Project({
    id("Nbs_Cleanup_Preprod")
    name = "preprod"
    archived = true

    params {
        param("env.CLUSTER", "preprod")
    }
}))

