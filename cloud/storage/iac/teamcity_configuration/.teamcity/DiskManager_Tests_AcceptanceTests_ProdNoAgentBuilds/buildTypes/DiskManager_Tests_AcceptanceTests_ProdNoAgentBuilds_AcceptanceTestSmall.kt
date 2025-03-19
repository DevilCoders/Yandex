package DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_AcceptanceTests_ProdNoAgentBuilds_AcceptanceTestSmall : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Acceptance test (small)"
    description = """Disk_count: 4; Disk_type: "network_ssd"; Disk_size (GiB): [2, 4, 8, 16]; Disk_blocksize: 4KiB; Disk_validation_size: 50%; Disk_validation_blocksize: 4MiB"""

    params {
        param("sandbox.config_path", "%configs_dir%/runner/dm/acceptance_tests/%cluster%/small.yaml")
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

    failureConditions {
        executionTimeoutMin = 600
    }
})
