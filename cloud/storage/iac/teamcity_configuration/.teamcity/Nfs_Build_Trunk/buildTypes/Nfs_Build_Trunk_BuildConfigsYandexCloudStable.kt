package Nfs_Build_Trunk.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nfs_Build_Trunk_BuildConfigsYandexCloudStable : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaPackage)
    name = "Build configs (yandex-cloud/stable)"

    params {
        param("env.DEBIAN_DISTRIBUTION", "stable")
        param("env.PACKAGES_TO_BUILD", """
            cloud/storage/core/packages/yandex-cloud-storage-breakpad/pkg.json
            cloud/filestore/packages/yc-nfs-http-proxy-systemd/pkg.json
            cloud/filestore/packages/yc-nfs-breakpad-systemd/pkg.json
            kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/nfs/yandex-search-kikimr-nfs-conf-hw-nbs-stable-lab-global/pkg.json
            kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/nfs-control/yandex-search-kikimr-nfs-control-conf-hw-nbs-stable-lab-global/pkg.json
            kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/nfs-control/yandex-search-kikimr-nfs-control-conf-hw-nbs-stable-lab-global/pkg.json
            kikimr/production/packages/yandex-cloud/preprod/myt/nfs/yandex-search-kikimr-nfs-conf-preprod-myt/pkg.json
            kikimr/production/packages/yandex-cloud/preprod/sas/nfs/yandex-search-kikimr-nfs-conf-preprod-sas/pkg.json
            kikimr/production/packages/yandex-cloud/preprod/vla/nfs/yandex-search-kikimr-nfs-conf-preprod-vla/pkg.json
            kikimr/production/packages/yandex-cloud/preprod/myt/nfs-control/yandex-search-kikimr-nfs-control-conf-preprod-myt/pkg.json
            kikimr/production/packages/yandex-cloud/preprod/sas/nfs-control/yandex-search-kikimr-nfs-control-conf-preprod-sas/pkg.json
            kikimr/production/packages/yandex-cloud/preprod/vla/nfs-control/yandex-search-kikimr-nfs-control-conf-preprod-vla/pkg.json
            kikimr/production/packages/yandex-cloud/prod/myt/nfs/yandex-search-kikimr-nfs-conf-prod-myt/pkg.json
            kikimr/production/packages/yandex-cloud/prod/sas/nfs/yandex-search-kikimr-nfs-conf-prod-sas/pkg.json
            kikimr/production/packages/yandex-cloud/prod/vla/nfs/yandex-search-kikimr-nfs-conf-prod-vla/pkg.json
            kikimr/production/packages/yandex-cloud/prod/myt/nfs-control/yandex-search-kikimr-nfs-control-conf-prod-myt/pkg.json
            kikimr/production/packages/yandex-cloud/prod/sas/nfs-control/yandex-search-kikimr-nfs-control-conf-prod-sas/pkg.json
            kikimr/production/packages/yandex-cloud/prod/vla/nfs-control/yandex-search-kikimr-nfs-control-conf-prod-vla/pkg.json
            kikimr/testing/packages/yandex-cloud/testing/vla/nfs/yandex-search-kikimr-nfs-conf-testing-vla/pkg.json
            kikimr/testing/packages/yandex-cloud/testing/myt/nfs/yandex-search-kikimr-nfs-conf-testing-myt/pkg.json
            kikimr/testing/packages/yandex-cloud/testing/sas/nfs/yandex-search-kikimr-nfs-conf-testing-sas/pkg.json
            kikimr/testing/packages/yandex-cloud/testing/vla/nfs-control/yandex-search-kikimr-nfs-control-conf-testing-vla/pkg.json
            kikimr/testing/packages/yandex-cloud/testing/myt/nfs-control/yandex-search-kikimr-nfs-control-conf-testing-myt/pkg.json
            kikimr/testing/packages/yandex-cloud/testing/sas/nfs-control/yandex-search-kikimr-nfs-control-conf-testing-sas/pkg.json
        """.trimIndent())
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
