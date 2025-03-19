package Nbs_FioPerformanceTests_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_FioPerformanceTests_Prod_FioPerformanceTestsBlockSize64k : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiFioPerformanceTestSuite)
    name = "Fio Performance Tests (block size 64K)"
    paused = true

    params {
        param("env.TEST_SUITE", "large_block_size")
    }

    triggers {
        schedule {
            id = "TRIGGER_1539"
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
