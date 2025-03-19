package Nfs_Build_Stable222.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_Build_Stable222_BuildBinariesYandexCloudTesting : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaPackage)
    name = "Build binaries (yandex-cloud/testing)"

    params {
        param("env.PACKAGES_TO_BUILD", "cloud/filestore/packages/yandex-cloud-filestore-client/pkg.json;cloud/filestore/packages/yandex-cloud-filestore-server/pkg.json;cloud/filestore/packages/yandex-cloud-filestore-vhost/pkg.json;cloud/filestore/packages/yandex-cloud-filestore-http-proxy/pkg.json;")
    }

    steps {
        script {
            name = "Sign packages"
            id = "RUNNER_11411"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                yc-nbs-ci-sign-packages \
                  --teamcity \
                  --task-id ${'$'}TASK_ID \
                  --project-id ${'$'}PROJECT_ID
                  
                sleep 1200
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = "--network host"
        }
    }

    triggers {
        schedule {
            id = "TRIGGER_1499"
            schedulingPolicy = daily {
                hour = 5
            }
            branchFilter = ""
            triggerBuild = always()
        }
    }
})
