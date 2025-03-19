package Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds

import Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds")
    name = "Fio Performance Tests (overlay builds)"
    description = "Builds with fio performance tests for overlay disks"
    archived = true

    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay1)
    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay2)
    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay3)
    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay4)
    buildType(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay5)

    params {
        param("env.INSTANCE_RAM", "4")
        param("env.TEST_SUITE", "overlay_disk_max_count")
        param("env.IMAGE", "ubuntu1604-stable")
        param("env.INSTANCE_CORES", "4")
        param("env.IN_PARALLEL", "true")
        param("env.FORCE", "true")
    }
})
