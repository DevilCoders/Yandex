package Nbs_CorruptionTests_Preprod

import Nbs_CorruptionTests_Preprod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_CorruptionTests_Preprod")
    name = "preprod"
    archived = true

    buildType(Nbs_CorruptionTests_Preprod_CorruptionTests64mbBlocksize)
    buildType(Nbs_CorruptionTests_Preprod_CorruptionTestsRangesIntersection)
    buildType(Nbs_CorruptionTests_Preprod_CorruptionTests512bytesBlocksize)

    params {
        param("env.CLUSTER", "preprod")
    }
})
