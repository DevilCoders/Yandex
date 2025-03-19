package DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger

object DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest2TiB : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Eternal acceptance test (2 TiB)"
    description = """Disk_type: "network_ssd"; Disk_size (TiB): 2; Disk_blocksize: 4KiB;"""

    params {
        param("sandbox.config_path", "%configs_dir%/runner/dm/eternal_acceptance_tests/%cluster%/2_tib.yaml")
    }

    triggers {
        finishBuildTrigger {
            id = "TRIGGER_1847"
            buildType = "${DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest1TiB.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    failureConditions {
        executionTimeoutMin = 600
    }
})
