job("dbaas-sync-checks"){
    description("""
<h2>Sync time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/dbaas-sync-checks/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild(false)
    label('dbaas')

    authenticationToken('token')

    triggers {
        cron('H * * * *')
    }
    
    parameters {
        stringParam('trigger_id')
    }

    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
        timestamps()
        colorizeOutput('xterm')
        credentialsBinding {
            string('JUGGLER_OAUTH_TOKEN', 'robot-pgaas-deploy-juggler-token')
        }
    }

    steps {
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb/juggler-config')
        shell('cd arcadia/cloud/mdb/juggler-config && PATH=/opt/ansible-juggler/bin:$PATH ./apply.sh')
    }

    publishers {
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
        mailer('mdb-cc@yandex-team.ru', true, true)
    }
}
