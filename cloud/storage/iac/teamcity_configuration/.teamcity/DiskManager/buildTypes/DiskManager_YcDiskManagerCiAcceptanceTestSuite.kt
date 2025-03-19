package DiskManager.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.Swabra
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.perfmon
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.swabra
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_YcDiskManagerCiAcceptanceTestSuite : Template({
    name = "yc-disk-manager-ci-acceptance-test-suite"

    allowExternalStatus = true
    artifactRules = """
        ssh-*.log
        tcpdump-*.txt
    """.trimIndent()

    params {
        param("env.ZONE_ID", "ru-central1-a")
        param("env.CLUSTER", "hw-nbs-stable-lab")
        param("env.TEST_TYPE", "")
        param("yc-nbs-ci-tools.docker.image", "registry.yandex.net/yandex-cloud/yc-nbs-ci-tools:latest")
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
                	COMMON_ARGS="${'$'}COMMON_ARGS --ipc-type ${'$'}IPC_TYPE"
                fi
                
                if [ ! -z  "${'$'}PLACEMENT_GROUP_NAME" ]; then
                	COMMON_ARGS="${'$'}COMMON_ARGS --placement-group-name ${'$'}PLACEMENT_GROUP_NAME"
                fi
                
                if [ ! -z "${'$'}COMPUTE_NODE" ]; then
                	COMMON_ARGS="${'$'}COMMON_ARGS --compute-node ${'$'}COMPUTE_NODE"
                fi
                
                if [ ! -z  "${'$'}ZONE_ID" ]; then
                	COMMON_ARGS="${'$'}COMMON_ARGS --zone-id ${'$'}ZONE_ID"
                fi
                
                if [ ! -z  "${'$'}INSTANCE_CORES" ]; then
                	COMMON_ARGS="${'$'}COMMON_ARGS --instance-cores ${'$'}INSTANCE_CORES"
                fi
                
                if [ ! -z  "${'$'}INSTANCE_RAM" ]; then
                	COMMON_ARGS="${'$'}COMMON_ARGS --instance-ram ${'$'}INSTANCE_RAM"
                fi
                
                if [ "${'$'}DEBUG" = true ]; then
                	COMMON_ARGS="${'$'}COMMON_ARGS --debug"
                fi
                
                if [ "${'$'}VERBOSE" = true ]; then
                	COMMON_ARGS="${'$'}COMMON_ARGS --verbose"
                fi
                
                if [ "${'$'}CONSERVE_SNAPSHOTS" = true ]; then
                	COMMON_ARGS="${'$'}COMMON_ARGS --conserve-snapshots"
                fi
                
                if [ ! -z  "${'$'}TEST_SUITE" ]; then
                	TEST_TYPE_ARGS="${'$'}TEST_TYPE_ARGS --test-suite ${'$'}TEST_SUITE"
                fi
                
                if [ ! -z  "${'$'}VERIFY_TEST" ]; then
                	TEST_TYPE_ARGS="${'$'}TEST_TYPE_ARGS --verify-test ${'$'}VERIFY_TEST"
                fi
                
                if [ ! -z  "${'$'}DISK_SIZE" ]; then
                	TEST_TYPE_ARGS="${'$'}TEST_TYPE_ARGS --disk-size ${'$'}DISK_SIZE"
                fi
                
                if [ ! -z  "${'$'}DISK_BLOCKSIZE" ]; then
                	TEST_TYPE_ARGS="${'$'}TEST_TYPE_ARGS --disk-blocksize ${'$'}DISK_BLOCKSIZE"
                fi
                
                if [ ! -z  "${'$'}DISK_TYPE" ]; then
                	TEST_TYPE_ARGS="${'$'}TEST_TYPE_ARGS --disk-type ${'$'}DISK_TYPE"
                fi
                
                if [ ! -z  "${'$'}DISK_WRITE_SIZE_PERCENTAGE" ]; then
                	TEST_TYPE_ARGS="${'$'}TEST_TYPE_ARGS --disk-write-size-percentage ${'$'}DISK_WRITE_SIZE_PERCENTAGE"
                fi
                
                yc-disk-manager-ci-acceptance-test \
                  --teamcity \
                  --cluster ${'$'}CLUSTER \
                  --acceptance-test /usr/bin/acceptance-test \
                  ${'$'}COMMON_ARGS \
                  ${'$'}TEST_TYPE \
                  ${'$'}TEST_TYPE_ARGS
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
                hour = 0
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
