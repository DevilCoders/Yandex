job("arcadia_infrastructure_redis_compute"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/arcadia_infrastructure_redis_compute/buildTimeGraph/png' />""")
    logRotator(30, 200, 30, 200)
    label('dbaas')
    concurrentBuild()

    authenticationToken('token')

    parameters {
        stringParam('ARCANUMID')
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
        buildName('${ENV,var="ARCANUMID"} ${BUILD_NUMBER}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps{
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia && echo "Arcanum ID: $ARCANUMID" && if [ "$ARCANUMID" != "trunk" ]; then retry bash -c "(./ya unshelve -d -a $ARCANUMID || ./ya unshelve -a $ARCANUMID)"; fi')
        shell('cd arcadia && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia/cloud/mdb/dataproc-infra-tests && LC_ALL=en_US.UTF-8 make yandex')
        shell('export PATH=$PATH:$(pwd)/arcadia/ && cd arcadia/cloud/mdb/dataproc-infra-tests && LC_ALL=en_US.UTF-8 make start_redis_env')
        shell('export PATH=$PATH:$(pwd)/arcadia/ && cd arcadia/cloud/mdb/dataproc-infra-tests && DISABLE_INIT=true LC_ALL=en_US.UTF-8 make redis junit-directory=junit')
    }

    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/jobresults/**/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/logs/**/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/meta/**/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/redis/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/junit/*')
            allowEmpty(true)
        }
        postBuildTask {
            task('.*', 'export PATH=$PATH:$(pwd)/arcadia/ && cd arcadia/cloud/mdb/dataproc-infra-tests && make clean')
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
