job("mysync_jepsen_test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/mysync_jepsen_test/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild(true)
    label('dbaas')
    weight(5)
    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        timestamps()
        buildName('mysync jepsen \${BUILD_NUMBER}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
        credentialsBinding {
            string('ARCANUMAUTH', 'arcanum_token')
        }
        colorizeOutput('xterm')
    }

    triggers {
        cron('H 2 * * *')
    }

    steps{
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('docker ps -qa | xargs -r docker rm -f || true')
        shell('docker network ls | grep bridge | grep net | awk \'{print $1}\' | xargs -r -n1 docker network rm || true')
        shell('cd arcadia/cloud/mdb/mysync/tests && make jepsen_test')
    }
    publishers {
        archiveArtifacts {
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
