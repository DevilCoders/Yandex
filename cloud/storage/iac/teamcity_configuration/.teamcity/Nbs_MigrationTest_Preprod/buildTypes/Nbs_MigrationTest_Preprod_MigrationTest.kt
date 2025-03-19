package Nbs_MigrationTest_Preprod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_MigrationTest_Preprod_MigrationTest : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiMigrationTest)
    name = "Migration Test"

    params {
        param("env.SERVICE_ACCOUNT_ID", "bfbf9rg1scirhqet6kn8")
        param("env.TABLET_ID", "")
        param("env.CHECK_LOAD", "true")
        param("env.TIMEOUT", "19800")
        param("env.INSTANCE_IP", "2a02:6b8:c0e:501:0:f805:40b1:251")
        param("env.CLUSTER", "preprod")
        param("env.COMPUTE_NODE", "nbs-control-vla2.svc.cloud-preprod.yandex.net")
        param("env.DISK_NAME", "eternal-1023gb-nonrepl-test-disk")
        param("yc-nbs-ci-tools.docker.image", "registry.yandex.net/yandex-cloud/yc-nbs-ci-tools:latest")
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
                
                if [ "${'$'}KILL_TABLET" = true ]; then
                	ARGS="${'$'}ARGS --kill-tablet --kill-period ${'$'}KILL_PERIOD"
                fi
                 
                if [ "${'$'}CHECK_LOAD" = true ]; then
                	ARGS="${'$'}ARGS --check-load "
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
        stepsOrder = arrayListOf("RUNNER_6805")
    }

    triggers {
        schedule {
            id = "TRIGGER_1488"
            schedulingPolicy = daily {
                hour = 14
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
            enableQueueOptimization = false
            param("cronExpression_hour", "*/6")
            param("cronExpression_dw", "*")
        }
    }
})
