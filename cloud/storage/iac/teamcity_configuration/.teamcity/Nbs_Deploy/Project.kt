package Nbs_Deploy

import Nbs_Deploy.vcsRoots.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_Deploy")
    name = "Deploy"

    vcsRoot(Nbs_Deploy_Infra)

    params {
        param("env.CLUSTER", "")
        param("env.SERVICE", "blockstore")
    }

    subProject(Nbs_Deploy_HwNbsDevLab.Project)
    subProject(Nbs_Deploy_HwNbsStableLab.Project)
})
