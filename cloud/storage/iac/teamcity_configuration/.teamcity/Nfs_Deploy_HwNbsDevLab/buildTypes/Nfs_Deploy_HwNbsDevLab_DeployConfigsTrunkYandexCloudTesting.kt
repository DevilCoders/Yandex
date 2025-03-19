package Nfs_Deploy_HwNbsDevLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_Deploy_HwNbsDevLab_DeployConfigsTrunkYandexCloudTesting : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiDeployPackages)
    name = "Deploy configs (trunk, yandex-cloud/testing)"
    paused = true

    triggers {
        schedule {
            id = "TRIGGER_1457"
            schedulingPolicy = daily {
                hour = 3
            }
            branchFilter = ""
            triggerBuild = always()
        }
    }

    dependencies {
        dependency(Nfs_Build_Trunk.buildTypes.Nfs_Build_Trunk_BuildConfigsYandexCloudTesting) {
            snapshot {
                reuseBuilds = ReuseBuilds.NO
                onDependencyFailure = FailureAction.FAIL_TO_START
                synchronizeRevisions = false
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_1822"
                artifactRules = "built_packages.json=>configs"
            }
        }
    }
})
