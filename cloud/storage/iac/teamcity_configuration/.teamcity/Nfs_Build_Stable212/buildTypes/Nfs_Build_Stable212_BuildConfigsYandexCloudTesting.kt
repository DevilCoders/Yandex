package Nfs_Build_Stable212.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_Build_Stable212_BuildConfigsYandexCloudTesting : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaPackage)
    name = "Build configs (yandex-cloud/testing)"
    paused = true

    params {
        param("env.PACKAGES_TO_BUILD", "kikimr/testing/packages/yandex-cloud/hw-nbs-dev-lab/global/nfs/yandex-search-kikimr-nfs-conf-hw-nbs-dev-lab-global/pkg.json;kikimr/testing/packages/yandex-cloud/hw-nbs-lab/global/nfs/yandex-search-kikimr-nfs-conf-hw-nbs-lab-global/pkg.json;kikimr/testing/packages/yandex-cloud/hw-nbs-stable-lab/global/nfs/yandex-search-kikimr-nfs-conf-hw-nbs-stable-lab-global/pkg.json;")
    }
})
