#!/bin/bash

if [ -z "$2" ]
then
    echo "Usage: $(basename "$0") <suffix> <make target>"
    exit 1
fi

full_name="arcadia_infrastructure_$2"
image_version="$1"

cat > "$full_name".groovy <<EOF
job("$full_name"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/$full_name/buildTimeGraph/png' />""")
    logRotator(7, 30, 7, 30)
    concurrentBuild(true)
    label('dbaas')


    authenticationToken('token')

    parameters {
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
            string('yav_oauth', 'robot-pgaas-ci-yav_oauth')
        }
        buildName('\${ENV,var="ARCANUMID"} \${BUILD_NUMBER}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps{
        shell('flock -s /tmp/arcadia.lock timeout 600 rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia && echo "Arcanum ID: \$ARCANUMID" && if [ "\$ARCANUMID" != "trunk" ]; then retry bash -c "(./ya unshelve -d -a \$ARCANUMID || ./ya unshelve -a \$ARCANUMID)"; fi')
        shell('cd arcadia && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia/cloud/mdb/images/$image_version/tests && LC_ALL=en_US.UTF-8 make lint')
        shell('cd arcadia/cloud/mdb/images/$image_version/tests && LC_ALL=en_US.UTF-8 make test junit-directory=junit BRANCH="pr/\${ARCANUMID}" COMMIT="ci/\${BUILD_NUMBER}"')
    }

    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/images/$image_version/tests/artifacts/*')
            allowEmpty(true)
        }
        postBuildTask {
            task('.*', 'cd arcadia/cloud/mdb/images/$image_version && echo "MDB-12842: Temporary skipping make sweep"')
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
EOF
