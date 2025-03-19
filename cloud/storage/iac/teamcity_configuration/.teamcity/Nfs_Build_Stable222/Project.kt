package Nfs_Build_Stable222

import Nfs_Build_Stable222.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_Build_Stable222")
    name = "Stable-22-2"

    buildType(Nfs_Build_Stable222_BuildBinariesYandexCloudTesting)

    params {
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-22-2")
    }
    buildTypesOrder = arrayListOf(Nfs_Build_Stable222_BuildBinariesYandexCloudTesting)
})
