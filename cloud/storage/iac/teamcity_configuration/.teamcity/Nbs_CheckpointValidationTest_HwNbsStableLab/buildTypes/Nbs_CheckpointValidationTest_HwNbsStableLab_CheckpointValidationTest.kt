package Nbs_CheckpointValidationTest_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nbs_CheckpointValidationTest_HwNbsStableLab_CheckpointValidationTest : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiCheckpointValidationTest)
    name = "Checkpoint Validation Test"

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
    }

    steps {
        script {
            name = "Run checkpoint validation test"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -ex
                
                ARGS=""
                
                if [ ! -z  "${'$'}SERVICE_ACCOUNT_ID" ]; then
                	ARGS="${'$'}ARGS --service-account-id ${'$'}SERVICE_ACCOUNT_ID"
                fi
                
                yc-nbs-ci-checkpoint-validation-test \
                  --teamcity --verbose \
                  --cluster ${'$'}CLUSTER \
                  --validator-path /usr/bin/checkpoint-validator \
                  ${'$'}ARGS
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host"""
        }
    }
})
