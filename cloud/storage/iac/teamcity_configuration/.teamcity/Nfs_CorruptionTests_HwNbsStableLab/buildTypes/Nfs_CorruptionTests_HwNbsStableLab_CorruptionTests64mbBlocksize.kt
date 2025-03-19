package Nfs_CorruptionTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_CorruptionTests_HwNbsStableLab_CorruptionTests64mbBlocksize : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNfsCiCorruptionTestSuite)
    name = "Corruption Tests (64MB blocksize)"
    paused = true

    artifactRules = "*.txt"

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
        param("env.TEST_SUITE", "64MB-bs")
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
