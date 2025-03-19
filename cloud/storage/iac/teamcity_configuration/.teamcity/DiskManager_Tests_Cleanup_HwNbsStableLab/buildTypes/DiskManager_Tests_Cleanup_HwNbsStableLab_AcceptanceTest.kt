package DiskManager_Tests_Cleanup_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object DiskManager_Tests_Cleanup_HwNbsStableLab_AcceptanceTest : BuildType({
    templates(_Self.buildTypes.Nbs_Cleanup_YcCleanup)
    name = "Acceptance test"
    description = "Remove stale entities from DM Acceptance test"
    paused = true

    params {
        param("env.IMAGE_TTL_DAYS", "")
        param("env.INSTANCE_REGEX", """^"acceptance-test-acceptance-(small|medium|big|enormous)-[0-9]+"${'$'}""")
        param("env.DISK_REGEX", """^"acceptance-test-acceptance-(small|medium|big|enormous)-[0-9]+"${'$'}""")
        param("env.SNAPSHOT_TTL_DAYS", "")
    }
    
    disableSettings("RUNNER_13013", "RUNNER_13101")
})
