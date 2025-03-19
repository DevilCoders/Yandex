package Nbs_NrdCheckEmptinessTests

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_NrdCheckEmptinessTests")
    name = "NRD Check Emptiness Tests"

    cleanup {
        keepRule {
            id = "KEEP_RULE_23"
            keepAtLeast = days(548)
            dataToKeep = everything()
            applyPerEachBranch = true
            preserveArtifactsDependencies = true
        }
        baseRule {
            option("disableCleanupPolicies", true)
        }
    }
    subProjectsOrder = arrayListOf(RelativeId("Nbs_NrdCheckEmptinessTests_Prod"), RelativeId("Nbs_NrdCheckEmptinessTests_Preprod"), RelativeId("Nbs_NrdCheckEmptinessTests_HwNbsStableLab"), RelativeId("Nbs_NrdCheckEmptinessTests_HwNbsStableLabNoAgentBuilds"), RelativeId("Nbs_NrdCheckEmptinessTests_PreprodNoAgentBuilds"), RelativeId("Nbs_NrdCheckEmptinessTests_ProdNoAgentBuilds"))

    subProject(Nbs_NrdCheckEmptinessTests_Prod.Project)
    subProject(Nbs_NrdCheckEmptinessTests_HwNbsStableLab.Project)
    subProject(Nbs_NrdCheckEmptinessTests_Preprod.Project)
    subProject(Nbs_NrdCheckEmptinessTests_HwNbsStableLabNoAgentBuilds.Project)
    subProject(Nbs_NrdCheckEmptinessTests_PreprodNoAgentBuilds.Project)
    subProject(Nbs_NrdCheckEmptinessTests_ProdNoAgentBuilds.Project)
})
