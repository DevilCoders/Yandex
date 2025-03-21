package patches.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.BuildType
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule
import jetbrains.buildServer.configs.kotlin.v2019_2.ui.*

/*
This patch script was generated by TeamCity on settings change in UI.
To apply the patch, create a buildType with id = 'Nfs_CorruptionTests_PreprodNoAgentBuilds_CorruptionTests4kbBlocksize'
in the project with id = 'Nfs_CorruptionTests_PreprodNoAgentBuilds', and delete the patch script.
*/
create(RelativeId("Nfs_CorruptionTests_PreprodNoAgentBuilds"), BuildType({
    templates(RelativeId("Nbs_YcNbsCiRunYaMakeAgentless"))
    id("Nfs_CorruptionTests_PreprodNoAgentBuilds_CorruptionTests4kbBlocksize")
    name = "Corruption Tests (4KB blocksize)"

    params {
        param("sandbox.config_path", "%configs_dir%/runner/nfs/corruption_tests/%cluster%/4_kib_blocksize.yaml")
    }

    triggers {
        schedule {
            id = "TRIGGER_534"
            schedulingPolicy = daily {
                hour = 7
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }

    failureConditions {
        executionTimeoutMin = 600
    }
}))

