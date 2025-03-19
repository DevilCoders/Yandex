package DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger

object DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest512GiB : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Eternal acceptance test (512 GiB)"
    description = """Disk_type: "network_ssd"; Disk_size (GiB): 512; Disk_blocksize: 4KiB;"""

    params {
        param("sandbox.config_path", "%configs_dir%/runner/dm/eternal_acceptance_tests/%cluster%/512_gib.yaml")
    }

    triggers {
        finishBuildTrigger {
            id = "TRIGGER_1839"
            buildType = "${DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest256GiB.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    failureConditions {
        executionTimeoutMin = 600
    }
})
