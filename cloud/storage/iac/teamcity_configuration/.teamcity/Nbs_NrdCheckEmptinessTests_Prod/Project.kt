package Nbs_NrdCheckEmptinessTests_Prod

import Nbs_NrdCheckEmptinessTests_Prod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_NrdCheckEmptinessTests_Prod")
    name = "prod"
    archived = true

    buildType(Nbs_NrdCheckEmptinessTests_Prod_ZeroCheck)

    params {
        param("env.CLUSTER", "prod")
    }

    cleanup {
        baseRule {
            option("disableCleanupPolicies", true)
        }
    }
})
