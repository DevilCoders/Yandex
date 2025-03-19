package Nfs_Build_Stable212.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_Build_Stable212_BuildConfigsYandexCloudStable : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaPackage)
    name = "Build configs (yandex-cloud/stable)"
    paused = true

    params {
        param("env.DEBIAN_DISTRIBUTION", "stable")
        param("env.PACKAGES_TO_BUILD", """
            kikimr/testing/packages/yandex-cloud/hw-nbs-dev-lab/global/nfs/yandex-search-kikimr-nfs-conf-hw-nbs-dev-lab-global/pkg.json
            kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/nfs/yandex-search-kikimr-nfs-conf-hw-nbs-stable-lab-global/pkg.json
            kikimr/production/packages/yandex-cloud/preprod/myt/nfs/yandex-search-kikimr-nfs-conf-preprod-myt/pkg.json
            kikimr/production/packages/yandex-cloud/preprod/sas/nfs/yandex-search-kikimr-nfs-conf-preprod-sas/pkg.json
            kikimr/production/packages/yandex-cloud/preprod/vla/nfs/yandex-search-kikimr-nfs-conf-preprod-vla/pkg.json
            kikimr/testing/packages/yandex-cloud/testing/vla/nfs/yandex-search-kikimr-nfs-conf-testing-vla/pkg.json
            kikimr/testing/packages/yandex-cloud/testing/myt/nfs/yandex-search-kikimr-nfs-conf-testing-myt/pkg.json
            kikimr/testing/packages/yandex-cloud/testing/sas/nfs/yandex-search-kikimr-nfs-conf-testing-sas/pkg.json
        """.trimIndent())
    }
})
