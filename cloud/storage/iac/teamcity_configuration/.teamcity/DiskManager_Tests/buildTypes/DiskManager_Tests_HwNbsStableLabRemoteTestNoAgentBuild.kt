package DiskManager_Tests.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.XmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.xmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_HwNbsStableLabRemoteTestNoAgentBuild : BuildType({
    templates(_Self.buildTypes.Nbs_YcNbsCiRunYaMakeAgentless)
    name = "[HW-NBS-STABLE-LAB] Remote Test (No-agent build)"
    description = "Disk Manager Remote Test"

    buildNumberPattern = "%build.counter%[%build.vcs.number%]"

    params {
        param("sandbox.config_path", "%configs_dir%/runner/dm/remote_test/remote_test.yaml")
    }

    triggers {
        schedule {
            id = "TRIGGER_1288"
            schedulingPolicy = daily {
                hour = 0
            }
            branchFilter = ""
            triggerBuild = always()
        }
    }

    failureConditions {
        executionTimeoutMin = 600
    }

    features {
        xmlReport {
            id = "BUILD_EXT_3202"
            reportType = XmlReport.XmlReportType.JUNIT
            rules = "%output_dir%/**/junit_report.xml"
            verbose = true
        }
    }
})
