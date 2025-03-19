package DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger

object DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest4TiB : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Eternal acceptance test (4 TiB)"
    description = """Disk_type: "network_ssd"; Disk_size (TiB): 4; Disk_blocksize: 4KiB;"""

    params {
        param("sandbox.config_path", "%configs_dir%/runner/dm/eternal_acceptance_tests/%cluster%/4_tib.yaml")
    }

    triggers {
        finishBuildTrigger {
            id = "TRIGGER_1849"
            buildType = "${DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds_EternalAcceptanceTest2TiB.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    failureConditions {
        executionTimeoutMin = 600
    }
})
