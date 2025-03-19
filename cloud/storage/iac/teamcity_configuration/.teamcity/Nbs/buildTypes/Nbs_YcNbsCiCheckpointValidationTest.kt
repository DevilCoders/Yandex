package Nbs.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.perfmon
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_YcNbsCiCheckpointValidationTest : Template({
    name = "yc-nbs-ci-checkpoint-validation-test"

    allowExternalStatus = true
    maxRunningBuilds = 1
    publishArtifacts = PublishMode.SUCCESSFUL

    params {
        param("env.CLUSTER", "")
        password("env.YT_OAUTH_TOKEN", "credentialsJSON:9ccf0009-c6b7-44f1-97c0-0211e7cb89b9", display = ParameterDisplay.HIDDEN)
    }

    steps {
        script {
            name = "Run checkpoint validation test"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -ex
                
                ARGS=""
                
                if [ ! -z  "${'$'}SERVICE_ACCOUNT_ID" ]; then
                	ARGS="${'$'}ARGS --service-account-id ${'$'}SERVICE_ACCOUNT_ID"
                fi
                
                yc-nbs-ci-checkpoint-validation-test \
                  --teamcity \
                  --cluster ${'$'}CLUSTER \
                  --validator-path /usr/bin/checkpoint-validator \
                  ${'$'}ARGS
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host"""
        }
    }

    triggers {
        schedule {
            id = "TRIGGER_1593"
            schedulingPolicy = cron {
                hours = "/3"
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
            id = "BUILD_EXT_2929"
            teamcitySshKey = "id_rsa_overlay"
        }
        sshAgent {
            id = "BUILD_EXT_3005"
            teamcitySshKey = "robot-yc-nbs"
        }
    }

    requirements {
        doesNotContain("teamcity.agent.name", "kiwi", "RQ_4057")
        doesNotMatch("teamcity.agent.name", "cm-build-agent-.*", "RQ_5194")
    }
})
