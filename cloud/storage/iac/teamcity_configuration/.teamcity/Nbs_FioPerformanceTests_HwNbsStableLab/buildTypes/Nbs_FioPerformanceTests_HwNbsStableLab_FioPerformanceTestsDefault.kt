package Nbs_FioPerformanceTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsDefault : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiFioPerformanceTestSuite)
    name = "Fio Performance Tests (default)"
    paused = true

    triggers {
        schedule {
            id = "TRIGGER_1542"
            schedulingPolicy = daily {
                hour = 5
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
    
    disableSettings("TRIGGER_1461")
})
