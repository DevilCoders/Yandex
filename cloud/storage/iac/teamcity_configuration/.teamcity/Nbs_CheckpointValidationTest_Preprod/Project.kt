package Nbs_CheckpointValidationTest_Preprod

import Nbs_CheckpointValidationTest_Preprod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_CheckpointValidationTest_Preprod")
    name = "preprod"

    buildType(Nbs_CheckpointValidationTest_Preprod_CheckpointValidationTest)
})
