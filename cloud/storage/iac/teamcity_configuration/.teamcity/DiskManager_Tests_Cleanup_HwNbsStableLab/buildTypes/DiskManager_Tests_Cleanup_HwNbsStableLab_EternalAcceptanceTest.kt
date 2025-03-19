package DiskManager_Tests_Cleanup_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object DiskManager_Tests_Cleanup_HwNbsStableLab_EternalAcceptanceTest : BuildType({
    templates(_Self.buildTypes.Nbs_Cleanup_YcCleanup)
    name = "Eternal acceptance test"
    description = "Remove stale entities from DM eternal acceptance test"
    paused = true

    params {
        param("env.IMAGE_TTL_DAYS", "")
        param("env.INSTANCE_REGEX", """^"acceptance-test-eternal-[0-9]+"${'$'}""")
        param("env.DISK_REGEX", """^"acceptance-test-eternal-[0-9]+(b|kib|mib|gib|tib)-[0-9]+(b|kib|mib|gib|tib)-[0-9]+"${'$'}""")
        param("env.SNAPSHOT_TTL_DAYS", "")
        param("env.DISK_TTL_DAYS", "5")
    }
    
    disableSettings("RUNNER_13013", "RUNNER_13101")
})
