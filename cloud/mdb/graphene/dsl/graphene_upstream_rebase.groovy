job("graphene-upstream-rebase"){
    description("""
<h2>Gen time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/graphene-upstream-rebase/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild(false)
    label('dbaas')
    weight(5)

    scm {
        git {
            branch('master')
            remote {
                url('https://github.com/postgres/postgres.git')
            }
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
        shell('''|CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" ./configure --enable-tap-tests --enable-cassert --enable-debug
                 |make PATH="/usr/lib/llvm-14/bin:${PATH}" -j32
                 |make check
                 |make check-world
                 |make maintainer-clean
                 |
                 |cat <<EOH >> .git/config
                 |[remote "gerrit"]
                 |    url = ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/postgresql
                 |    fetch = +refs/heads/*:refs/remotes/gerrit/*
                 |EOH
                 |
                 |git fetch gerrit
                 |
                 |for branch in master cover-data-generator slowdowns
                 |do
                 |    git checkout gerrit/${branch} -b ${branch}
                 |    git rebase origin/master
                 |    CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14 PATH="/usr/lib/llvm-14/bin:${PATH}" ./configure --enable-tap-tests --enable-cassert --enable-debug
                 |    make PATH="/usr/lib/llvm-14/bin:${PATH}" -j32
                 |    make check
                 |    make check-world
                 |    make maintainer-clean
                 |done
                 |
                 |for branch in master cover-data-generator slowdowns
                 |do
                 |    git checkout ${branch}
                 |    git push gerrit ${branch} --force
                 |done'''.stripMargin('|'))
    }

    publishers {
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
        mailer('mdb-cc@yandex-team.ru', true, false)
    }
}
