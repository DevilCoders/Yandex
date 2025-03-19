package Nbs_FioPerformanceTests_HwNbsStableLab

import Nbs_FioPerformanceTests_HwNbsStableLab.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_FioPerformanceTests_HwNbsStableLab")
    name = "hw-nbs-stable-lab"
    archived = true

    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsBlockSize32)
    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsMaxIops)
    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsDefault)
    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlay)
    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsNrd)
    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsMaxBandwidth)

    params {
        param("env.FORCE", "false")
        param("env.CLUSTER", "hw-nbs-stable-lab")
        param("env.DEBUG", "false")
    }

    subProject(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds.Project)
})
