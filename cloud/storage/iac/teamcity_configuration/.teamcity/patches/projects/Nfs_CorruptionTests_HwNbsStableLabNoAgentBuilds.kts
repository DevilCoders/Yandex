package patches.projects

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.ui.*

/*
This patch script was generated by TeamCity on settings change in UI.
To apply the patch, create a project with id = 'Nfs_CorruptionTests_HwNbsStableLabNoAgentBuilds'
in the project with id = 'Nfs_CorruptionTests', and delete the patch script.
*/
create(RelativeId("Nfs_CorruptionTests"), Project({
    id("Nfs_CorruptionTests_HwNbsStableLabNoAgentBuilds")
    name = "hw-nbs-stable-lab (No-agent builds)"

    params {
        param("cluster", "hw_nbs_stable_lab")
    }
}))

