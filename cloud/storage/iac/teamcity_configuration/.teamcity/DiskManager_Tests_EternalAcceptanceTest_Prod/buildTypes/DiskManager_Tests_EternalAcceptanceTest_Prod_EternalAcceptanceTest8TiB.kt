package DiskManager_Tests_EternalAcceptanceTest_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.notifications
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_EternalAcceptanceTest_Prod_EternalAcceptanceTest8TiB : BuildType({
    templates(DiskManager.buildTypes.DiskManager_YcDiskManagerCiAcceptanceTestSuite)
    name = "Eternal acceptance test (8 TiB)"
    description = """Disk_type: "network_ssd"; Disk_size (TiB): 8; Disk_blocksize: 4KiB;"""
    paused = true

    maxRunningBuilds = 1

    params {
        param("env.INSTANCE_RAM", "32")
        param("env.DISK_SIZE", "8192")
        param("env.INSTANCE_CORES", "32")
        param("env.DISK_WRITE_SIZE_PERCENTAGE", "1")
    }

    triggers {
        schedule {
            id = "TRIGGER_1461"
            enabled = false
            schedulingPolicy = daily {
                hour = 0
                minute = 10
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
            param("cronExpression_hour", "0")
            param("cronExpression_dw", "2/2")
            param("cronExpression_min", "10")
            param("cronExpression_dm", "?")
        }
        finishBuildTrigger {
            id = "TRIGGER_1850"
            buildType = "${DiskManager_Tests_EternalAcceptanceTest_Prod_EternalAcceptanceTest4TiB.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    failureConditions {
        executionTimeoutMin = 2880
    }

    features {
        notifications {
            id = "BUILD_EXT_2963"
            notifierSettings = emailNotifier {
                email = "vlad-serikov@yandex-team.ru"
            }
            buildFailedToStart = true
            buildFailed = true
            firstBuildErrorOccurs = true
        }
    }
})
