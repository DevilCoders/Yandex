package Nfs_LocalTests_Stable222

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests_Stable222")
    name = "Stable-22-2"

    params {
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-22-2")
        param("branch_name", "stable_22_2")
    }

    subProject(Nfs_LocalTests_Stable222_FunctionalTests.Project)
    subProject(Nfs_LocalTests_Stable222_LoadTestsNoAgentBuilds.Project)
    subProject(Nfs_LocalTests_Stable222_LoadTests.Project)
    subProject(Nfs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds.Project)
})
