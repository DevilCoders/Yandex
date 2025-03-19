replica-set-online:
    cmd.run:
        - name: >
            my-wait-synced --defaults-file=/home/mysql/.my.cnf
            --wait={{ salt['pillar.get']('sync-timeout',1200)|int }}s
            --replica-lag={{ salt['pillar.get']('data:mysql:config:mdb_offline_mode_disable_lag', 30)|int }}s
            &&
            mysql --defaults-file=/home/mysql/.my.cnf --execute 'SET GLOBAL offline_mode = 0'
        - unless: >
            mysql --defaults-file=/home/mysql/.my.cnf --execute 'SELECT @@offline_mode' -N | grep -qw 0
