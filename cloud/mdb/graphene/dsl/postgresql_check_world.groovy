job("postgresql-check-world"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/postgresql-check-world/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')
    weight(5)

    parameters {
        stringParam('GERRIT_REFSPEC', 'master')
        stringParam('GERRIT_BRANCH', 'master')
    }

    triggers {
        gerrit {
            events{
                patchsetCreated()
                draftPublished()
            }
            project('mdb/postgresql', 'reg_exp:.*')
        }
    }

    wrappers {
        timestamps()
        buildName('${ENV,var="GERRIT_TOPIC"} ${BUILD_NUMBER}')
        colorizeOutput('xterm')
    }

    steps{
        shell('''|git init
                 |timeout 1800 git fetch --tags --progress -- "ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/postgresql" '+refs/heads/*:refs/remotes/origin/*'
                 |timeout 1800 git pull "ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/postgresql" ${GERRIT_REFSPEC}
                 |git checkout ${GERRIT_PATCHSET_REVISION} -b current
                 |git rebase origin/${GERRIT_BRANCH} current
                 |git log -1
                 |CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" CFLAGS="${CFLAGS}" ./configure --enable-tap-tests --enable-debug --enable-cassert
                 |timeout 1200 make PATH="/usr/lib/llvm-14/bin:${PATH}" -j32
                 |timeout 3600 make check
                 |timeout 3600 make check-world'''.stripMargin('|'))
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
