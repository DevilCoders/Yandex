package Nbs_Deploy_HwNbsDevLab

import Nbs_Deploy_HwNbsDevLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_Deploy_HwNbsDevLab")
    name = "hw-nbs-dev-lab"

    buildType(Nbs_Deploy_HwNbsDevLab_DeployNbsConfigsTrunkYandexCloudTesting)
    buildType(Nbs_Deploy_HwNbsDevLab_DeployNbsBinariesTrunkYandexCloudTesting)

    params {
        param("env.CLUSTER", "hw-nbs-dev-lab")
    }
})
