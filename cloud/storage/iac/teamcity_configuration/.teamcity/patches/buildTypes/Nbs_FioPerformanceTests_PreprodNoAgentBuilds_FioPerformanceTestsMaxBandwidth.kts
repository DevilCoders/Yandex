package patches.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.BuildType
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule
import jetbrains.buildServer.configs.kotlin.v2019_2.ui.*

/*
This patch script was generated by TeamCity on settings change in UI.
To apply the patch, create a buildType with id = 'Nbs_FioPerformanceTests_PreprodNoAgentBuilds_FioPerformanceTestsMaxBandwidth'
in the project with id = 'Nbs_FioPerformanceTests_PreprodNoAgentBuilds', and delete the patch script.
*/
create(RelativeId("Nbs_FioPerformanceTests_PreprodNoAgentBuilds"), BuildType({
    templates(RelativeId("Nbs_YcNbsCiRunYaMakeAgentless"))
    id("Nbs_FioPerformanceTests_PreprodNoAgentBuilds_FioPerformanceTestsMaxBandwidth")
    name = "Fio Performance Tests (max bandwidth)"

    maxRunningBuilds = 0

    params {
        param("sandbox.config_path", "%configs_dir%/runner/nbs/fio_performance_tests/%cluster%/max_bandwidth.yaml")
    }

    triggers {
        schedule {
            id = "TRIGGER_1543"
            schedulingPolicy = daily {
                hour = 2
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        schedule {
            id = "TRIGGER_1903"
            schedulingPolicy = daily {
                hour = 5
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        schedule {
            id = "TRIGGER_1904"
            schedulingPolicy = daily {
                hour = 8
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
}))

