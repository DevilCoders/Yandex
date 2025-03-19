package Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.XmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.xmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseHardening : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Functional Tests (release, hardening)"

    params {
        param("sandbox.config_path", "%configs_dir%/runner/nbs/local_tests/%branch_name%/functional_tests/hardering.yaml")
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
        feature {
            id = "ru.yandex.teamcity.plugin.sandbox.SandboxTaskLauncherFeature"
            type = "ru.yandex.teamcity.plugin.sandbox.SandboxTaskLauncherFeature"
            param("owner", "%owner%")
            param("sandbox_task_type", "NBS_TEAMCITY_RUNNER")
            param("secure:authorisation_token", "credentialsJSON:e880f320-9eee-4192-b8f4-09f5f273043b")
            param("description", "Run NbsTeamcityRunner in sandbox")
        }
    }
})
