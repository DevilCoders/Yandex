package Nbs_Deploy_HwNbsStableLab

import Nbs_Deploy_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_Deploy_HwNbsStableLab")
    name = "hw-nbs-stable-lab"

    buildType(Nbs_Deploy_HwNbsStableLab_DeployNbsBinariesStable222yandexCloudTesting)
    buildType(Nbs_Deploy_HwNbsStableLab_DeployNbsConfigsTrunkYandexCloudTesting)

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
    }
})
