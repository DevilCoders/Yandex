job("arcadia-ch-tools-test-func"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/arcadia-ch-tools-test-func/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')

    authenticationToken('token')

    parameters {
        stringParam('CLICKHOUSE_VERSION')
        stringParam('ARCANUMID', 'trunk')
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
        buildName('${ENV,var="ARCANUMID"} ${BUILD_NUMBER} ${CLICKHOUSE_VERSION}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps{
        shell('env')
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia && echo "Arcanum ID: $ARCANUMID" && if [ "$ARCANUMID" != "trunk" ]; then retry bash -c "(./ya unshelve -d -a $ARCANUMID || ./ya unshelve -a $ARCANUMID)"; fi')
        shell('cd arcadia && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia/cloud/mdb/clickhouse/tools/tests && PATH=$WORKSPACE/arcadia:$PATH make test')
    }

    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/clickhouse/tools/tests/staging/logs/**/*')
            allowEmpty(true)
        }
        postBuildTask {
            task('.*', 'cd arcadia/cloud/mdb/clickhouse/tools/tests && make clean')
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
