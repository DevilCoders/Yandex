package DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestEnormous : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Acceptance test (enormous)"
    description = """Disk_count: 3; Disk_type: "network_ssd"; Disk_size (TiB): [2, 4, 8]; Disk_blocksize: 4KiB; Disk_validation_size: 1.5625%; Disk_validation_blocksize: 4MiB"""

    params {
        param("sandbox.config_path", "%configs_dir%/runner/dm/acceptance_tests/%cluster%/enormous.yaml")
    }

    triggers {
        schedule {
            id = "TRIGGER_1461"
            enabled = false
            schedulingPolicy = daily {
                hour = 21
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        finishBuildTrigger {
            id = "TRIGGER_1840"
            buildType = "${DiskManager_Tests_AcceptanceTests_HwNbsStableLabNoAgentBuilds_AcceptanceTestBig.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    failureConditions {
        executionTimeoutMin = 600
    }
})
