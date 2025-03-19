package Nbs_CorruptionTests_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_CorruptionTests_Prod_CorruptionTests64mbBlocksize : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiCorruptionTestSuite)
    name = "Corruption Tests (64MB blocksize)"
    paused = true

    params {
        param("env.TEST_SUITE", "64MB-bs")
    }
})
