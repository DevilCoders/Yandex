package Nbs_Build_Stable222.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nbs_Build_Stable222_BuildNbsBinariesYandexCloudTesting : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaPackage)
    name = "Build NBS binaries (yandex-cloud/testing)"

    params {
        param("env.PACKAGES_TO_BUILD", "cloud/blockstore/packages/yandex-cloud-blockstore-client/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-disk-agent/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-http-proxy/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-nbd/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-plugin/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-server/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-server-nodbg/pkg.json")
    }

    steps {
        script {
            name = "Sign packages"
            id = "RUNNER_7920"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                yc-nbs-ci-sign-packages \
                  --teamcity \
                  --task-id ${'$'}TASK_ID \
                  --project-id ${'$'}PROJECT_ID
                  
                sleep 3600
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = "--network host"
        }
        stepsOrder = arrayListOf("RUNNER_6805", "RUNNER_7920")
    }
})
