package Nfs_CorruptionTests_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_CorruptionTests_Prod_CorruptionTests64mbBlocksize : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNfsCiCorruptionTestSuite)
    name = "Corruption Tests (64MB blocksize)"
    paused = true

    params {
        param("env.CLUSTER", "prod")
        param("env.TEST_SUITE", "64MB-bs")
    }

    triggers {
        schedule {
            id = "TRIGGER_1745"
            schedulingPolicy = daily {
                hour = 4
                minute = 30
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
})
