package Nfs_BuildArcadiaTest

import Nfs_BuildArcadiaTest.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_BuildArcadiaTest")
    name = "Build Arcadia Test"
    archived = true

    buildType(Nfs_BuildArcadiaTest_HwNbsStableLab)
    buildType(Nfs_BuildArcadiaTest_Prod)
    buildType(Nfs_BuildArcadiaTest_Preprod)
})
