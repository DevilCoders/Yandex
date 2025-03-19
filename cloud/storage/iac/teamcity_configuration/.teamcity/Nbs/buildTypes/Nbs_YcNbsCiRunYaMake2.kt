package Nbs.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nbs_YcNbsCiRunYaMake2 : Template({
    name = "yc-nbs-ci-run-ya-make-2"

    params {
        param("sandbox.test", "True")
        param("sandbox.build_type", "")
        param("sandbox.ya_timeout", "")
        param("sandbox.targets", "")
        param("sandbox.test_threads", "32")
        param("sandbox.keep_on", "True")
        param("sandbox.checkout_arcadia_from_url", "")
        param("sandbox.build_system", "ya_force")
        param("sandbox.use_aapi_fuse", "True")
        param("sandbox.junit_report", "True")
        param("sandbox.definition_flags", "")
        param("sandbox.sanitize", "")
    }

    features {
        feature {
            id = "ru.yandex.teamcity.plugin.sandbox.SandboxTaskLauncherFeature"
            type = "ru.yandex.teamcity.plugin.sandbox.SandboxTaskLauncherFeature"
            param("owner", "TEAMCITY")
            param("sandbox_task_priority", "SERVICE:NORMAL")
            param("kill_timeout", "15000")
            param("sandbox_task_type", "YA_MAKE")
            param("secure:authorisation_token", "credentialsJSON:9f44c742-f345-4805-b419-042d17c38730")
            param("description", "build")
            param("container_resource", "2185033214")
        }
    }

    requirements {
        doesNotMatch("teamcity.agent.name", "cm-build-agent-.*", "RQ_5110")
    }
})
