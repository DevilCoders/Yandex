package Nbs_Build_Trunk.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nbs_Build_Trunk_BuildNbsBinariesYandexCloudTesting : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaPackage)
    name = "Build NBS binaries (yandex-cloud/testing)"

    params {
        param("env.PACKAGES_TO_BUILD", "cloud/blockstore/packages/yandex-cloud-blockstore-client/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-disk-agent/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-http-proxy/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-nbd/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-plugin/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-server/pkg.json;cloud/blockstore/packages/yandex-cloud-blockstore-server-nodbg/pkg.json")
    }

    steps {
        script {
            name = "Build packages"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                echo ${'$'}PACKAGES_TO_BUILD | sed 's/;/\n/g' > packages_to_build.txt
                
                yc-nbs-ci-run-ya-package \
                  --in packages_to_build.txt \
                  --out built_packages.json \
                  --teamcity \
                  --timeout ${'$'}TIMEOUT \
                  --arcadia-url ${'$'}ARCADIA_URL \
                  --publish-to ${'$'}PUBLISH_TO \
                  --debian-distribution ${'$'}DEBIAN_DISTRIBUTION
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = "--network host"
        }
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
    }
})
