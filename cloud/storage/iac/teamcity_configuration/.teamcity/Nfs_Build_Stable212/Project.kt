package Nfs_Build_Stable212

import Nfs_Build_Stable212.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_Build_Stable212")
    name = "Stable-21-2"
    archived = true

    buildType(Nfs_Build_Stable212_BuildConfigsYandexCloudStable)
    buildType(Nfs_Build_Stable212_BuildConfigsYandexCloudTesting)
    buildType(Nfs_Build_Stable212_BuildBinariesYandexCloudTesting)

    params {
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-21-2")
    }
    buildTypesOrder = arrayListOf(Nfs_Build_Stable212_BuildBinariesYandexCloudTesting, Nfs_Build_Stable212_BuildConfigsYandexCloudTesting, Nfs_Build_Stable212_BuildConfigsYandexCloudStable)
})
