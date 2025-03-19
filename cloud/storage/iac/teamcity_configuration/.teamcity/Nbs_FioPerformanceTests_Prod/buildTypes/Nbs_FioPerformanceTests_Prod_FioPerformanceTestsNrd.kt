package Nbs_FioPerformanceTests_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_FioPerformanceTests_Prod_FioPerformanceTestsNrd : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiFioPerformanceTestSuite)
    name = "Fio Performance Tests (nrd)"
    paused = true

    params {
        param("env.TEST_SUITE", "nrd")
    }

    triggers {
        schedule {
            id = "TRIGGER_1541"
            schedulingPolicy = daily {
                hour = 0
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        schedule {
            id = "TRIGGER_1628"
            schedulingPolicy = daily {
                hour = 5
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        schedule {
            id = "TRIGGER_1629"
            schedulingPolicy = daily {
                hour = 10
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
    
    disableSettings("TRIGGER_1461")
})
