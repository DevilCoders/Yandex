package Nbs_WorstCaseReadPerformanceTests_HwNbsStableLab

import Nbs_WorstCaseReadPerformanceTests_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_WorstCaseReadPerformanceTests_HwNbsStableLab")
    name = "hw-nbs-stable-lab"

    buildType(Nbs_WorstCaseReadPerformanceTests_HwNbsStableLab_WorstCaseReadTest)

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
    }
})
