package DiskManager.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.exec
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object DiskManager_RunWithScript : Template({
    name = "Run With Script"

    buildNumberPattern = "%build.counter%[%build.vcs.number%]"

    vcs {
        root(DiskManager.vcsRoots.DiskManager_YandexCloudDiskManagerSvn1111)

        checkoutMode = CheckoutMode.MANUAL
    }

    steps {
        script {
            name = "Checkout SVN svn_teamcity_scripts_path"
            id = "RUNNER_14876"
            scriptContent = """
                set -u
                set -e
                
                rm -rf teamcity
                
                svn export -q svn+ssh://arcadia.yandex.ru/arc/%env.branch_path%/arcadia/%svn_teamcity_scripts_path%@%build.vcs.number% teamcity --force --non-interactive
            """.trimIndent()
        }
        exec {
            name = "Run teamcity_script"
            id = "RUNNER_15022"
            path = "python"
            arguments = "%teamcity_script%"
        }
    }

    requirements {
        exists("svn.version", "RQ_23203")
        exists("Python", "RQ_23204")
        matches("teamcity.agent.name", "kiwi-.*", "RQ_23214")
    }
})
