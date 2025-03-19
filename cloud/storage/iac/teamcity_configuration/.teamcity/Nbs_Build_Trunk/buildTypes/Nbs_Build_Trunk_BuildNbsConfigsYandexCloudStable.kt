package Nbs_Build_Trunk.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nbs_Build_Trunk_BuildNbsConfigsYandexCloudStable : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiRunYaPackage)
    name = "Build NBS configs (yandex-cloud/stable)"

    params {
        param("env.DEBIAN_DISTRIBUTION", "stable")
        param("env.PACKAGES_TO_BUILD", "cloud/storage/core/packages/yandex-cloud-storage-breakpad/pkg.json;cloud/blockstore/packages/yc-nbs-breakpad-systemd/pkg.json;cloud/blockstore/packages/yc-nbs-http-proxy-systemd/pkg.json;cloud/blockstore/packages/yc-nbs-systemd/pkg.json;kikimr/production/packages/yandex-cloud/preprod/myt/nbs/yandex-search-kikimr-nbs-conf-pre-prod-myt/pkg.json;kikimr/production/packages/yandex-cloud/preprod/sas/nbs/yandex-search-kikimr-nbs-conf-pre-prod-sas/pkg.json;kikimr/production/packages/yandex-cloud/preprod/vla/nbs/yandex-search-kikimr-nbs-conf-pre-prod-vla/pkg.json;kikimr/production/packages/yandex-cloud/preprod/myt/nbs_control/yandex-search-kikimr-nbs-control-conf-pre-prod-myt/pkg.json;kikimr/production/packages/yandex-cloud/preprod/sas/nbs_control/yandex-search-kikimr-nbs-control-conf-pre-prod-sas/pkg.json;kikimr/production/packages/yandex-cloud/preprod/vla/nbs_control/yandex-search-kikimr-nbs-control-conf-pre-prod-vla/pkg.json;kikimr/production/packages/yandex-cloud/prod/myt/nbs/yandex-search-kikimr-nbs-conf-prod-myt/pkg.json;kikimr/production/packages/yandex-cloud/prod/sas/nbs/yandex-search-kikimr-nbs-conf-prod-sas/pkg.json;kikimr/production/packages/yandex-cloud/prod/vla/nbs/yandex-search-kikimr-nbs-conf-prod-vla/pkg.json;kikimr/production/packages/yandex-cloud/prod/myt/nbs_control/yandex-search-kikimr-nbs-control-conf-prod-myt/pkg.json;kikimr/production/packages/yandex-cloud/prod/sas/nbs_control/yandex-search-kikimr-nbs-control-conf-prod-sas/pkg.json;kikimr/production/packages/yandex-cloud/prod/vla/nbs_control/yandex-search-kikimr-nbs-control-conf-prod-vla/pkg.json;kikimr/testing/packages/yandex-cloud/hw-nbs-dev-lab/global/nbs/yandex-search-kikimr-nbs-conf-hw-nbs-dev-lab-global/pkg.json;kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/nbs/yandex-search-kikimr-nbs-conf-hw-nbs-stable-lab-global/pkg.json;kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/nbs_control/yandex-search-kikimr-nbs-control-conf-hw-nbs-stable-lab-global/pkg.json;kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/storage/yandex-search-kikimr-storage-conf-hw-nbs-stable-lab-global/pkg.json;kikimr/testing/packages/yandex-cloud/testing/vla/nbs/yandex-search-kikimr-nbs-conf-testing-vla/pkg.json;kikimr/testing/packages/yandex-cloud/testing/myt/nbs/yandex-search-kikimr-nbs-conf-testing-myt/pkg.json;kikimr/testing/packages/yandex-cloud/testing/sas/nbs/yandex-search-kikimr-nbs-conf-testing-sas/pkg.json;kikimr/testing/packages/yandex-cloud/testing/vla/nbs_control/yandex-search-kikimr-nbs-control-conf-testing-vla/pkg.json;kikimr/testing/packages/yandex-cloud/testing/myt/nbs_control/yandex-search-kikimr-nbs-control-conf-testing-myt/pkg.json;kikimr/testing/packages/yandex-cloud/testing/sas/nbs_control/yandex-search-kikimr-nbs-control-conf-testing-sas/pkg.json;kikimr/production/packages/yandex-cloud/israel/az1/nbs/yandex-search-kikimr-nbs-conf-israel-az1/pkg.json;kikimr/production/packages/yandex-cloud/israel/az1/nbs_control/yandex-search-kikimr-nbs-control-conf-israel-az1/pkg.json")
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
    }
})
