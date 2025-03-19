package DiskManager

import DiskManager.buildTypes.*
import DiskManager.vcsRoots.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("DiskManager")
    name = "Disk Manager"
    description = "High level API on top of Network Blockstore and Snapshot"

    vcsRoot(DiskManager_YandexCloudDiskManagerSvn)
    vcsRoot(DiskManager_YandexCloudDiskManagerSvn1)
    vcsRoot(DiskManager_YandexCloudDiskManagerSvn11)
    vcsRoot(DiskManager_YandexCloudDiskManagerSvn111)
    vcsRoot(DiskManager_YandexCloudDiskManagerSvn1111)

    template(DiskManager_YcCleanup)
    template(DiskManager_RunWithScript)
    template(DiskManager_YcDiskManagerCiAcceptanceTestSuite)

    params {
        password("env.sandbox_oauth", "credentialsJSON:6c34e40c-1546-49fd-af1a-71435dfdbc80", display = ParameterDisplay.HIDDEN)
        password("env.z2_api_key", "credentialsJSON:52eaa770-86f6-4ae6-a48f-b5f15608147b", display = ParameterDisplay.HIDDEN)
        param("svn_basename", "cloud/disk_manager")
        param("svn_teamcity_scripts_path", "cloud/disk_manager/build/ci/teamcity")
        param("env.branch_path", "trunk")
    }

    subProject(DiskManager_Tests.Project)
})
