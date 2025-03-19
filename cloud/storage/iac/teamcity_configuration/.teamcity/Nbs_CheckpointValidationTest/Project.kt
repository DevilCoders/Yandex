package Nbs_CheckpointValidationTest

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_CheckpointValidationTest")
    name = "Checkpoint Validation Test"

    subProject(Nbs_CheckpointValidationTest_HwNbsStableLab.Project)
    subProject(Nbs_CheckpointValidationTest_Preprod.Project)
})
