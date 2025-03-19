package Nbs_NrdCheckEmptinessTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_NrdCheckEmptinessTests_HwNbsStableLab_ZeroCheck : BuildType({
    templates(Nbs.buildTypes.Nbs_YcNbsCiCheckNrdEmptinessTest)
    name = "Zero Check"
    paused = true

    params {
        param("env.CLUSTER", "hw-nbs-stable-lab")
        param("env.DISK_SIZE", "465")
    }

    triggers {
        schedule {
            id = "TRIGGER_534"
            schedulingPolicy = cron {
                hours = "/3"
                dayOfWeek = "*"
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
            param("hour", "6")
        }
    }
})
