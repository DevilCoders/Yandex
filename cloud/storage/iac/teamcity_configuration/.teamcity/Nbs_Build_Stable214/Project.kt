package Nbs_Build_Stable214

import Nbs_Build_Stable214.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_Build_Stable214")
    name = "Stable-21-4"
    archived = true

    buildType(Nbs_Build_Stable214_BuildNbsBinariesYandexCloudTesting)

    params {
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-21-4")
    }
})
