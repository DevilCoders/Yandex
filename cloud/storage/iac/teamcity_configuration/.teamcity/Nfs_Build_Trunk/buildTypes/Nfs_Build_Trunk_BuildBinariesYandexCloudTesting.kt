package Nfs_Build_Trunk.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nfs_Build_Trunk_BuildBinariesYandexCloudTesting : BuildType({
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
})
