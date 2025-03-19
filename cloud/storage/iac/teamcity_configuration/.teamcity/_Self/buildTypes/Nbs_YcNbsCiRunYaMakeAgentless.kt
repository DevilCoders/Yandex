package _Self.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.notifications
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.swabra

object Nbs_YcNbsCiRunYaMakeAgentless : Template({
    name = "yc-nbs-ci-run-sandbox-agentless"

    maxRunningBuilds = 1

    params {
        param("owner", "TEAMCITY")
        param("configs_dir", "cloud/blockstore/sandbox/configs")
        param("teamcity.vcs.branchDecreaseThreshold", "25000")
        password("oauth_token", "credentialsJSON:9f44c742-f345-4805-b419-042d17c38730", label = "oauth_token")
        param("sandbox.binary_executor_release_type", "stable")
        param("teamcity.buildQueue.allowMerging", "false")
        param("output_dir", "%teamcity.build.checkoutDir%/artifacts")
        param("branch_spec", "+:arcadia/(trunk)")
        param("sandbox.config_path", "")
        param("sandbox.root_path", "%configs_dir%")
    }

    vcs {
        root(AbsoluteId("Root_Arcadia"))

        checkoutMode = CheckoutMode.MANUAL
        cleanCheckout = true
    }

    steps {
        step {
            name = "Download sandbox task resources"
            id = "RUNNER_6640"
            type = "sandbox_launcher_run_type"
        }
    }

    failureConditions {
        executionTimeoutMin = 600
    }

    features {
        feature {
            id = "ru.yandex.teamcity.plugin.sandbox.SandboxTaskLauncherFeature"
            type = "ru.yandex.teamcity.plugin.sandbox.SandboxTaskLauncherFeature"
            param("owner", "%owner%")
            param("sandbox_task_type", "NBS_TEAMCITY_RUNNER")
            param("secure:authorisation_token", "credentialsJSON:e880f320-9eee-4192-b8f4-09f5f273043b")
            param("description", "Run NbsTeamcityRunner in sandbox")
        }
        notifications {
            id = "BUILD_EXT_2908"
            notifierSettings = emailNotifier {
                email = "vlad-serikov@yandex-team.ru"
            }
            buildFailedToStart = true
            buildFailed = true
            firstBuildErrorOccurs = true
        }
        freeDiskSpace {
            id = "jetbrains.agent.free.space"
            requiredSpace = "1gb"
            failBuild = true
        }
        swabra {
            id = "swabra"
            verbose = true
        }
    }

    requirements {
        doesNotContain("teamcity.agent.name", "kiwi", "RQ_3991")
        matches("teamcity.agent.name", "build-agent-.*", "RQ_3995")
    }
})
