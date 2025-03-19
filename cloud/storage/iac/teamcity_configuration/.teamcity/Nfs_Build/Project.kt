package Nfs_Build

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_Build")
    name = "Build"
    subProjectsOrder = arrayListOf(RelativeId("Nfs_Build_Trunk"), RelativeId("Nfs_Build_Stable222"), RelativeId("Nfs_Build_Stable214"), RelativeId("Nfs_Build_Stable212"))

    subProject(Nfs_Build_Stable212.Project)
    subProject(Nfs_Build_Stable222.Project)
    subProject(Nfs_Build_Trunk.Project)
    subProject(Nfs_Build_Stable214.Project)
})
