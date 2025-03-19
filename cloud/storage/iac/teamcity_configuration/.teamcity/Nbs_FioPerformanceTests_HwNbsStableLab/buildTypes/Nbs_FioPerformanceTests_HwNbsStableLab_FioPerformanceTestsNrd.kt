package Nbs_FioPerformanceTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsNrd : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiFioPerformanceTestSuite)
    name = "Fio Performance Tests (nrd)"
    paused = true

    detectHangingBuilds = false

    params {
        param("env.FORCE", "true")
        param("env.TEST_SUITE", "nrd")
        param("env.DEBUG", "true")
    }

    triggers {
        schedule {
            id = "TRIGGER_1541"
            schedulingPolicy = daily {
                hour = 2
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
    
    disableSettings("TRIGGER_1461")
})
