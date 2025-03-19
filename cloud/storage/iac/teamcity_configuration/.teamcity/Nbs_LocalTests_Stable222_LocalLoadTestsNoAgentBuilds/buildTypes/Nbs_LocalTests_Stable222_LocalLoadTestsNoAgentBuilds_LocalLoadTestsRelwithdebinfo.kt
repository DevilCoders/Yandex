package Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.XmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.xmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_LocalTests_Stable222_LocalLoadTestsNoAgentBuilds_LocalLoadTestsRelwithdebinfo : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Local Load Tests (relwithdebinfo)"

    params {
        param("sandbox.config_path", "%configs_dir%/runner/nbs/local_tests/%branch_name%/load_tests/relwithdebinfo.yaml")
    }

    triggers {
        schedule {
            id = "TRIGGER_1461"
            schedulingPolicy = daily {
                hour = 3
            }
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }

    features {
        xmlReport {
            id = "BUILD_EXT_2942"
            reportType = XmlReport.XmlReportType.JUNIT
            rules = "%output_dir%/**/junit_report.xml"
            verbose = true
        }
    }
})
