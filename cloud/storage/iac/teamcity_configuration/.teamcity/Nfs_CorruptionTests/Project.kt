package Nfs_CorruptionTests

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_CorruptionTests")
    name = "Corruption Tests"

    subProject(Nfs_CorruptionTests_HwNbsStableLab.Project)
    subProject(Nfs_CorruptionTests_Prod.Project)
    subProject(Nfs_CorruptionTests_Preprod.Project)
})
