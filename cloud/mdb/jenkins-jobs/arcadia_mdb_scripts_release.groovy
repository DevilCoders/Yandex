job("arcadia-mdb-scripts-release"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/arcadia-mdb-scripts-release/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')

    authenticationToken('token')

    triggers {
        cron('H 5 * * 3')
    }

    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        timestamps()
        colorizeOutput('xterm')
        credentialsBinding {
            string('S3_SECRET_KEY', 'mdb-scripts-s3-secret-key')
        }
        buildName('${ENV,var="VERSION"}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps {
        shell('env')
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb/cli/')
        shell('cd arcadia/cloud/mdb/cli/ && ./release.sh "jsOAyjqeuwkkuQn6yDv4" "$S3_SECRET_KEY"')
        shell('echo "VERSION=$(cat arcadia/cloud/mdb/cli/build/current_version)" > version.properties')
        environmentVariables {
            propertiesFile('version.properties')
        }
    }

    publishers {
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
