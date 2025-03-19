package Nfs_LocalTests_Stable212

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests_Stable212")
    name = "Stable-21-2"
    archived = true

    params {
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-21-2")
    }

    subProject(Nfs_LocalTests_Stable212_LoadTests.Project)
    subProject(Nfs_LocalTests_Stable212_FunctionalTests.Project)
})
