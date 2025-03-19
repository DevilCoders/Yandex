package Nfs_Build_Trunk.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nfs_Build_Trunk_BuildConfigsYandexCloudTesting : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaPackage)
    name = "Build configs (yandex-cloud/testing)"

    params {
        param("env.PACKAGES_TO_BUILD", "cloud/storage/core/packages/yandex-cloud-storage-breakpad/pkg.json;cloud/filestore/packages/yc-nfs-breakpad-systemd/pkg.json;kikimr/testing/packages/yandex-cloud/hw-nbs-dev-lab/global/nfs/yandex-search-kikimr-nfs-conf-hw-nbs-dev-lab-global/pkg.json;kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/nfs/yandex-search-kikimr-nfs-conf-hw-nbs-stable-lab-global/pkg.json;kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/nfs-control/yandex-search-kikimr-nfs-control-conf-hw-nbs-stable-lab-global/pkg.json;kikimr/testing/packages/yandex-cloud/testing/vla/nfs/yandex-search-kikimr-nfs-conf-testing-vla/pkg.json;kikimr/testing/packages/yandex-cloud/testing/sas/nfs/yandex-search-kikimr-nfs-conf-testing-sas/pkg.json;kikimr/testing/packages/yandex-cloud/testing/myt/nfs/yandex-search-kikimr-nfs-conf-testing-myt/pkg.json;")
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
