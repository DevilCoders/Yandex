package Nfs_Build_Trunk

import Nfs_Build_Trunk.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_Build_Trunk")
    name = "Trunk"

    buildType(Nfs_Build_Trunk_BuildConfigsYandexCloudTesting)
    buildType(Nfs_Build_Trunk_BuildConfigsYandexCloudStable)
    buildType(Nfs_Build_Trunk_BuildBinariesYandexCloudTesting)

    params {
        param("env.ARCADIA_URL", "arcadia:/arc/trunk/arcadia")
    }
    buildTypesOrder = arrayListOf(Nfs_Build_Trunk_BuildBinariesYandexCloudTesting, Nfs_Build_Trunk_BuildConfigsYandexCloudTesting, Nfs_Build_Trunk_BuildConfigsYandexCloudStable)
})
