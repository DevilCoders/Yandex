package Nbs_FioPerformanceTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsBlockSize32 : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiFioPerformanceTestSuite)
    name = "Fio Performance Tests (block size 32)"
    paused = true

    artifactRules = """
        tcpdump-*.txt
        ssh-*.log
    """.trimIndent()

    params {
        param("env.TEST_SUITE", "large_block_size")
    }

    triggers {
        schedule {
            id = "TRIGGER_1542"
            schedulingPolicy = daily {
                hour = 20
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
    
    disableSettings("TRIGGER_1461")
})
