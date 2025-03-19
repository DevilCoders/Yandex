package DiskManager_Tests_Cleanup_HwNbsStableLab.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object DiskManager_Tests_Cleanup_HwNbsStableLab_AcceptanceTestCommon : BuildType({
    templates(_Self.buildTypes.Nbs_Cleanup_YcCleanup)
    name = "Acceptance test (common)"
    description = "Remove stale entities from acceptance-test binary (common)"
    paused = true

    params {
        param("env.INSTANCE_TTL_DAYS", "")
        param("env.SNAPSHOT_TTL_DAYS", "2")
        param("env.DISK_REGEX", """^"acceptance-test-disk-[0-9]+"${'$'}""")
        param("env.IMAGE_REGEX", """^"acceptance-test-image-[0-9]+"${'$'}""")
        param("env.SNAPSHOT_REGEX", """^"acceptance-test-snapshot-[0-9]+"${'$'}""")
    }
    
    disableSettings("RUNNER_6805")
})
