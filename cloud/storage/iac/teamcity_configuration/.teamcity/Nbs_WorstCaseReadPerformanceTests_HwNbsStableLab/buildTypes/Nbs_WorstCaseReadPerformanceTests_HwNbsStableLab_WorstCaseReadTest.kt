package Nbs_WorstCaseReadPerformanceTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.perfmon
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_WorstCaseReadPerformanceTests_HwNbsStableLab_WorstCaseReadTest : BuildType({
    name = "Worst Case Read Test"

    allowExternalStatus = true
    publishArtifacts = PublishMode.SUCCESSFUL

    params {
        password("env.YT_OAUTH_TOKEN", "credentialsJSON:9ccf0009-c6b7-44f1-97c0-0211e7cb89b9", display = ParameterDisplay.HIDDEN)
    }

    steps {
        script {
            name = "Run test suite"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                ARGS=""
                
                if [ ! -z  "${'$'}IPC_TYPE" ]; then
                	ARGS="${'$'}ARGS --ipc-type ${'$'}IPC_TYPE"
                fi
                
                yc-nbs-ci-worst-case-read-performance-test-suite \
                  --cluster ${'$'}CLUSTER \
                  --write-block-size 128 \
                  --teamcity \
                  ${'$'}ARGS
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659""""
        }
    }

    triggers {
        schedule {
            schedulingPolicy = daily {
                hour = 2
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        schedule {
            enabled = false
            schedulingPolicy = daily {
                hour = 3
            }
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }

    features {
        perfmon {
        }
        freeDiskSpace {
            requiredSpace = "1gb"
            failBuild = true
        }
        sshAgent {
            teamcitySshKey = "robot-yc-nbs"
        }
        sshAgent {
            teamcitySshKey = "id_rsa_overlay"
        }
    }

    requirements {
        doesNotContain("teamcity.agent.name", "kiwi")
        doesNotMatch("teamcity.agent.name", "cm-build-agent-.*")
    }
})
