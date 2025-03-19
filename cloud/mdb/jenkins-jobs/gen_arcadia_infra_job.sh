#!/bin/bash

if [ -z "$2" ]
then
    echo "Usage: $(basename "$0") <suffix> <make target>"
    exit 1
fi

full_name="arcadia_infrastructure_$1"

cat > "$full_name".groovy <<EOF
job("$full_name"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/$full_name/buildTimeGraph/png' />""")
    logRotator(30, 200, 30, 200)
    concurrentBuild(true)
    label('dbaas')

    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        timestamps()
        buildName('\${ENV,var="ARCANUMID"} \${BUILD_NUMBER}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
        credentialsBinding {
            string('ARCANUMAUTH', 'arcanum_token')
        }
        colorizeOutput('xterm')
    }

    steps{
        shell('flock -s /tmp/arcadia.lock timeout 600 rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('cd arcadia && echo "Arcanum ID: \$ARCANUMID" && if [ "\$ARCANUMID" != "trunk" ]; then retry bash -c "(./ya unshelve -d -a \$ARCANUMID || ./ya unshelve -a \$ARCANUMID)"; fi')
        shell('cd arcadia && retry timeout 600 ./ya make -j0 --checkout cloud/mdb')
        shell('mkdir -p arcadia/cloud/mdb/dbaas_infra_tests/staging/code')
        shell('cd arcadia/cloud/mdb/dbaas_infra_tests && mkdir -p staging/code/go-mdb && for i in \$(find .. -maxdepth 1 | grep / | grep -v "../dbaas_infra_tests"); do cp -r \$i staging/code/go-mdb; done && cd staging/code/go-mdb && git init')
        shell('cd arcadia/cloud/mdb/dbaas_infra_tests && mkdir -p staging/code/idm-service && cp -r ../idm_service/* staging/code/idm-service && cd staging/code/idm-service && git init')
        shell('cd arcadia/cloud/mdb/dbaas_infra_tests && mkdir -p staging/code/metadb && cp -r ../dbaas_metadb/* staging/code/metadb && cd staging/code/metadb && git init')
        shell('cd arcadia/cloud/mdb/dbaas_infra_tests && mkdir -p staging/code/internal-api && cp -r ../dbaas-internal-api-image/* staging/code/internal-api && cd staging/code/internal-api && git init')
        shell('cd arcadia/cloud/mdb/dbaas_infra_tests && mkdir -p staging/code/dbaas-worker && cp -r ../dbaas_worker/* staging/code/dbaas-worker && cd staging/code/dbaas-worker && git init')
        shell('cd arcadia/cloud/mdb/dbaas_infra_tests && mkdir -p staging/code/salt/srv && cp -r ../salt/* staging/code/salt/srv && cd staging/code/salt/srv && git init')
        shell('cd arcadia/cloud/mdb/dbaas_infra_tests && mkdir -p staging/code/salt-master/srv && cp -r ../salt/* staging/code/salt-master/srv && cd staging/code/salt-master/srv && git init')
        shell('cd arcadia/cloud/mdb/dbaas_infra_tests && mkdir -p staging/code/salt-master/srv/salt/components/pg-code && cp -r ../pg/* staging/code/salt-master/srv/salt/components/pg-code && cd staging/code/salt-master/srv/salt/components/pg-code && git init')
        shell('cd arcadia/cloud/mdb/dbaas_infra_tests && timeout 14400 make $2 junit-directory=junit')
    }

    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/dbaas_infra_tests/staging/logs/**/*')
            pattern('arcadia/cloud/mdb/dbaas_infra_tests/junit/*')
            allowEmpty(true)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
EOF
