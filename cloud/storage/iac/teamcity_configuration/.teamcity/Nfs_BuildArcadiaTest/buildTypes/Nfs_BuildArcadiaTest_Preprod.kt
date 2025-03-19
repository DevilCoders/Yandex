package Nfs_BuildArcadiaTest.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_BuildArcadiaTest_Preprod : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNfsCiBuildArcadiaTest)
    name = "preprod"
    paused = true

    params {
        param("env.ZONE_ID", "ru-central1-a")
        param("env.CLUSTER", "preprod")
        param("env.TEST_CASE", "nfs")
    }

    triggers {
        schedule {
            id = "TRIGGER_534"
            schedulingPolicy = daily {
                hour = 3
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
            param("cronExpression_dw", "*")
        }
    }
})
