package Nbs_LocalTests_Stable222

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_LocalTests_Stable222")
    name = "Stable-22-2"

    params {
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-22-2")
        param("branch_name", "stable_22_2")
    }

    subProject(Nbs_LocalTests_Stable222_FunctionalTests.Project)
    subProject(Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds.Project)
    subProject(Nbs_LocalTests_Stable222_LocalLoadTests.Project)
    subProject(Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds.Project)
})
