package Nbs_Deploy_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nbs_Deploy_HwNbsStableLab_DeployNbsBinariesStable222yandexCloudTesting : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiDeployPackages)
    name = "Deploy NBS binaries (stable-22-2, yandex-cloud/testing)"

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
                  --packages binaries/built_packages.json \
                  --teamcity
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = "--network host"
        }
    }

    dependencies {
        dependency(Nbs_Build_Stable222.buildTypes.Nbs_Build_Stable222_BuildNbsBinariesYandexCloudTesting) {
            snapshot {
                reuseBuilds = ReuseBuilds.NO
                onDependencyFailure = FailureAction.FAIL_TO_START
                synchronizeRevisions = false
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_1825"
                artifactRules = "built_packages.json=>binaries"
            }
        }
    }
})
