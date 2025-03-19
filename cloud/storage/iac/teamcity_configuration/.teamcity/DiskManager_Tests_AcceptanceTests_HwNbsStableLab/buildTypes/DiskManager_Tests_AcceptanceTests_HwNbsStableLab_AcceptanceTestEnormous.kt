package DiskManager_Tests_AcceptanceTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.notifications
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestEnormous : BuildType({
    templates(DiskManager.buildTypes.DiskManager_YcDiskManagerCiAcceptanceTestSuite)
    name = "Acceptance test (enormous)"
    description = """Disk_count: 3; Disk_type: "network_ssd"; Disk_size (TiB): [2, 4, 8]; Disk_blocksize: 4KiB; Disk_validation_size: 1.5625%; Disk_validation_blocksize: 4MiB"""
    paused = true

    params {
        param("env.TEST_SUITE", "enormous")
    }

    triggers {
        schedule {
            id = "TRIGGER_1461"
            schedulingPolicy = daily {
                hour = 21
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        finishBuildTrigger {
            id = "TRIGGER_1840"
            enabled = false
            buildType = "${DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestBig.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    failureConditions {
        executionTimeoutMin = 1440
    }

    features {
        notifications {
            id = "BUILD_EXT_3043"
            notifierSettings = emailNotifier {
                email = "vlad-serikov@yandex-team.ru"
            }
            buildFailedToStart = true
            buildFailed = true
            firstBuildErrorOccurs = true
        }
    }
})
