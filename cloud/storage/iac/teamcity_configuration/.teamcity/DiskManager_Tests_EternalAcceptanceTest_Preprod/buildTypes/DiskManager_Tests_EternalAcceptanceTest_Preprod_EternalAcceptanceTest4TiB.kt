package DiskManager_Tests_EternalAcceptanceTest_Preprod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.notifications
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_EternalAcceptanceTest_Preprod_EternalAcceptanceTest4TiB : BuildType({
    templates(DiskManager.buildTypes.DiskManager_YcDiskManagerCiAcceptanceTestSuite)
    name = "Eternal acceptance test (4 TiB)"
    description = """Disk_type: "network_ssd"; Disk_size (TiB): 4; Disk_blocksize: 4KiB;"""
    paused = true

    maxRunningBuilds = 1

    params {
        param("env.INSTANCE_RAM", "32")
        param("env.DISK_SIZE", "4096")
        param("env.INSTANCE_CORES", "32")
        param("env.DISK_WRITE_SIZE_PERCENTAGE", "1")
    }

    triggers {
        schedule {
            id = "TRIGGER_1461"
            enabled = false
            schedulingPolicy = daily {
                hour = 0
                minute = 20
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
            param("cronExpression_hour", "0")
            param("cronExpression_dw", "3/2")
            param("cronExpression_min", "20")
            param("cronExpression_dm", "?")
        }
        finishBuildTrigger {
            id = "TRIGGER_1849"
            buildType = "${DiskManager_Tests_EternalAcceptanceTest_Preprod_EternalAcceptanceTest2TiB.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    failureConditions {
        executionTimeoutMin = 1440
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
