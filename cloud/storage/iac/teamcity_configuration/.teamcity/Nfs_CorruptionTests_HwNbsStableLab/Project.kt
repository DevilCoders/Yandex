package Nfs_CorruptionTests_HwNbsStableLab

import Nfs_CorruptionTests_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_CorruptionTests_HwNbsStableLab")
    name = "hw-nbs-stable-lab"
    archived = true

    buildType(Nfs_CorruptionTests_HwNbsStableLab_CorruptionTests4kbBlocksize)
    buildType(Nfs_CorruptionTests_HwNbsStableLab_CorruptionTests64mbBlocksize)
})
