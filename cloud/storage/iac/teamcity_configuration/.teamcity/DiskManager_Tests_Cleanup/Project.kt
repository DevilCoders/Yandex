package DiskManager_Tests_Cleanup

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_Cleanup")
    name = "Cleanup"
    description = "Remove stale instances, disks, images and snapshots after DM tests"

    subProject(DiskManager_Tests_Cleanup_Preprod.Project)
    subProject(DiskManager_Tests_Cleanup_HwNbsStableLab.Project)
    subProject(DiskManager_Tests_Cleanup_Prod.Project)
})
