package Nbs

import Nbs.buildTypes.*
import Nbs.vcsRoots.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project
import jetbrains.buildServer.configs.kotlin.v2019_2.sharedResource

object Project : Project({
    id("Nbs")
    name = "NBS"

    vcsRoot(Nbs_Infra)

    template(Nbs_YcNbsCiCheckNrdEmptinessTest)
    template(Nbs_YcNbsCiRunYaMake2)
    template(Nbs_YcNbsCiCorruptionTestSuite)
    template(Nbs_YcNbsCiDeployPackages)
    template(Nbs_YcNbsCiCheckpointValidationTest)
    template(Nbs_YcNbsCiRunYaPackage)
    template(Nbs_YcNbsCiRunYaMake)
    template(Nbs_YcNbsCiWorstCaseReadPerformanceTestSuite)
    template(Nbs_YcNbsCiMigrationTest)
    template(Nbs_YcNbsCiFioPerformanceTestSuite)

    params {
        param("yc-nbs-ci-tools.docker.image", "registry.yandex.net/yandex-cloud/yc-nbs-ci-tools:latest")
    }

    features {
        sharedResource {
            id = "PROJECT_EXT_813"
            name = "ci_control_builds"
            enabled = true
            resourceType = infinite()
        }
    }
    subProjectsOrder = arrayListOf(RelativeId("Nbs_LocalTests"), RelativeId("Nbs_Build"), RelativeId("Nbs_Deploy"), RelativeId("Nbs_CorruptionTests"), RelativeId("Nbs_NrdCheckEmptinessTests"), RelativeId("Nbs_WorstCaseReadPerformanceTests"), RelativeId("Nbs_CheckpointValidationTest"), RelativeId("Nbs_FioPerformanceTests"), RelativeId("Nbs_MigrationTest"), RelativeId("Nbs_Archive"))

    subProject(Nbs_CorruptionTests.Project)
    subProject(Nbs_CheckpointValidationTest.Project)
    subProject(Nbs_NrdCheckEmptinessTests.Project)
    subProject(Nbs_Deploy.Project)
    subProject(Nbs_WorstCaseReadPerformanceTests.Project)
    subProject(Nbs_LocalTests.Project)
    subProject(Nbs_Build.Project)
    subProject(Nbs_FioPerformanceTests.Project)
    subProject(Nbs_MigrationTest.Project)
})
