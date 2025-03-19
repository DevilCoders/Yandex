job("ch-backup-test-infra"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/ch-backup-test-infra/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')

    scm {
        git {
            remote {
                name('origin')
                url('ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/ch-backup.git')
                credentials('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
                refspec('$GERRIT_REFSPEC')
            }
            branches('$GERRIT_PATCHSET_REVISION')
            extensions {
                cloneOptions {
                    timeout(300)
                }
            }
        }
    }

    wrappers {
        preBuildCleanup()
        timestamps()
        buildName('#${BUILD_NUMBER} ${GERRIT_TOPIC} ${GERRIT_REFSPEC} ${CLICKHOUSE_VERSION}')
        colorizeOutput('xterm')
    }

    steps{
        shell('env')
        shell('git log -1')
        shell('CLICKHOUSE_VERSION=${CLICKHOUSE_VERSION} timeout 14400 make integration_test')
    }

    publishers {
        archiveArtifacts {
            pattern('staging/logs/**/*')
            allowEmpty(true)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
