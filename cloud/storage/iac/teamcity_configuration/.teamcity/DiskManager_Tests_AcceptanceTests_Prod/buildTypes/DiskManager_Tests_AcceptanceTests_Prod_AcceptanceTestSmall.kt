package DiskManager_Tests_AcceptanceTests_Prod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.notifications
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object DiskManager_Tests_AcceptanceTests_Prod_AcceptanceTestSmall : BuildType({
    templates(DiskManager.buildTypes.DiskManager_YcDiskManagerCiAcceptanceTestSuite)
    name = "Acceptance test (small)"
    description = """Disk_count: 4; Disk_type: "network_ssd"; Disk_size (GiB): [2, 4, 8, 16]; Disk_blocksize: 4KiB; Disk_validation_size: 50%; Disk_validation_blocksize: 4MiB"""
    paused = true

    params {
        param("env.INSTANCE_RAM", "2")
        param("env.TEST_SUITE", "small")
        param("env.INSTANCE_CORES", "2")
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
        stepsOrder = arrayListOf("RUNNER_6805")
    }

    triggers {
        schedule {
            id = "TRIGGER_1461"
            schedulingPolicy = daily {
                hour = 0
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }

    features {
        notifications {
            id = "BUILD_EXT_678"
            notifierSettings = emailNotifier {
                email = "vlad-serikov@yandex-team.ru"
            }
            buildFailedToStart = true
            buildFailed = true
            firstBuildErrorOccurs = true
        }
    }
})
