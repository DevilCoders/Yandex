package DiskManager_Tests_EternalAcceptanceTest

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager_Tests_EternalAcceptanceTest")
    name = "Eternal acceptance test"

    params {
        param("env.TEST_TYPE", "eternal")
        param("env.DISK_TYPE", "network-ssd")
        param("env.CONSERVE_SNAPSHOTS", "true")
        param("env.DISK_BLOCKSIZE", "4096")
        param("env.DEBUG", "false")
    }

    subProject(DiskManager_Tests_EternalAcceptanceTest_ProdNoAgentBuilds.Project)
    subProject(DiskManager_Tests_EternalAcceptanceTest_Prod.Project)
    subProject(DiskManager_Tests_EternalAcceptanceTest_PreprodNoAgentBuilds.Project)
    subProject(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLabNoAgentBuilds.Project)
    subProject(DiskManager_Tests_EternalAcceptanceTest_HwNbsStableLab.Project)
    subProject(DiskManager_Tests_EternalAcceptanceTest_Preprod.Project)
})
