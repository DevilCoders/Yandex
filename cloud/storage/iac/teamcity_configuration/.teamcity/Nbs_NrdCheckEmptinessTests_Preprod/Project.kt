package Nbs_NrdCheckEmptinessTests_Preprod

import Nbs_NrdCheckEmptinessTests_Preprod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_NrdCheckEmptinessTests_Preprod")
    name = "preprod"
    archived = true
    defaultTemplate = RelativeId("Nbs_YcNbsCiCheckNrdEmptinessTest")

    buildType(Nbs_NrdCheckEmptinessTests_Preprod_ZeroCheck)

    params {
        param("env.CLUSTER", "preprod")
    }
})
