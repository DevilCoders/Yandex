package _Self

import _Self.buildTypes.*
import _Self.vcsRoots.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.projectFeatures.buildReportTab

object Project : Project({
    description = "Umbrella project for other NBS-related projects"

    vcsRoot(YandexCloudBlockstoreSvn)
    vcsRoot(Test)

    template(Nbs_YcNbsCiRunYaMakeAgentless)
    template(Nbs_Cleanup_YcCleanup)

    params {
        param("yc-nbs-ci-tools.docker.image", "registry.yandex.net/yandex-cloud/yc-nbs-ci-tools:latest")
    }

    features {
        buildReportTab {
            id = "PROJECT_EXT_1192"
            title = "[SB] Task status"
            startPage = "task_status.html"
        }
    }

    subProject(Nfs.Project)
    subProject(DiskManager.Project)
    subProject(Nbs.Project)
})
