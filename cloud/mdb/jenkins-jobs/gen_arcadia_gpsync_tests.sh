#!/bin/bash

for test_type in behave behave_night jepsen; do

    triggers="
        triggers {
            cron('H 2 * * *')
        }"
    archive_junit="
        archiveJunit('arcadia/cloud/mdb/gpsync/junit_report/*.xml') {
            allowEmptyResults()
        }"
    mailer="mailer('mdb-cc@yandex-team.ru', true, true)"
    
    case $test_type in
        behave)
            triggers=""
            test_cmd="timeout 14400 make -L TIMEOUT=600 check"
            mailer=""
            ;;
        behave_night)
            test_cmd="timeout 14400 make -L TIMEOUT=600 check_unstoppable"
            mailer=""
            ;;
        jepsen)
            archive_junit=""
            test_cmd="timeout 14400 make -L jepsen"
            ;;
    esac
    
    full_name="arcadia_gpsync_${test_type}_test"
    cat > "$full_name".groovy <<EOF
job("$full_name"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/$full_name/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild(true)
    label('dbaas')
    weight(5)

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
        buildName('#\${BUILD_NUMBER} - Arcanum: \${ENV,var="ARCANUMID"}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
        credentialsBinding {
            string('ARCANUMAUTH', 'arcanum_token')
        }
        colorizeOutput('xterm')
    }
    $triggers

    steps{
        shell('''
            TAGS_ARE_MANDATORY=false /usr/local/bin/arcadia_pass_context.py
            if grep "^SUMMARY=.*\\\\[noci\\\\]" infra.properties; then
                echo "[noci] tag found, not running tests"
            else
                flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/
                cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 -t --checkout cloud/mdb/gpsync
                echo "Arcanum ID: \$ARCANUMID" && if [ "\$ARCANUMID" != "trunk" ]; then retry bash -c "(./ya pr checkout -D \$ARCANUMID || ./ya pr checkout \$ARCANUMID)"; fi
                retry timeout 600 ./ya make -j0 -t --checkout cloud/mdb/gpsync
                docker ps -qa | xargs -r docker rm -f || true
                docker network ls | grep bridge | grep net | awk '{print \$1}' | xargs -r -n1 docker network rm || true
                export PATH=\$PATH:\$(pwd) && cd cloud/mdb/gpsync && timeout 14400 make -L TIMEOUT=600 check
            fi
        ''')
    }
    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/gpsync/logs/**/*')
            allowEmpty(true)
        }
        $archive_junit
        $mailer
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            cleanWhenFailure(false)
            cleanWhenUnstable(false)
            cleanWhenAborted(false)
            setFailBuild(false)
        }
    }
}
EOF
done
