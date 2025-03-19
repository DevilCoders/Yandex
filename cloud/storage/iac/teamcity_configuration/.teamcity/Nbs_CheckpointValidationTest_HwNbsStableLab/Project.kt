package Nbs_CheckpointValidationTest_HwNbsStableLab

import Nbs_CheckpointValidationTest_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_CheckpointValidationTest_HwNbsStableLab")
    name = "hw-nbs-stable-lab"

    buildType(Nbs_CheckpointValidationTest_HwNbsStableLab_CheckpointValidationTest)
})
