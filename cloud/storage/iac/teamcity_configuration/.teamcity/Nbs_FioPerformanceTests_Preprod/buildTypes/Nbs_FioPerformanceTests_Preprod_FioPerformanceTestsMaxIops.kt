package Nbs_FioPerformanceTests_Preprod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_FioPerformanceTests_Preprod_FioPerformanceTestsMaxIops : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiFioPerformanceTestSuite)
    name = "Fio Performance Tests (max iops)"
    paused = true

    params {
        param("env.TEST_SUITE", "max_iops")
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
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host"""
        }
    }

    triggers {
        schedule {
            id = "TRIGGER_1544"
            schedulingPolicy = daily {
                hour = 0
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        schedule {
            id = "TRIGGER_1615"
            schedulingPolicy = daily {
                hour = 4
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
        schedule {
            id = "TRIGGER_1616"
            schedulingPolicy = daily {
                hour = 8
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
    
    disableSettings("TRIGGER_1461")
})
