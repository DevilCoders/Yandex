package Nbs_CorruptionTests_HwNbsStableLab

import Nbs_CorruptionTests_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_CorruptionTests_HwNbsStableLab")
    name = "hw-nbs-stable-lab"
    archived = true

    buildType(Nbs_CorruptionTests_HwNbsStableLab_CorruptionTestsRangesIntersection)
    buildType(Nbs_CorruptionTests_HwNbsStableLab_CorruptionTests512bytesBlocksize)
    buildType(Nbs_CorruptionTests_HwNbsStableLab_CorruptionTests64mbBlocksize)

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
    }
})
