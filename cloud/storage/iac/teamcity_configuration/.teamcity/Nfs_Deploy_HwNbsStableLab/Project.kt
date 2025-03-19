package Nfs_Deploy_HwNbsStableLab

import Nfs_Deploy_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_Deploy_HwNbsStableLab")
    name = "hw-nbs-stable-lab"

    buildType(Nfs_Deploy_HwNbsStableLab_DeployBinariesStable222yandexCloudTesting)
    buildType(Nfs_Deploy_HwNbsStableLab_DeployConfigsTrunkYandexCloudTesting)

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
    }
})
