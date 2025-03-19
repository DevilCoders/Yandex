package Nbs_MigrationTest_HwNbsStableLab

import Nbs_MigrationTest_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_MigrationTest_HwNbsStableLab")
    name = "hw-nbs-stable-lab"

    buildType(Nbs_MigrationTest_HwNbsStableLab_MigrationTest)
    buildType(Nbs_MigrationTest_HwNbsStableLab_MigrationTestRdma)
})
