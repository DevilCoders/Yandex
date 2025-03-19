job("mdb-telegraf-test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/mdb-telegraf-test/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')

    scm {
        git {
            remote {
                name('origin')
                url('ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/mdb-telegraf.git')
                credentials('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
                refspec('$GERRIT_REFSPEC')
            }
            branches('*/master')
            extensions {
                cloneOptions {
                    timeout(300)
                }
            }
        }
    }

    triggers {
        gerrit {
            events{
                patchsetCreated()
                draftPublished()
            }
            project('mdb/mdb-telegraf', 'reg_exp:.*')
        }
    }

    wrappers {
        timestamps()
        buildName('${ENV,var="GERRIT_TOPIC"} ${BUILD_NUMBER}')
        colorizeOutput('xterm')
    }

    steps{
        shell('timeout 300 git checkout ${GERRIT_PATCHSET_REVISION} -b current')
        shell('timeout 300 git rebase origin/${GERRIT_BRANCH} current')
        shell('git log -1')
        shell('timeout 14400 make test')
    }

    publishers {
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
        buildDescription('', '${GERRIT_PROJECT}')
    }
}
