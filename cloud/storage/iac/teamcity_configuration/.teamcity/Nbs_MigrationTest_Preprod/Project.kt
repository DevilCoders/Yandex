package Nbs_MigrationTest_Preprod

import Nbs_MigrationTest_Preprod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_MigrationTest_Preprod")
    name = "preprod"

    buildType(Nbs_MigrationTest_Preprod_MigrationTest)
    buildType(Nbs_MigrationTest_Preprod_MigrationTestRdma)
})
