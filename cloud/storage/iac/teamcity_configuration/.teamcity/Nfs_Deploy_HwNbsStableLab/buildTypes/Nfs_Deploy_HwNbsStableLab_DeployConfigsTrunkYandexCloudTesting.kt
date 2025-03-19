package Nfs_Deploy_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nfs_Deploy_HwNbsStableLab_DeployConfigsTrunkYandexCloudTesting : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiDeployPackages)
    name = "Deploy configs (trunk, yandex-cloud/testing)"

    steps {
        script {
            name = "Deploy packages"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -ex
                
                yc-nbs-ci-deploy-packages \
                  --cluster ${'$'}CLUSTER \
                  --service ${'$'}SERVICE \
                  --packages configs/built_packages.json \
                  --teamcity
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
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
