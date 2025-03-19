package Nbs_FioPerformanceTests_Prod

import Nbs_FioPerformanceTests_Prod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_FioPerformanceTests_Prod")
    name = "prod"
    archived = true

    buildType(Nbs_FioPerformanceTests_Prod_FioPerformanceTestsBlockSize64k)
    buildType(Nbs_FioPerformanceTests_Prod_FioPerformanceTestsNrd)
    buildType(Nbs_FioPerformanceTests_Prod_FioPerformanceTestsMaxBandwidth)
    buildType(Nbs_FioPerformanceTests_Prod_FioPerformanceTestsOverlay)
    buildType(Nbs_FioPerformanceTests_Prod_FioPerformanceTestsDefault)
    buildType(Nbs_FioPerformanceTests_Prod_FioPerformanceTestsMaxIops)

    params {
        param("env.FORCE", "true")
    }

    subProject(Nbs_FioPerformanceTests_Prod_FioPerformanceTestsOverlayBuilds.Project)
})
