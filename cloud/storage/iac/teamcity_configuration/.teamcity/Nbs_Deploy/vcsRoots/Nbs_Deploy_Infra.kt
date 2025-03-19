package Nbs_Deploy.vcsRoots

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.vcs.GitVcsRoot

object Nbs_Deploy_Infra : GitVcsRoot({
    name = "Infra"
    url = "ssh://git@bb.yandex-team.ru/cloud/infra.git"
    branch = "refs/heads/master"
    branchSpec = "+:*"
    authMethod = uploadedKey {
        uploadedKey = "robot-yc-bitbucket"
    }
    param("useAlternates", "true")
})
