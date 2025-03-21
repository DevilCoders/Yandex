package patches.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.BuildType
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule
import jetbrains.buildServer.configs.kotlin.v2019_2.ui.*

/*
This patch script was generated by TeamCity on settings change in UI.
To apply the patch, create a buildType with id = 'Nbs_FioPerformanceTests_HwNbsStableLabNoAgentBuilds_FioPerformanceTestsMaxIops'
in the project with id = 'Nbs_FioPerformanceTests_HwNbsStableLabNoAgentBuilds', and delete the patch script.
*/
create(RelativeId("Nbs_FioPerformanceTests_HwNbsStableLabNoAgentBuilds"), BuildType({
    templates(RelativeId("Nbs_YcNbsCiRunYaMakeAgentless"))
    id("Nbs_FioPerformanceTests_HwNbsStableLabNoAgentBuilds_FioPerformanceTestsMaxIops")
    name = "Fio Performance Tests (max iops)"

    params {
        param("sandbox.config_path", "%configs_dir%/runner/nbs/fio_performance_tests/%cluster%/max_iops.yaml")
    }

    triggers {
        schedule {
            id = "TRIGGER_1544"
            schedulingPolicy = daily {
                hour = 6
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
}))

