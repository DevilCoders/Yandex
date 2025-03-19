package Nfs_Deploy_HwNbsDevLab

import Nfs_Deploy_HwNbsDevLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_Deploy_HwNbsDevLab")
    name = "hw-nbs-dev-lab"
    archived = true

    buildType(Nfs_Deploy_HwNbsDevLab_DeployConfigsTrunkYandexCloudTesting)

    params {
        param("env.CLUSTER", "hw-nbs-dev-lab")
    }
})
