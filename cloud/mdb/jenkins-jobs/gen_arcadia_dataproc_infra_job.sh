#!/bin/bash

if [ -z "$2" ]
then
    echo "Usage: $(basename "$0") <make start target> <make test target>"
    exit 1
fi

full_name="arcadia_infrastructure_$2"

cat > "$full_name".groovy <<EOF
job("$full_name"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/$full_name/buildTimeGraph/png' />""")
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
        credentialsBinding {
            string('ARCANUMAUTH', 'arcanum_token')
            string('YAV_OAUTH', 'robot-pgaas-ci-yav_oauth')
        }
        buildName('\${ENV,var="ARCANUMID"} \${BUILD_NUMBER}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps{
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia && echo "Arcanum ID: \$ARCANUMID" && if [ "\$ARCANUMID" != "trunk" ]; then retry bash -c "(./ya unshelve -d -a \$ARCANUMID || ./ya unshelve -a \$ARCANUMID)"; fi')
        shell('cd arcadia && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia/cloud/mdb/dataproc-infra-tests && LC_ALL=en_US.UTF-8 make yandex')
        shell('export PATH=\$PATH:\$(pwd)/arcadia/ && cd arcadia/cloud/mdb/dataproc-infra-tests && LC_ALL=en_US.UTF-8 make $1')
        shell('export PATH=\$PATH:\$(pwd)/arcadia/ && cd arcadia/cloud/mdb/dataproc-infra-tests && LC_ALL=en_US.UTF-8 make $2')
    }

    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/jobresults/**/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/logs/**/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/meta/**/*')
            pattern('arcadia/cloud/mdb/dataproc-infra-tests/staging/redis/*')
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
EOF
