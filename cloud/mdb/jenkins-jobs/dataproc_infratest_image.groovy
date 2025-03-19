job("dataproc_infratest_image"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/dataproc_infratest_image/buildTimeGraph/png' />""")
    triggers {
        cron('@daily')
    }

    logRotator(7, 30)
    label('dbaas')

    authenticationToken('token')

    parameters {
        stringParam('trigger_id')
    }

    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        timestamps()
        colorizeOutput('xterm')
        credentialsBinding {
            string('ARCANUMAUTH', 'arcanum_token')
            string('YAV_OAUTH', 'robot-pgaas-ci-yav_oauth')
        }
        buildName('${BUILD_NUMBER}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps{
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('export PATH=$PATH:$(pwd)/arcadia/ && cd arcadia/cloud/mdb/dataproc-infra-tests && LC_ALL=en_US.UTF-8 make cache')
    }

    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/logs/**/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/meta/**/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/redis/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/junit/*')
            allowEmpty(true)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
