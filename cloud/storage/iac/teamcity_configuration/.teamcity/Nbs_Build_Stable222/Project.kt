package Nbs_Build_Stable222

import Nbs_Build_Stable222.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_Build_Stable222")
    name = "Stable-22-2"

    buildType(Nbs_Build_Stable222_BuildNbsBinariesYandexCloudTesting)

    params {
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-22-2")
    }
})
