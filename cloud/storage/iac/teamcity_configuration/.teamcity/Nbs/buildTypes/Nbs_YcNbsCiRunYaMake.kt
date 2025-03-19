package Nbs.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.XmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.perfmon
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.xmlReport
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_YcNbsCiRunYaMake : Template({
    name = "yc-nbs-ci-run-ya-make"

    allowExternalStatus = true
    artifactRules = "junit_report.xml"
    publishArtifacts = PublishMode.SUCCESSFUL

    params {
        param("env.DISK_SPACE", "40")
        param("env.RAM", "8")
        param("env.DEFINITION_FLAGS", "-DSKIP_JUNK -DUSE_EAT_MY_DATA -DDEBUGINFO_LINES_ONLY")
        param("env.TIMEOUT", "7200")
        param("env.BUILD_TYPE", "release")
        param("env.CORES", "8")
        param("env.SANITIZE", "")
        param("env.ARCADIA_URL", "arcadia-arc:/#releases/ydb/stable-20-4")
        param("env.CONTAINER", "2185033214")
        password("env.SANDBOX_OAUTH_TOKEN", "credentialsJSON:9f44c742-f345-4805-b419-042d17c38730", description = "robot-yc-nbs sandbox oauth token", display = ParameterDisplay.HIDDEN)
        param("env.THREADS", "4")
    }

    steps {
        script {
            name = "Run tests"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                echo ${'$'}TEST_TARGETS | sed 's/;/\n/g' > test_targets.txt
                
                ARGS=""
                
                if [[ ${'$'}SANITIZE ]]; then
                	ARGS="${'$'}ARGS --sanitize ${'$'}SANITIZE"
                fi
                
                if [[ ${'$'}CONTAINER ]]; then
                	ARGS="${'$'}ARGS --container-resource ${'$'}CONTAINER"
                fi
                
                if [[ ${'$'}SANDBOX_ENV_VARS ]]; then
                    ARGS="${'$'}ARGS --env-vars ${'$'}SANDBOX_ENV_VARS"
                fi
                
                if [[ ${'$'}RAM ]]; then
                    ARGS="${'$'}ARGS --ram ${'$'}RAM"
                fi
                
                if [[ ${'$'}CORES ]]; then
                    ARGS="${'$'}ARGS --cores ${'$'}CORES"
                fi
                
                if [[ ${'$'}THREADS ]]; then
                    ARGS="${'$'}ARGS --threads ${'$'}THREADS"
                fi
                
                if [[ ${'$'}DISK_SPACE ]]; then
                    ARGS="${'$'}ARGS --disk_space ${'$'}DISK_SPACE"
                fi
                
                if [[ "${'$'}DISABLE_TEST_TIMEOUT" = true ]]; then
                    ARGS="${'$'}ARGS --disable-test-timeout"
                fi
                
                yc-nbs-ci-run-ya-make \
                  --test-targets-file test_targets.txt \
                  --teamcity \
                  --timeout ${'$'}TIMEOUT \
                  --arcadia-url ${'$'}ARCADIA_URL \
                  --build-type ${'$'}BUILD_TYPE \
                  --definition-flags "${'$'}DEFINITION_FLAGS" \
                  ${'$'}ARGS
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = "--network host"
        }
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
        perfmon {
            id = "perfmon"
        }
        freeDiskSpace {
            id = "jetbrains.agent.free.space"
            requiredSpace = "1gb"
            failBuild = true
        }
        sshAgent {
            id = "BUILD_EXT_2868"
            teamcitySshKey = "robot-yc-nbs"
        }
        xmlReport {
            id = "BUILD_EXT_2942"
            reportType = XmlReport.XmlReportType.JUNIT
            rules = "junit_report.xml"
            verbose = true
        }
    }

    requirements {
        doesNotContain("teamcity.agent.name", "kiwi", "RQ_4057")
        matches("teamcity.agent.name", "build-agent-.*", "RQ_5098")
    }
})
