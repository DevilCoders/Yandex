package Nfs_LocalTests_Trunk

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.Project

object Project : Project({
    id("Nfs_LocalTests_Trunk")
    name = "Trunk"

    params {
        param("env.ARCADIA_URL", "arcadia:/arc/trunk/arcadia")
        param("branch_name", "trunk")
    }

    subProject(Nfs_LocalTests_Trunk_FunctionalTestsNfsGateway.Project)
    subProject(Nfs_LocalTests_Trunk_LoadTestsGatewayNoAgentBuilds.Project)
    subProject(Nfs_LocalTests_Trunk_LoadTestsNfsGateway.Project)
    subProject(Nfs_LocalTests_Trunk_LoadTestsNoAgentBuilds.Project)
    subProject(Nfs_LocalTests_Trunk_FunctionalTestsNoAgentBuilds.Project)
    subProject(Nfs_LocalTests_Trunk_FunctionalTests.Project)
    subProject(Nfs_LocalTests_Trunk_LoadTests.Project)
    subProject(Nfs_LocalTests_Trunk_FunctionalTestsGatewayNoAgentBuilds.Project)
})
