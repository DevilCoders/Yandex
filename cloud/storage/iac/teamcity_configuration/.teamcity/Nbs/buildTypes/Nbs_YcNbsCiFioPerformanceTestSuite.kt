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

object Nbs_YcNbsCiFioPerformanceTestSuite : Template({
    name = "yc-nbs-ci-fio-performance-test-suite"

    allowExternalStatus = true
    artifactRules = """
        ssh-*.log
        tcpdump-*.txt
    """.trimIndent()

    params {
        param("env.INSTANCE_RAM", "8")
        param("env.PLACEMENT_GROUP", "nbs-fio-tests")
        param("env.IN_PARALLEL", "false")
        param("env.FORCE", "false")
        param("env.CLUSTER", "prod")
        param("env.TEST_SUITE", "default")
        param("env.INSTANCE_CORES", "8")
        password("env.YT_OAUTH_TOKEN", "credentialsJSON:9ccf0009-c6b7-44f1-97c0-0211e7cb89b9", display = ParameterDisplay.HIDDEN)
    }

    steps {
        script {
            name = "Run test suite"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                ARGS=""
                
                if [ ! -z  "${'$'}IPC_TYPE" ]; then
                	ARGS="${'$'}ARGS --ipc-type ${'$'}IPC_TYPE"
                fi
                
                if [ "${'$'}DEBUG" = true ]; then
                	ARGS="${'$'}ARGS --debug"
                fi
                
                if [ ! -z  "${'$'}COMPUTE_NODE" ]; then
                	ARGS="${'$'}ARGS --compute-node ${'$'}COMPUTE_NODE"
                fi
                
                if [ ! -z  "${'$'}PLACEMENT_GROUP" ]; then
                	ARGS="${'$'}ARGS --placement-group-name ${'$'}PLACEMENT_GROUP"
                fi
                
                if [ ! -z  "${'$'}IMAGE" ]; then
                	ARGS="${'$'}ARGS --image-name ${'$'}IMAGE"
                fi
                
                if [ ! -z  "${'$'}INSTANCE_CORES" ]; then
                	ARGS="${'$'}ARGS --instance-cores ${'$'}INSTANCE_CORES"
                fi
                
                if [ ! -z  "${'$'}INSTANCE_RAM" ]; then
                	ARGS="${'$'}ARGS --instance-ram ${'$'}INSTANCE_RAM"
                fi
                
                if [ "${'$'}NO_YT" = true ]; then
                	ARGS="${'$'}ARGS --no-yt"
                fi
                
                if [ "${'$'}FORCE" = true ]; then
                	ARGS="${'$'}ARGS --force"
                fi
                
                if [ "${'$'}IN_PARALLEL" = true ]; then
                	ARGS="${'$'}ARGS --in-parallel"
                fi
                
                yc-nbs-ci-fio-performance-test-suite \
                  --cluster ${'$'}CLUSTER \
                  --test-suite ${'$'}TEST_SUITE \
                  --teamcity \
                  ${'$'}ARGS
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """-u root --rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host --privileged"""
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

    failureConditions {
        executionTimeoutMin = 600
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
        sshAgent {
            id = "BUILD_EXT_2948"
            teamcitySshKey = "id_rsa_overlay"
        }
        swabra {
            id = "swabra"
            filesCleanup = Swabra.FilesCleanup.AFTER_BUILD
        }
    }

    requirements {
        doesNotContain("teamcity.agent.name", "kiwi", "RQ_4057")
        doesNotMatch("teamcity.agent.name", "cm-build-agent-.*", "RQ_5196")
    }
})
