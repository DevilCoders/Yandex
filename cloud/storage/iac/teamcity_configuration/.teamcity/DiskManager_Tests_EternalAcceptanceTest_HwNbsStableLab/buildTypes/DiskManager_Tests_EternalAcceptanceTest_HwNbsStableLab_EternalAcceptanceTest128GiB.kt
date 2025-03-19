package DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.notifications
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest128GiB : BuildType({
    templates(DiskManager.buildTypes.DiskManager_YcDiskManagerCiAcceptanceTestSuite)
    name = "Eternal acceptance test (128 GiB)"
    description = """Disk_type: "network_ssd"; Disk_size (GiB): 128; Disk_blocksize: 4KiB;"""
    paused = true

    maxRunningBuilds = 1

    params {
        param("env.INSTANCE_RAM", "16")
        param("env.DISK_SIZE", "128")
        param("env.INSTANCE_CORES", "16")
        param("env.DISK_WRITE_SIZE_PERCENTAGE", "5")
    }

    triggers {
        schedule {
            id = "TRIGGER_1461"
            enabled = false
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        finishBuildTrigger {
            id = "TRIGGER_1837"
            buildType = "${DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab_EternalAcceptanceTest64GiB.id}"
            successfulOnly = true
            branchFilter = ""
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
