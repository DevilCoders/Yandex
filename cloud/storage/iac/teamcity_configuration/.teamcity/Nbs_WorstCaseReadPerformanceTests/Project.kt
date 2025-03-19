package Nbs_WorstCaseReadPerformanceTests

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_WorstCaseReadPerformanceTests")
    name = "Worst Case Read Performance Tests"
    subProjectsOrder = arrayListOf(RelativeId("Nbs_WorstCaseReadPerformanceTests_HwNbsStableLab"), RelativeId("Nbs_WorstCaseReadPerformanceTests_HwNbsStableLabNoAgentBuilds"), RelativeId("Nbs_WorstCaseReadPerformanceTests_PreprodNoAgentBuilds"))

    subProject(Nbs_WorstCaseReadPerformanceTests_HwNbsStableLab.Project)
    subProject(Nbs_WorstCaseReadPerformanceTests_PreprodNoAgentBuilds.Project)
    subProject(Nbs_WorstCaseReadPerformanceTests_HwNbsStableLabNoAgentBuilds.Project)
})
