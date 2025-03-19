package Nfs_CorruptionTests_Preprod

import Nfs_CorruptionTests_Preprod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_CorruptionTests_Preprod")
    name = "preprod"
    archived = true

    buildType(Nfs_CorruptionTests_Preprod_CorruptionTests64mbBlocksize)
    buildType(Nfs_CorruptionTests_Preprod_CorruptionTests4kbBlocksize)

    params {
        param("env.ZONE_ID", "ru-central1-a")
    }
})
