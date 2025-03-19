job("arcadia-rdsync-test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/arcadia-rdsync-test/buildTimeGraph/png' />""")
    logRotator(7, 30)
    label('dbaas')
    weight(5)
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
        }
        buildName('${ENV,var="ARCANUMID"} ${BUILD_NUMBER}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps{
        shell('''
            TAGS_ARE_MANDATORY=false /usr/local/bin/arcadia_pass_context.py
            if grep "^SUMMARY=.*\\[noci\\]" infra.properties; then
                echo "[noci] tag found, not running tests"
            else
                flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/
                cd arcadia
                retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb
                echo "Arcanum ID: $ARCANUMID" && [[ -z $ARCANUMID ]] || retry bash -c "(./ya unshelve -d -a $ARCANUMID || ./ya unshelve -a $ARCANUMID)"
                retry timeout 600 ./ya make -j0 --checkout cloud/mdb/rdsync
                docker ps -qa | xargs -r docker rm -f || true
                docker network ls | grep bridge | grep net | awk \'{print $1}\' | xargs -r -n1 docker network rm || true
                cd cloud/mdb/rdsync/tests && make test
            fi
        ''')
    }

    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/rdsync/tests/logs/*')
            pattern('arcadia/cloud/mdb/rdsync/tests/logs/**/*')
            allowEmpty(true)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
