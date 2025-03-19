package Nfs.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.perfmon
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_YcNfsCiBuildArcadiaTest : Template({
    name = "yc-nfs-ci-build-arcadia-test"

    allowExternalStatus = true
    artifactRules = "**/*"
    detectHangingBuilds = false
    maxRunningBuilds = 1

    params {
        param("env.CLUSTER", "")
        param("env.TEST_CASE", "")
    }

    steps {
        script {
            name = "Run build test"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -ex
                
                ARGS=""
                
                if [ ! -z  "${'$'}COMPUTE_NODE" ]; then
                	ARGS="${'$'}ARGS --compute-node ${'$'}COMPUTE_NODE"
                fi
                
                if [ ! -z  "${'$'}ZONE_ID" ]; then
                	ARGS="${'$'}ARGS --zone-id ${'$'}ZONE_ID"
                fi
                
                if [ "${'$'}DEBUG" = true ]; then
                	ARGS="${'$'}ARGS --debug"
                fi
                
                yc-nfs-ci-build-arcadia-test \
                  --teamcity \
                  --cluster ${'$'}CLUSTER \
                  --test-case ${'$'}TEST_CASE \
                  ${'$'}ARGS
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host --cap-add SYS_PTRACE --privileged"""
        }
    }

    triggers {
        schedule {
            id = "TRIGGER_534"
            schedulingPolicy = cron {
                dayOfWeek = "*"
            }
            triggerBuild = always()
            withPendingChangesOnly = false
            param("hour", "6")
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
    }

    requirements {
        doesNotContain("teamcity.agent.name", "kiwi", "RQ_4057")
        doesNotMatch("teamcity.agent.name", "cm-build-agent-.*", "RQ_5208")
    }
})
