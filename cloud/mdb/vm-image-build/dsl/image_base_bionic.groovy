job("image-base-bionic"){
    description("""
<h2>Generation time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/dbaas-vm-image/buildTimeGraph/png' />""")
    logRotator(7, 200, 7, 200)
    concurrentBuild(false)
    label('dbaas')

    triggers {
        cron('H 2 * * *')
    }

    wrappers {
        timestamps()
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
        colorizeOutput('xterm')
    }

    steps{
        shell('''`flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/
                 `cd arcadia
                 `svn cleanup
                 `svn up
                 `cd cloud/mdb/vm-image-build
                 `make clean
                 `make base-bionic.img
                 `mv base-bionic* ../../../..'''.stripMargin('`'))
    }

    publishers {
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
        archiveArtifacts {
            pattern('base-bionic*')
            onlyIfSuccessful()
            allowEmpty(false)
        }
        mailer('mdb-cc@yandex-team.ru', true, true)
    }
}
