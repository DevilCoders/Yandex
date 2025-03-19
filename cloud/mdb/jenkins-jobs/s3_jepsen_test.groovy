job("s3_jepsen_test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/s3_jepsen_test/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild(true)
    label('dbaas')
    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        timestamps()
        buildName('s3 jepsen \${BUILD_NUMBER}')
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
        shell('cd arcadia/cloud/mdb/pg/pgproxy/s3db && make jepsen')
    }
    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/pg/pgproxy/s3db/logs/**/*')
            allowEmpty(true)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
