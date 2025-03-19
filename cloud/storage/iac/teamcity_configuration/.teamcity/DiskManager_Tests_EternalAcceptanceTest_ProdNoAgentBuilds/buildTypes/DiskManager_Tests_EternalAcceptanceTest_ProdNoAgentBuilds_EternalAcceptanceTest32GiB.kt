package DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger

object DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest32GiB : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Eternal acceptance test (32 GiB)"
    description = """Disk_type: "network_ssd"; Disk_size (GiB): 32; Disk_blocksize: 4KiB;"""

    params {
        param("sandbox.config_path", "%configs_dir%/runner/dm/eternal_acceptance_tests/%cluster%/32_gib.yaml")
    }

    triggers {
        finishBuildTrigger {
            id = "TRIGGER_1824"
            buildType = "${DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds_EternalAcceptanceTest16GiB.id}"
            successfulOnly = true
            branchFilter = ""
        }
    }

    failureConditions {
        executionTimeoutMin = 600
    }
})
