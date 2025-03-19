job("graphene-master-insert-benchmark"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/graphene-master-insert-benchmark/buildTimeGraph/png' />""")
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
        copyArtifacts('graphene-cover-data-generator') {
            includePatterns('test-data.tar.gz')
            targetDirectory('.')
            buildSelector {
                latestSuccessful(true)
            }
        }
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
                 |CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" ./configure
                 |make PATH="/usr/lib/llvm-14/bin:${PATH}" -j 32
                 |cd storage
                 |CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
                 |cd build
                 |make storage_server -j 32
                 |./storage >storage.log 2>&1 &
                 |echo $! > storage.pid
                 |cd ../..
                 |tar -zxf test-data.tar.gz
                 |./storage/build/wal_import --benchmark --wal_path data_result/pg_wal/000000010000000000000001 --rmgr_ids all'''.stripMargin('|'))
    }

    publishers {
        postBuildTask {
            task('.*', 'if test -f $(pwd)/storage/build/storage.pid; then if kill -0 $(cat $(pwd)/storage/build/storage.pid) >/dev/null 2>&1; then kill -9 $(cat $(pwd)/storage/build/storage.pid); fi; fi')
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
        buildDescription('', '${GERRIT_PROJECT}')
    }
}
