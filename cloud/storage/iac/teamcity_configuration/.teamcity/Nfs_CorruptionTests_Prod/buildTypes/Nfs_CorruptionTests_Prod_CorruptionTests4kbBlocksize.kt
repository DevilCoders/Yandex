package Nfs_CorruptionTests_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_CorruptionTests_Prod_CorruptionTests4kbBlocksize : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNfsCiCorruptionTestSuite)
    name = "Corruption Tests (4KB blocksize)"
    paused = true

    params {
        param("env.CLUSTER", "prod")
        param("env.TEST_SUITE", "4kb-bs")
    }

    triggers {
        schedule {
            id = "TRIGGER_1744"
            schedulingPolicy = daily {
                hour = 4
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
})
