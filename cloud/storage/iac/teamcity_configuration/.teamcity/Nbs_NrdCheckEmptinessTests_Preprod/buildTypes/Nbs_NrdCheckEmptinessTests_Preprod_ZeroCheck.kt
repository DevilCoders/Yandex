package Nbs_NrdCheckEmptinessTests_Preprod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nbs_NrdCheckEmptinessTests_Preprod_ZeroCheck : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiCheckNrdEmptinessTest)
    name = "Zero Check"
    paused = true

    params {
        param("env.DISK_SIZE", "186")
    }

    steps {
        script {
            name = "Run check emptiness test"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -ex
                
                ARGS=""
                
                if [ ! -z  "${'$'}DISK_SIZE" ]; then
                	ARGS="${'$'}ARGS --disk-size ${'$'}DISK_SIZE"
                fi
                
                if [ ! -z  "${'$'}IO_DEPTH" ]; then
                	ARGS="${'$'}ARGS --io-depth ${'$'}IO_DEPTH"
                fi
                
                if [ ! -z  "${'$'}COMPUTE_NODE" ]; then
                	ARGS="${'$'}ARGS --compute-node ${'$'}COMPUTE_NODE"
                fi
                
                if [ ! -z  "${'$'}HOST_GROUP" ]; then
                	ARGS="${'$'}ARGS --host-group ${'$'}HOST_GROUP"
                fi
                
                yc-nbs-ci-check-nrd-disk-emptiness-test \
                  --teamcity \
                  --cluster ${'$'}CLUSTER \
                  --verify-test-path /usr/bin/verify-test \
                  ${'$'}ARGS
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host --cap-add SYS_PTRACE --privileged"""
        }
    }
})
