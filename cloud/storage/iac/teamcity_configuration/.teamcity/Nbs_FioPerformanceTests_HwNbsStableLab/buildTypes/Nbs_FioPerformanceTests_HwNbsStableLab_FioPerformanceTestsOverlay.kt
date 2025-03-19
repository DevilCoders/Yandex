package Nbs_FioPerformanceTests_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.notifications
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule

object Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlay : BuildType({
    name = "Fio Performance Tests (overlay)"
    description = """Run all builds from "fio permormance tests (overlay builds)" project in parallel"""
    paused = true

    type = BuildTypeSettings.Type.COMPOSITE

    vcs {
        showDependenciesChanges = true
    }

    triggers {
        schedule {
            schedulingPolicy = daily {
                hour = 22
            }
            branchFilter = ""
            triggerBuild = always()
            withPendingChangesOnly = false
            enableQueueOptimization = false
        }
    }

    features {
        notifications {
            notifierSettings = emailNotifier {
                email = "vlad-serikov@yandex-team.ru"
            }
            buildFailedToStart = true
            buildFailed = true
            firstBuildErrorOccurs = true
        }
    }

    dependencies {
        snapshot(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds.buildTypes.Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay1) {
            reuseBuilds = ReuseBuilds.NO
        }
        snapshot(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds.buildTypes.Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay2) {
            reuseBuilds = ReuseBuilds.NO
        }
        snapshot(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds.buildTypes.Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay3) {
            reuseBuilds = ReuseBuilds.NO
        }
        snapshot(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds.buildTypes.Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay4) {
            reuseBuilds = ReuseBuilds.NO
        }
        snapshot(Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds.buildTypes.Nbs_FioPerformanceTests_HwNbsStableLab_FioPerformanceTestsOverlayBuilds_FioPerformanceTestsOverlay5) {
            reuseBuilds = ReuseBuilds.NO
        }
    }
})
