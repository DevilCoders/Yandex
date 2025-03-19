job("mysync-night-test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/mysync-night-test/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')
    weight(5)

    triggers {
        cron('H 4 * * *')
    }

    wrappers {
        timestamps()
        colorizeOutput('xterm')
    }

    steps{
        shell('/usr/local/bin/arcadia_pass_context.py')
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb/mysync')
        shell('docker ps -qa | xargs -r docker rm -f || true')
        shell('docker network ls | grep bridge | grep net | awk \'{print $1}\' | xargs -r -n1 docker network rm || true')
        shell('cd arcadia/cloud/mdb/mysync/tests && make test')
    }

    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/mysync/tests/logs/*')
            pattern('arcadia/cloud/mdb/mysync/tests/logs/**/*')
            allowEmpty(true)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
