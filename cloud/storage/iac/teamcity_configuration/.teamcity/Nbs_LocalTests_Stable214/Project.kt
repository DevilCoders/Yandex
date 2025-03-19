package Nbs_LocalTests_Stable214

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_LocalTests_Stable214")
    name = "Stable-21-4"
    archived = true

    params {
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-21-4")
    }

    subProject(Nbs_LocalTests_Stable214_LocalLoadTests.Project)
    subProject(Nbs_LocalTests_Stable214_FunctionalTests.Project)
})
