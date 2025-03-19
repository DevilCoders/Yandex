job("graphene-master-storage-test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/graphene-master-storage-test/buildTimeGraph/png' />""")
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

    environmentVariables {
        env('CPM_SOURCE_CACHE', '/home/robot-pgaas-ci/.cache/CPM')
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
                 |if [ ! -d storage ]
                 |then
                 |    exit 0
                 |fi
                 |CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" ./configure --enable-debug --enable-cassert
                 |make PATH="/usr/lib/llvm-14/bin:${PATH}" -j 32
                 |cd storage
                 |CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" cmake -Bbuild -DBUILD_SERVER=OFF -DBUILD_TEST=ON
                 |cd build
                 |make PATH="/usr/lib/llvm-14/bin:${PATH}" check-format
                 |make storage_test -j 32
                 |make test'''.stripMargin('|'))
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
