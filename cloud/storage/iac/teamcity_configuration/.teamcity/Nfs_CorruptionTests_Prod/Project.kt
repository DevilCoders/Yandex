package Nfs_CorruptionTests_Prod

import Nfs_CorruptionTests_Prod.buildTypes.*
import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_CorruptionTests_Prod")
    name = "prod"
    archived = true

    buildType(Nfs_CorruptionTests_Prod_CorruptionTests64mbBlocksize)
    buildType(Nfs_CorruptionTests_Prod_CorruptionTests4kbBlocksize)
})
