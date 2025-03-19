package Nfs_BuildArcadiaTest.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nfs_BuildArcadiaTest_HwNbsStableLab : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNfsCiBuildArcadiaTest)
    name = "hw-nbs-stable-lab"
    paused = true

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
        param("env.TEST_CASE", "nfs")
    }

    steps {
        script {
            name = "Run build test"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -ex
                
                ARGS="-v"
                
                if [ ! -z  "${'$'}COMPUTE_NODE" ]; then
                	ARGS="${'$'}ARGS --compute-node ${'$'}COMPUTE_NODE"
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
            schedulingPolicy = daily {
                hour = 4
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
            param("cronExpression_dw", "*")
        }
    }
})
