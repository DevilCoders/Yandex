package DiskManager_Tests_AcceptanceTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.notifications
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestMedium : BuildType({
    templates(DiskManager.buildTypes.DiskManager_YcDiskManagerCiAcceptanceTestSuite)
    name = "Acceptance test (medium)"
    description = """Disk_count: 3; Disk_type: "network_ssd"; Disk_size (GiB): [32, 64, 128]; Disk_blocksize: 4KiB; Disk_validation_size: 25%; Disk_validation_blocksize: 4MiB"""
    paused = true

    params {
        param("env.INSTANCE_RAM", "4")
        param("env.TEST_SUITE", "medium")
        param("env.INSTANCE_CORES", "4")
    }

    triggers {
        schedule {
            id = "TRIGGER_1461"
            enabled = false
            schedulingPolicy = daily {
                hour = 19
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        finishBuildTrigger {
            id = "TRIGGER_1829"
            buildType = "${DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestSmall.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    features {
        notifications {
            id = "BUILD_EXT_2997"
            notifierSettings = emailNotifier {
                email = "vlad-serikov@yandex-team.ru"
            }
            buildFailedToStart = true
            buildFailed = true
            firstBuildErrorOccurs = true
        }
    }
})
