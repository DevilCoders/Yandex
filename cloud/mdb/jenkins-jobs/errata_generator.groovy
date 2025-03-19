job("errata-generator"){
    description("""
<h2>Generation time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/errata-generator/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild(false)
    label('dbaas')

    triggers {
        cron('H H/4 * * *')
    }

    wrappers {
        timestamps()
        colorizeOutput('xterm')
    }

    steps{
        shell('''"svn co svn+ssh://robot-pgaas-ci@arcadia-ro.yandex.ru/arc/trunk/arcadia/cloud/mdb/tools/errata-generator .
                 "s3cmd -c /etc/s3cmd.cfg get s3://errata/errata.json.gz --force
                 "mv -f errata.json.gz old-errata.json.gz
                 "make
                 "OLD_MD5=$(zcat old-errata.json.gz | md5sum | awk '{print $1}')
                 "NEW_MD5=$(zcat errata.json.gz | md5sum | awk '{print $1}')
                 "if [ $OLD_MD5 != $NEW_MD5 ]
                 "then
                 "   s3cmd -c /etc/s3cmd.cfg put errata.json.gz s3://errata/errata.json.gz
                 "fi
                 "'''.stripMargin('"'))
    }

    publishers {
        mailer('mdb-admin-cc@yandex-team.ru', true, true)
    }
}
