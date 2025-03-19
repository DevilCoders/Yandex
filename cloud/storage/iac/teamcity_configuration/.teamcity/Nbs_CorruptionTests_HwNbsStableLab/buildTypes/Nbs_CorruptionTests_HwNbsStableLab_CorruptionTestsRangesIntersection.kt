package Nbs_CorruptionTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_CorruptionTests_HwNbsStableLab_CorruptionTestsRangesIntersection : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiCorruptionTestSuite)
    name = "Corruption Tests (ranges intersection)"
    paused = true

    params {
        param("env.TEST_SUITE", "ranges-intersection")
    }

    steps {
        script {
            name = "Run corruption tests"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -ex
                
                yc-nbs-ci-corruption-test-suite \
                  --teamcity \
                  --cluster ${'$'}CLUSTER \
                  --test-suite ${'$'}TEST_SUITE \
                  --verify-test-path /usr/bin/verify-test
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659""""
        }
        stepsOrder = arrayListOf("RUNNER_6805")
    }

    triggers {
        schedule {
            id = "TRIGGER_534"
            schedulingPolicy = daily {
                hour = 8
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }
})
