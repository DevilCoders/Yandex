package patches.projects

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.ui.*

/*
This patch script was generated by TeamCity on settings change in UI.
To apply the patch, create a project with id = 'Nbs_FioPerformanceTests_ProdNoAgentBuilds_FioPerformanceTestsOverlayBuilds'
in the project with id = 'Nbs_FioPerformanceTests_ProdNoAgentBuilds', and delete the patch script.
*/
create(RelativeId("Nbs_FioPerformanceTests_ProdNoAgentBuilds"), Project({
    id("Nbs_FioPerformanceTests_ProdNoAgentBuilds_FioPerformanceTestsOverlayBuilds")
    name = "Fio Performance Tests (overlay builds)"
    description = "Builds with fio performance tests for overlay disks"
    archived = true

    params {
        param("env.INSTANCE_RAM", "4")
        param("env.TEST_SUITE", "overlay_disk_max_count")
        param("env.IMAGE", "ubuntu1604-stable")
        param("env.INSTANCE_CORES", "4")
        param("env.IN_PARALLEL", "true")
        param("env.FORCE", "true")
    }
}))

