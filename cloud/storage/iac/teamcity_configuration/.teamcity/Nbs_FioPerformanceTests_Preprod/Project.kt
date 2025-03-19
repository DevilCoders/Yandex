package Nbs_FioPerformanceTests_Preprod

import Nbs_FioPerformanceTests_Preprod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_FioPerformanceTests_Preprod")
    name = "preprod"
    archived = true

    buildType(Nbs_FioPerformanceTests_Preprod_FioPerformanceTestsBlockSize64k)
    buildType(Nbs_FioPerformanceTests_Preprod_FioPerformanceTestsMaxBandwidth)
    buildType(Nbs_FioPerformanceTests_Preprod_FioPerformanceTestsDefault)
    buildType(Nbs_FioPerformanceTests_Preprod_FioPerformanceTestsOverlay)
    buildType(Nbs_FioPerformanceTests_Preprod_FioPerformanceTestsNrd)
    buildType(Nbs_FioPerformanceTests_Preprod_FioPerformanceTestsMaxIops)

    params {
        param("env.FORCE", "true")
        param("env.CLUSTER", "preprod")
    }

    subProject(Nbs_FioPerformanceTests_Preprod_FioPerformanceTestsOverlayBuilds.Project)
})
