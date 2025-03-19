package Nbs_NrdCheckEmptinessTests_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_NrdCheckEmptinessTests_Prod_ZeroCheck : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiCheckNrdEmptinessTest)
    name = "Zero Check"
    paused = true

    requirements {
        contains("teamcity.agent.name", "build-agent-vla9.bootstrap.cloud-preprod.yandex.net", "RQ_5495")
    }
})
