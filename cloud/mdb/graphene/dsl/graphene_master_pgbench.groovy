job("graphene-master-pgbench"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/graphene-master-pgbench/buildTimeGraph/png' />""")
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
                 |cd storage
                 |CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" cmake -Bbuild -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON
                 |cd build
                 |make storage_client -j 32
                 |cd ../..
                 |CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" ./configure --enable-debug --enable-cassert --with-ssl=openssl --with-storage-client --prefix $(pwd)/pg_storage
                 |make PATH="/usr/lib/llvm-14/bin:${PATH}" -j 32
                 |make install
                 |cd storage
                 |CC=/usr/bin/clang-12 CXX=/usr/bin/clang++-12 PATH="/usr/lib/llvm-12/bin:${PATH}" cmake -Bbuild -DBUILD_SERVER=ON -DBUILD_CLIENT=ON
                 |cd build
                 |make storage_client storage_server -j 32
                 |./storage >storage.log 2>&1 &
                 |echo $! > storage.pid
                 |cd ../..
                 |sleep 10
                 |./pg_storage/bin/initdb --no-locale data_test
                 |cd data_test
                 |../pg_storage/bin/postgres -D . >postgresql.log 2>&1 &
                 |echo $! > postgres.pid
                 |sleep 30
                 |../pg_storage/bin/pgbench -s 100 -i postgres
                 |../pg_storage/bin/pgbench -T 300 -c 32 -j 8 postgres'''.stripMargin('|'))
    }

    publishers {
        postBuildTask {
            task('.*', 'if test -f $(pwd)/data_test/postgres.pid; then if kill -0 $(cat $(pwd)/data_test/postgres.pid) >/dev/null 2>&1; then kill $(cat $(pwd)/data_test/postgres.pid); fi; fi')
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
