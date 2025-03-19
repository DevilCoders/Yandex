package Nbs_CorruptionTests

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_CorruptionTests")
    name = "Corruption Tests"
    subProjectsOrder = arrayListOf(RelativeId("Nbs_CorruptionTests_Prod"), RelativeId("Nbs_CorruptionTests_Preprod"), RelativeId("Nbs_CorruptionTests_HwNbsStableLab"), RelativeId("Nbs_CorruptionTests_HwNbsStableLabNoAgentBuilds"), RelativeId("Nbs_CorruptionTests_PreprodNoAgentBuilds"), RelativeId("Nbs_CorruptionTests_ProdNoAgentBuilds"))

    subProject(Nbs_CorruptionTests_HwNbsStableLab.Project)
    subProject(Nbs_CorruptionTests_PreprodNoAgentBuilds.Project)
    subProject(Nbs_CorruptionTests_Prod.Project)
    subProject(Nbs_CorruptionTests_Preprod.Project)
    subProject(Nbs_CorruptionTests_HwNbsStableLabNoAgentBuilds.Project)
    subProject(Nbs_CorruptionTests_ProdNoAgentBuilds.Project)
})
