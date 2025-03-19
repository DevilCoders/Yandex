package Nbs_FioPerformanceTests_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_FioPerformanceTests_Prod_FioPerformanceTestsDefault : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiFioPerformanceTestSuite)
    name = "Fio Performance Tests (default)"
    paused = true

    triggers {
        schedule {
            id = "TRIGGER_1539"
            schedulingPolicy = daily {
                hour = 1
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        schedule {
            id = "TRIGGER_1626"
            schedulingPolicy = daily {
                hour = 5
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        schedule {
            id = "TRIGGER_1627"
            schedulingPolicy = daily {
                hour = 9
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
    
    disableSettings("TRIGGER_1461")
})
