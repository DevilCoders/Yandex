package Nfs_CorruptionTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_CorruptionTests_HwNbsStableLab_CorruptionTests4kbBlocksize : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNfsCiCorruptionTestSuite)
    name = "Corruption Tests (4KB blocksize)"
    paused = true

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
        param("env.TEST_SUITE", "4kb-bs")
    }

    triggers {
        schedule {
            id = "TRIGGER_534"
            schedulingPolicy = daily {
                hour = 7
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
})
