job("dbaas-test-checks"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/dbaas-test-checks/buildTimeGraph/png' />""")
    logRotator(7, 30)
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
        buildName('${ENV,var="ARCANUMID"} ${BUILD_NUMBER}')
        credentialsBinding {
            string('JUGGLER_OAUTH_TOKEN', 'robot-pgaas-deploy-juggler-token')
        }
    }

    steps{
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb/juggler-config')
        shell('cd arcadia && echo "Arcanum ID: $ARCANUMID" && [[ -z $ARCANUMID ]] || retry bash -c "(./ya unshelve -d -a $ARCANUMID || ./ya unshelve -a $ARCANUMID)"')
        shell('cd arcadia && retry timeout 600 ./ya make -j0 --checkout cloud/mdb/juggler-config')
        shell('cd arcadia/cloud/mdb/juggler-config && PATH=/opt/ansible-juggler/bin:$PATH ./test.sh')
    }

    publishers {
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
