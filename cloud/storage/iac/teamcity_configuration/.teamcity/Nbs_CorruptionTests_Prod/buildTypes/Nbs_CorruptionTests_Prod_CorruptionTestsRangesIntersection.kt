package Nbs_CorruptionTests_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_CorruptionTests_Prod_CorruptionTestsRangesIntersection : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiCorruptionTestSuite)
    name = "Corruption Tests (ranges intersection)"
    paused = true

    params {
        param("env.TEST_SUITE", "ranges-intersection")
    }
})
