package Nbs_CheckpointValidationTest_Preprod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_CheckpointValidationTest_Preprod_CheckpointValidationTest : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiCheckpointValidationTest)
    name = "Checkpoint Validation Test"

    params {
        param("env.SERVICE_ACCOUNT_ID", "bfbf9rg1scirhqet6kn8")
        param("env.CLUSTER", "preprod")
    }

    triggers {
        schedule {
            id = "TRIGGER_1593"
            schedulingPolicy = daily {
                hour = 13
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
            param("cronExpression_hour", "/3")
        }
    }
})
