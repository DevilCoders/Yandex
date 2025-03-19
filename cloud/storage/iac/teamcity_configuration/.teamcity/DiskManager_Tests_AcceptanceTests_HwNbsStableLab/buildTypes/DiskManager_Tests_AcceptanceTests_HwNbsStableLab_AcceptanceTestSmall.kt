package DiskManager_Tests_AcceptanceTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.notifications
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_AcceptanceTests_HwNbsStableLab_AcceptanceTestSmall : BuildType({
    templates(DiskManager.buildTypes.DiskManager_YcDiskManagerCiAcceptanceTestSuite)
    name = "Acceptance test (small)"
    description = """Disk_count: 4; Disk_type: "network_ssd"; Disk_size (GiB): [2, 4, 8, 16]; Disk_blocksize: 4KiB; Disk_validation_size: 50%; Disk_validation_blocksize: 4MiB"""
    paused = true

    params {
        param("env.INSTANCE_RAM", "2")
        param("env.TEST_SUITE", "small")
        param("env.INSTANCE_CORES", "2")
    }

    triggers {
        schedule {
            id = "TRIGGER_1461"
            schedulingPolicy = daily {
                hour = 0
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
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
