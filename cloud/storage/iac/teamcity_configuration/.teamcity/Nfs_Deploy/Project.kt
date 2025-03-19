package Nfs_Deploy

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_Deploy")
    name = "Deploy"

    params {
        param("env.CLUSTER", "")
        param("env.SERVICE", "filestore")
    }

    subProject(Nfs_Deploy_HwNbsDevLab.Project)
    subProject(Nfs_Deploy_HwNbsStableLab.Project)
})
