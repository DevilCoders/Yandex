package Nbs_MigrationTest_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.Swabra
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.swabra
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_MigrationTest_HwNbsStableLab_MigrationTest : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiMigrationTest)
    name = "Migration Test"

    publishArtifacts = PublishMode.NORMALLY_FINISHED

    params {
        param("env.TABLET_ID", "72075186224048609")
        param("env.KILL_PERIOD", "300")
        param("env.CHECK_LOAD", "true")
        param("env.KILL_TABLET", "true")
        param("env.TIMEOUT", "19800")
        param("env.INSTANCE_IP", "2a02:6b8:c02:80e:0:417a:0:3a3")
        param("env.CLUSTER", "hw-nbs-stable-lab")
        param("env.COMPUTE_NODE", "sas09-ct7-29.cloud.yandex.net")
        param("env.DISK_NAME", "eternal-1023gb-nonrepl-test-disk")
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
            dockerRunParameters = """-u root --rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host"""
        }
    }

    triggers {
        schedule {
            id = "TRIGGER_1488"
            schedulingPolicy = cron {
                hours = "*/6"
                dayOfWeek = "*"
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
            enableQueueOptimization = false
        }
    }

    features {
        swabra {
            id = "swabra"
            filesCleanup = Swabra.FilesCleanup.AFTER_BUILD
        }
    }

    requirements {
        matches("teamcity.agent.name", "build-agent-.*", "RQ_4829")
    }
})
