package Nbs_Build

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_Build")
    name = "Build"
    subProjectsOrder = arrayListOf(RelativeId("Nbs_Build_Trunk"), RelativeId("Nbs_Build_Stable222"), RelativeId("Nbs_Build_Stable214"))

    subProject(Nbs_Build_Stable222.Project)
    subProject(Nbs_Build_Trunk.Project)
    subProject(Nbs_Build_Stable214.Project)
})
