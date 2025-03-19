package _Self.vcsRoots

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.vcs.SvnVcsRoot

object YandexCloudBlockstoreSvn : SvnVcsRoot({
    name = "Yandex Cloud Blockstore SVN"
    url = "svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/cloud/blockstore"
    userName = "robot-yc-nbs"
    password = "credentialsJSON:f797fd19-4d86-4c07-9a5e-18e7bc9e95bd"
    uploadedKey = "robot-yc-nbs"
    labelingRules = ""
    labelingMessage = "%build.vcs.number%"
})
