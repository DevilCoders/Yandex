mysql_secrets: {{salt.yav.get('sec-01eyjnqk690dfvftz4vdxk06gg')|json}}

mysync:
  cluster-id: racktables-stable
  mysql-add-hosts:
    - noc-sas.yndx.net
    - noc-myt.yndx.net
  mysql-masters:
    - noc-sas.yndx.net
    - noc-myt.yndx.net
mysql:
  server-ids:
    myt-mysql: 1
    sas-mysql: 2
    vla-mysql: 3
    vla-mysql2: 4
    vla-mysql1: 5
    man-mysql1: 6
    man-mysql2: 7
    sas-mysql1: 8
    sas-mysql2: 9
    sas-mysql3: 10
    vla-mysql3: 11
    man1-rt1: 3685797296
    # included in pillar/nocdev-staging.sls
    # смотри pillar/nocdev-staging.sls для деталей
    n3: 103
    n4: 104
    n5: 105
    n6: 106

    rt-test: 101 # qyp n1.test.racktables.yandex-team.ru
    vla-rt-staging: 102 # qyp n2.test.racktables.yandex-team.ru

    vla-rt-staging1: 109 # n9.test.racktables.yandex-team.ru
    vla-rt-staging2: 110 # n10.test.racktables.yandex-team.ru

    iva-rt-staging1: 104 # n4.test.racktables.yandex-team.ru
    iva-rt-staging2: 105 # n5.test.racktables.yandex-team.ru

    sas-rt-staging1: 103 # n3.test.racktables.yandex-team.ru
    sas-rt-staging2: 106 # n6.test.racktables.yandex-team.ru
    sas-rt-staging3: 107 # n7.test.racktables.yandex-team.ru
    red-srv01: 108
