package Nbs.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.perfmon
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_YcNbsCiDeployPackages : Template({
    name = "yc-nbs-ci-deploy-packages"

    allowExternalStatus = true
    enablePersonalBuilds = false
    type = BuildTypeSettings.Type.DEPLOYMENT
    maxRunningBuilds = 1
    publishArtifacts = PublishMode.SUCCESSFUL

    params {
        password("env.Z2_TOKEN", "credentialsJSON:52eaa770-86f6-4ae6-a48f-b5f15608147b", display = ParameterDisplay.HIDDEN)
        param("env.CLUSTER", "")
    }

    steps {
        script {
            name = "Deploy packages"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -ex
                
                ARGS=""
                
                if [ -f "binaries/built_packages.json" ]; then
                	ARGS="${'$'}ARGS --packages binaries/built_packages.json"
                fi
                
                if [ -f "configs/built_packages.json" ]; then
                	ARGS="${'$'}ARGS --packages configs/built_packages.json"
                fi
                
                yc-nbs-ci-deploy-packages \
                  --cluster ${'$'}CLUSTER \
                  --service ${'$'}SERVICE \
                  --teamcity \
                  ${'$'}ARGS
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = "--network host"
        }
    }

    triggers {
        schedule {
            id = "TRIGGER_371"
            schedulingPolicy = daily {
                hour = 0
            }
            triggerBuild = always()
            withPendingChangesOnly = false
        }
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
    }

    requirements {
        doesNotContain("teamcity.agent.name", "kiwi", "RQ_4057")
        doesNotMatch("teamcity.agent.name", "cm-build-agent-.*", "RQ_5127")
    }
})
