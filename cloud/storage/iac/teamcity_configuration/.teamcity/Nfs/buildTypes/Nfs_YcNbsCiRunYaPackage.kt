package Nfs.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.perfmon
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object Nfs_YcNbsCiRunYaPackage : Template({
    name = "yc-nbs-ci-run-ya-package"

    allowExternalStatus = true
    artifactRules = "built_packages.json"
    publishArtifacts = PublishMode.SUCCESSFUL

    params {
        param("env.TASK_ID", "Cloud_Packages_SignNbsPackages")
        password("env.TEAMCITY_OAUTH_TOKEN", "credentialsJSON:5f84ab08-88a2-4d7d-9fe5-4283b63f2c42", display = ParameterDisplay.HIDDEN)
        param("env.PROJECT_ID", "Cloud_Packages")
        param("env.TIMEOUT", "7200")
        param("env.BUILD_TYPE", "release")
        param("env.ARCADIA_URL", "arcadia:/arc/trunk/arcadia")
        param("env.DEBIAN_DISTRIBUTION", "testing")
        param("env.PACKAGES_TO_BUILD", "")
        param("env.PUBLISH_TO", "yandex-cloud")
        password("env.SANDBOX_OAUTH_TOKEN", "credentialsJSON:9f44c742-f345-4805-b419-042d17c38730", display = ParameterDisplay.HIDDEN)
    }

    steps {
        script {
            name = "Build packages"
            id = "RUNNER_6805"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                echo ${'$'}PACKAGES_TO_BUILD | sed 's/;/\n/g' > packages_to_build.txt
                
                yc-nbs-ci-run-ya-package \
                  --in packages_to_build.txt \
                  --out built_packages.json \
                  --teamcity \
                  --timeout ${'$'}TIMEOUT \
                  --arcadia-url ${'$'}ARCADIA_URL \
                  --publish-to ${'$'}PUBLISH_TO \
                  --debian-distribution ${'$'}DEBIAN_DISTRIBUTION
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
        }
        script {
            name = "Sign packages"
            id = "RUNNER_11411"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                yc-nbs-ci-sign-packages \
                  --teamcity \
                  --task-id ${'$'}TASK_ID \
                  --project-id ${'$'}PROJECT_ID
                  
                sleep 1200
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = "--network host"
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
        doesNotMatch("teamcity.agent.name", "cm-build-agent-.*", "RQ_5201")
    }
})
