package Nbs_FioPerformanceTests

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_FioPerformanceTests")
    name = "Fio Performance Tests"
    subProjectsOrder = arrayListOf(RelativeId("Nbs_FioPerformanceTests_Prod"), RelativeId("Nbs_FioPerformanceTests_Preprod"), RelativeId("Nbs_FioPerformanceTests_HwNbsStableLab"), RelativeId("Nbs_FioPerformanceTests_HwNbsStableLabNoAgentBuilds"), RelativeId("Nbs_FioPerformanceTests_PreprodNoAgentBuilds"), RelativeId("Nbs_FioPerformanceTests_ProdNoAgentBuilds"))

    subProject(Nbs_FioPerformanceTests_ProdNoAgentBuilds.Project)
    subProject(Nbs_FioPerformanceTests_Prod.Project)
    subProject(Nbs_FioPerformanceTests_PreprodNoAgentBuilds.Project)
    subProject(Nbs_FioPerformanceTests_Preprod.Project)
    subProject(Nbs_FioPerformanceTests_HwNbsStableLab.Project)
    subProject(Nbs_FioPerformanceTests_HwNbsStableLabNoAgentBuilds.Project)
})
