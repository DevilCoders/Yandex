package Nbs_Build_Trunk

import Nbs_Build_Trunk.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_Build_Trunk")
    name = "Trunk"

    buildType(Nbs_Build_Trunk_BuildNbsConfigsYandexCloudStable)
    buildType(Nbs_Build_Trunk_BuildNbsConfigsYandexCloudTesting)
    buildType(Nbs_Build_Trunk_BuildNbsBinariesYandexCloudTesting)

    params {
        param("env.ARCADIA_URL", "arcadia:/arc/trunk/arcadia")
    }
    buildTypesOrder = arrayListOf(Nbs_Build_Trunk_BuildNbsBinariesYandexCloudTesting, Nbs_Build_Trunk_BuildNbsConfigsYandexCloudTesting, Nbs_Build_Trunk_BuildNbsConfigsYandexCloudStable)
})
