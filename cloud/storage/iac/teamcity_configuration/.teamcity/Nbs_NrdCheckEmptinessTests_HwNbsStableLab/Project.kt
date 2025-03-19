package Nbs_NrdCheckEmptinessTests_HwNbsStableLab

import Nbs_NrdCheckEmptinessTests_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_NrdCheckEmptinessTests_HwNbsStableLab")
    name = "hw-nbs-stable-lab"
    archived = true

    buildType(Nbs_NrdCheckEmptinessTests_HwNbsStableLab_ZeroCheck)

    params {
        param("env.CLUSTER", "prod")
    }
})
