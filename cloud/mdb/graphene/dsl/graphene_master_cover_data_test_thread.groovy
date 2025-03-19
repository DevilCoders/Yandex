job("graphene-master-cover-data-test-thread"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/graphene-master-cover-data-test-thread/buildTimeGraph/png' />""")
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
        shell('''`git init
                 `timeout 1800 git fetch --tags --progress -- "ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/postgresql" '+refs/heads/*:refs/remotes/origin/*'
                 `timeout 1800 git pull "ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/postgresql" ${GERRIT_REFSPEC}
                 `git checkout ${GERRIT_PATCHSET_REVISION} -b current
                 `git rebase origin/${GERRIT_BRANCH} current
                 `git log -1
                 `if [ ! -d storage ]
                 `then
                 `    exit 0
                 `fi
                 `CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 CFLAGS="-fno-omit-frame-pointer" PATH="/usr/lib/llvm-14/bin:${PATH}" ./configure --enable-debug --enable-cassert --prefix $(pwd)/pg_bin
                 `make PATH="/usr/lib/llvm-14/bin:${PATH}" -j 32
                 `make install
                 `cd storage
                 `CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" cmake -Bbuild -DUSE_SANITIZER='Thread'
                 `cd build
                 `make storage_server -j 32
                 `cd ../..
                 `flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/
                 `cd arcadia
                 `svn cleanup
                 `svn up
                 `if echo ${GERRIT_TOPIC} | grep -q arcanumid
                 `then
                 `    next_is_id=0
                 `    for i in ${GERRIT_TOPIC//\\-/\\ }
                 `    do
                 `        if [ "$i" = "arcanumid" ]
                 `        then
                 `            next_is_id=1
                 `        elif [ "$next_is_id" = "1" ]
                 `        then
                 `            retry bash -c "(./ya unshelve -d -a $i || ./ya unshelve -a $i)"
                 `            break
                 `        fi
                 `     done
                 `fi
                 `./ya make --checkout cloud/mdb/graphene/test-util/pg-wal-cover-comparator
                 `cd ..
                 `tar -zxf test-data.tar.gz
                 `./arcadia/cloud/mdb/graphene/test-util/pg-wal-cover-comparator/pg-wal-cover-comparator --storage-bin-dir $(pwd)/storage/build --pg-bin-dir $(pwd)/pg_bin --pg-data-dir $(pwd)/data_result'''.stripMargin('`'))
    }

    publishers {
        archiveArtifacts {
            pattern('storage.log')
            allowEmpty(true)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
        buildDescription('', '${GERRIT_PROJECT}')
    }
}
