package Nfs

import Nfs.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.projectFeatures.buildReportTab

object Project : Project({
    id("Nfs")
    name = "NFS"

    template(Nfs_YcNbsCiRunYaMake)
    template(Nfs_YcNfsCiCorruptionTestSuite)
    template(Nfs_YcNbsCiRunYaPackage)
    template(Nfs_YcNbsCiDeployPackages)
    template(Nfs_YcNfsCiBuildArcadiaTest)

    params {
        param("yc-nbs-ci-tools.docker.image", "registry.yandex.net/yandex-cloud/yc-nbs-ci-tools:latest")
    }

    features {
        buildReportTab {
            id = "PROJECT_EXT_1178"
            title = "[SB] Task status"
            startPage = "task_status.html"
        }
    }
    subProjectsOrder = arrayListOf(RelativeId("Nfs_LocalTests"), RelativeId("Nfs_CorruptionTests"), RelativeId("Nfs_BuildArcadiaTestNoAgentBuilds"), RelativeId("Nfs_Build"), RelativeId("Nfs_Deploy"), RelativeId("Nfs_BuildArcadiaTest"))

    subProject(Nfs_LocalTests.Project)
    subProject(Nfs_Build.Project)
    subProject(Nfs_BuildArcadiaTestNoAgentBuilds.Project)
    subProject(Nfs_BuildArcadiaTest.Project)
    subProject(Nfs_CorruptionTests.Project)
    subProject(Nfs_Deploy.Project)
})
