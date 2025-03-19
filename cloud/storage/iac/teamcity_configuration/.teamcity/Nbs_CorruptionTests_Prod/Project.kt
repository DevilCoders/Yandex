package Nbs_CorruptionTests_Prod

import Nbs_CorruptionTests_Prod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_CorruptionTests_Prod")
    name = "prod"
    archived = true

    buildType(Nbs_CorruptionTests_Prod_CorruptionTests64mbBlocksize)
    buildType(Nbs_CorruptionTests_Prod_CorruptionTestsRangesIntersection)
    buildType(Nbs_CorruptionTests_Prod_CorruptionTests512bytesBlocksize)

    params {
        param("env.CLUSTER", "prod")
    }
})
