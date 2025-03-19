package Nbs.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.Swabra
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.perfmon
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.swabra
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_YcNbsCiMigrationTest : Template({
    name = "yc-nbs-ci-migration-test"

    allowExternalStatus = true
    artifactRules = """
        ssh-*.log
        tcpdump-*.txt
    """.trimIndent()
    maxRunningBuilds = 1

    params {
        param("env.SERVICE_ACCOUNT_ID", "")
        param("env.TABLET_ID", "")
        param("env.KILL_PERIOD", "")
        param("env.CHECK_LOAD", "")
        param("env.KILL_TABLET", "")
        param("env.TIMEOUT", "")
        param("env.INSTANCE_IP", "")
        param("env.CLUSTER", "")
        param("env.COMPUTE_NODE", "")
        param("env.DISK_NAME", "")
        param("env.IPC_TYPE", "")
    }

    steps {
        script {
            name = "Run migration test"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -ex
                
                ARGS=""
                
                if [ ! -z  "${'$'}IPC_TYPE" ]; then
                	ARGS="${'$'}ARGS --ipc-type ${'$'}IPC_TYPE"
                fi
                
                if [ ! -z  "${'$'}SERVICE_ACCOUNT_ID" ]; then
                	ARGS="${'$'}ARGS --service-account-id ${'$'}SERVICE_ACCOUNT_ID"
                fi
                
                yc-nbs-ci-migration-test \
                  --teamcity \
                  --cluster ${'$'}CLUSTER \
                  --disk-name ${'$'}DISK_NAME \
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
            id = "TRIGGER_534"
            enabled = false
            schedulingPolicy = daily {
                hour = 6
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
            id = "BUILD_EXT_3105"
            teamcitySshKey = "robot-yc-nbs"
        }
        swabra {
            id = "swabra"
            filesCleanup = Swabra.FilesCleanup.DISABLED
        }
    }

    requirements {
        doesNotContain("teamcity.agent.name", "kiwi", "RQ_4057")
        doesNotMatch("teamcity.agent.name", "cm-build-agent-.*", "RQ_5200")
    }
})
