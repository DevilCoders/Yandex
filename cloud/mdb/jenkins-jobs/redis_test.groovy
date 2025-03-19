job("redis-test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/redis-test/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')
    weight(5)

    parameters {
        stringParam('GERRIT_REFSPEC', '7.0-mdb')
        stringParam('GERRIT_BRANCH', '7.0-mdb')
    }

    triggers {
        gerrit {
            events{
                patchsetCreated()
                draftPublished()
            }
            project('mdb/redis', 'reg_exp:.*')
        }
    }

    wrappers {
        timestamps()
        buildName('${ENV,var="GERRIT_TOPIC"} ${ENV,var="GERRIT_BRANCH"} ${BUILD_NUMBER}')
        colorizeOutput('xterm')
    }

    steps{
        shell('''|git init
                 |timeout 1800 git fetch --tags --progress -- "ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/redis" '+refs/heads/*:refs/remotes/origin/*'
                 |timeout 1800 git pull "ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/redis" ${GERRIT_REFSPEC}
                 |git checkout ${GERRIT_PATCHSET_REVISION} -b current
                 |git rebase origin/${GERRIT_BRANCH} current
                 |git log -1
                 |if [ -f utils/gen-test-certs.sh ]
                 |then
                 |    CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" make BUILD_TLS=yes -j32
                 |    ./utils/gen-test-certs.sh
                 |    if echo "$GERRIT_BRANCH" | grep -q ^7
                 |    then
                 |        ./runtest --large-memory --no-latency
                 |    fi
                 |    ./runtest --tls --no-latency
                 |    if [ -f src/senticache.c ]
                 |    then
                 |        ./runtest-senticache --tls
                 |    fi
                 |    ./runtest-sentinel --tls
                 |    ./runtest-cluster --tls
                 |else
                 |    CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" make -j32
                 |    ./runtest
                 |    ./runtest-sentinel
                 |    ./runtest-cluster
                 |fi
                 |'''.stripMargin('|'))
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
