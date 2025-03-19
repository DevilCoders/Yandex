data:
  mysql:
    users:
      repl:
        dbs:
          '*': [REPLICATION SLAVE]
        password: {{ salt.yav.get('ver-01eaa23q6862h73qxzfs48j03m[password]') }}
      admin:
        dbs:
          '*': [ALL PRIVILEGES]
        password: {{ salt.yav.get('ver-01eaa24vg8cadv2z7n3yw5s36x[password]') }}
      monitor:
        dbs:
          '*': [REPLICATION CLIENT]
          mysql: [SELECT]
        password: {{ salt.yav.get('ver-01eaa25qw0mwwfw02zjpprm1zz[password]') }}
