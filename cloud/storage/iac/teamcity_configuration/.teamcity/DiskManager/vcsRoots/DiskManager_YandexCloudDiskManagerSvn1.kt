package DiskManager.vcsRoots

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.vcs.SvnVcsRoot

object DiskManager_YandexCloudDiskManagerSvn1 : SvnVcsRoot({
    name = "Yandex Cloud Disk Manager SVN (1)"
    url = "svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/cloud"
    userName = "robot-kikimr-dev"
    password = "credentialsJSON:ec560d4a-7908-4a58-a77c-ba4e245b575e"
    configDir = ""
    uploadedKey = "robot-kikimr-dev"
    labelingRules = ""
    labelingMessage = "%build.vcs.number%"
})
