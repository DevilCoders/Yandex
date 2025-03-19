package Nbs_MigrationTest

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_MigrationTest")
    name = "Migration Test"

    subProject(Nbs_MigrationTest_HwNbsStableLab.Project)
    subProject(Nbs_MigrationTest_Preprod.Project)
})
