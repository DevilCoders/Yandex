package Nfs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.XmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.xmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_LocalTests_Stable222_FunctionalTestsNoAgentBuilds_FunctionalTestsReleaseThreadSanitizer : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "Functional Tests (release, thread sanitizer)"

    params {
        param("system.teamcity.build.checkoutDir.expireHours", "0")
        param("sandbox.config_path", "%configs_dir%/runner/nfs/local_tests/%branch_name%/functional_tests/thread_sanitizer.yaml")
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
