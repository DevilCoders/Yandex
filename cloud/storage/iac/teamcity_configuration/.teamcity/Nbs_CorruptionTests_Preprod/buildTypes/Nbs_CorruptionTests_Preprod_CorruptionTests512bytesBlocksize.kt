package Nbs_CorruptionTests_Preprod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_CorruptionTests_Preprod_CorruptionTests512bytesBlocksize : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiCorruptionTestSuite)
    name = "Corruption Tests (512 bytes blocksize)"
    paused = true

    params {
        param("env.TEST_SUITE", "512bytes-bs")
    }

    triggers {
        schedule {
            id = "TRIGGER_534"
            schedulingPolicy = daily {
                hour = 5
                minute = 30
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
})
