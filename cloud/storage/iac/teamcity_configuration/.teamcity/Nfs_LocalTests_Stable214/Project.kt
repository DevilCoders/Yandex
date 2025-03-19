package Nfs_LocalTests_Stable214

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests_Stable214")
    name = "Stable-21-4"
    archived = true

    params {
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-21-4")
    }

    subProject(Nfs_LocalTests_Stable214_LoadTests.Project)
    subProject(Nfs_LocalTests_Stable214_FunctionalTests.Project)
})
