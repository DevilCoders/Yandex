package Nfs_Build_Stable212.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_Build_Stable212_BuildBinariesYandexCloudTesting : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaPackage)
    name = "Build binaries (yandex-cloud/testing)"
    paused = true

    params {
        param("env.PACKAGES_TO_BUILD", "cloud/filestore/packages/yandex-cloud-filestore-client/pkg.json;cloud/filestore/packages/yandex-cloud-filestore-server/pkg.json;cloud/filestore/packages/yandex-cloud-filestore-vhost/pkg.json;cloud/filestore/packages/yandex-cloud-filestore-http-proxy/pkg.json;")
    }
})
