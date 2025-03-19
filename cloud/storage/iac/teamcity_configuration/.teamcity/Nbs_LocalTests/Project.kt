package Nbs_LocalTests

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nbs_LocalTests")
    name = "Local Tests"

    params {
        param("env.DISK_SPACE", "200")
        param("env.TEST_TARGETS", "")
        param("env.RAM", "64")
        param("env.TIMEOUT", "18000")
        param("env.CORES", "32")
        param("env.ARCADIA_URL", "arcadia:/arc/trunk/arcadia")
        param("env.DISABLE_TEST_TIMEOUT", "true")
    }

    subProject(Nbs_LocalTests_Stable214.Project)
    subProject(Nbs_LocalTests_Stable222.Project)
})
