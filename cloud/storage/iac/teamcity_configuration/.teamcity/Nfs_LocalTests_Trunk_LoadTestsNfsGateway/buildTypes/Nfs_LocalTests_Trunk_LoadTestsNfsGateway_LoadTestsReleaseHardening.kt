package Nfs_LocalTests_Trunk_LoadTestsNfsGateway.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*

object Nfs_LocalTests_Trunk_LoadTestsNfsGateway_LoadTestsReleaseHardening : BuildType({
    templates(Nfs.buildTypes.Nfs_YcNbsCiRunYaMake)
    name = "Load Tests (release, hardening)"
    paused = true
})
