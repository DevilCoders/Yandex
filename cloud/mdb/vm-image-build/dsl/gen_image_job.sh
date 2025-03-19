#!/bin/bash

if [ -z "$2" ]
then
    echo "Usage: $(basename "$0") <suffix> <make target>"
    exit 1
fi

# script names may only contain letters, digits and underscores, but may not start with a digit
full_name=`sed 's/-/_/g' <<< dbaas_image_$1`

cat > "$full_name".groovy <<EOF
job("$full_name"){
    description("""
<h2>Build time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/$full_name/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    throttleConcurrentBuilds {
        maxPerNode(1)
        categories(['image'])
    }
    label('dbaas')

    triggers {
        cron('H 4 * * *')
    }

    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        timestamps()
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
        colorizeOutput('xterm')
    }

    steps{
        copyArtifacts('image-base-bionic') {
            includePatterns('base-bionic*')
            targetDirectory('.')
            buildSelector {
                latestSuccessful(true)
            }
        }
        shell('''|flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/
                 |cd arcadia
                 |svn cleanup
                 |svn up
                 |sudo modprobe nbd max_part=16
                 |chmod 600 ../base-bionic
                 |mv ../base-bionic* cloud/mdb/vm-image-build/
                 |cd cloud/mdb/vm-image-build
                 |timeout 14400 make $2'''.stripMargin('|'))
    }

    publishers {
        archiveArtifacts {
            pattern('arcadia/cloud/mdb/vm-image-build/*.log')
            allowEmpty(true)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
        mailer('mdb-cc@yandex-team.ru', true, true)
    }
}
EOF
