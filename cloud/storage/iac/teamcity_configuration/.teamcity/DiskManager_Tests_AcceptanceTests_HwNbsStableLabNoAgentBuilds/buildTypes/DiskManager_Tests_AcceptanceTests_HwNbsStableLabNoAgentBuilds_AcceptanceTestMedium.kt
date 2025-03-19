package DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestMedium : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Acceptance test (medium)"
    description = """Disk_count: 3; Disk_type: "network_ssd"; Disk_size (GiB): [32, 64, 128]; Disk_blocksize: 4KiB; Disk_validation_size: 25%; Disk_validation_blocksize: 4MiB"""

    params {
        param("sandbox.config_path", "%configs_dir%/runner/dm/acceptance_tests/%cluster%/medium.yaml")
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
            buildType = "${DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestSmall.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    failureConditions {
        executionTimeoutMin = 600
    }
})
