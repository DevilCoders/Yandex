job("graphene-cover-data-generator"){
    description("""
<h2>Gen time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/graphene-cover-data-generator/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild(false)
    label('dbaas')
    weight(5)

    scm {
        git {
            remote {
                name('gerrit')
                url('ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/postgresql')
                credentials('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
            }
            branch('cover-data-generator')
        }
    }

    triggers {
        scm('H * * * *')
    }

    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        timestamps()
        colorizeOutput('xterm')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps{
        shell('''|CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" ./configure --prefix $(pwd)/pg_bin
                 |make PATH="/usr/lib/llvm-14/bin:${PATH}" -j 32
                 |cd contrib
                 |make PATH="/usr/lib/llvm-14/bin:${PATH}" -j 32
                 |cd ..
                 |make install
                 |cd contrib
                 |make install
                 |cd ..
                 |flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/
                 |cd arcadia
                 |svn up
                 |./ya make --checkout cloud/mdb/graphene/test-util/pg-wal-cover-generator
                 |./ya make --checkout cloud/mdb/graphene/test-util/pg-wal-cover-checker
                 |cd ..
                 |./arcadia/cloud/mdb/graphene/test-util/pg-wal-cover-generator/pg-wal-cover-generator --bin-dir $(pwd)/pg_bin
                 |./arcadia/cloud/mdb/graphene/test-util/pg-wal-cover-checker/pg-wal-cover-checker --bin-dir $(pwd)/pg_bin
                 |tar -zcf test-data.tar.gz data_init data_result'''.stripMargin('|'))
    }

    publishers {
        archiveArtifacts {
            pattern('test-data.tar.gz')
            allowEmpty(false)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
