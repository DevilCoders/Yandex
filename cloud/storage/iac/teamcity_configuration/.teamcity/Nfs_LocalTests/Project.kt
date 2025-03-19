package Nfs_LocalTests

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests")
    name = "Local Tests"

    params {
        param("env.DISK_SPACE", "200")
        param("env.TEST_TARGETS", "")
        param("env.RAM", "64")
        param("env.TIMEOUT", "18000")
        param("env.CORES", "32")
        param("env.ARCADIA_URL", "arcadia:/arc/trunk/arcadia")
        param("env.DISABLE_TEST_TIMEOUT", "true")
    }
    subProjectsOrder = arrayListOf(RelativeId("Nfs_LocalTests_Trunk"), RelativeId("Nfs_LocalTests_Stable222"), RelativeId("Nfs_LocalTests_Stable224"), RelativeId("Nfs_LocalTests_Stable214"), RelativeId("Nfs_LocalTests_Stable212"))

    subProject(Nfs_LocalTests_Stable212.Project)
    subProject(Nfs_LocalTests_Stable222.Project)
    subProject(Nfs_LocalTests_Stable214.Project)
    subProject(Nfs_LocalTests_Stable224.Project)
    subProject(Nfs_LocalTests_Trunk.Project)
})
