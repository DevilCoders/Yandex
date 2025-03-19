{% set control_plane_folder = 'aoeme1ci0qvbsjia4ks7' %}
{% set dataproc_infratest_folder = 'aoeb0d5hocqev4i6rmmf' %}
data:
    images:
        - name: 'postgresql'
          no_check: true
        - name: 'postgresql-10'
          no_check: true
        - name: 'postgresql-10-1c'
          no_check: true
        - name: 'postgresql-11'
          no_check: true
        - name: 'postgresql-11-1c'
          no_check: true
        - name: 'postgresql-12'
          no_check: true
        - name: 'postgresql-12-1c'
          no_check: true
        - name: 'postgresql-13'
          no_check: true
        - name: 'postgresql-13-1c'
          no_check: true
        - name: 'postgresql-14'
          no_check: true
        - name: 'postgresql-14-1c'
          no_check: true
        - name: 'clickhouse'
          no_check: true
        - name: 'mongodb'
          no_check: true
        - name: 'zookeeper'
          no_check: true
        - name: 'zookeeper'
          alias: zookeeper-infratest
          no_check: true
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'redis'
          no_check: true
        - name: 'redis-50'
          no_check: true
        - name: 'redis-60'
          no_check: true
        - name: 'redis-62'
          no_check: true
        - name: 'redis-62'
          alias: redis-62-infratest
          no_check: true
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'redis-70'
          no_check: true
        - name: 'redis-70'
          alias: redis-70-infratest
          no_check: true
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'mysql'
          no_check: true
        - name: 'elasticsearch'
          no_check: true
        - name: 'elasticsearch-710'
          no_check: true
        - name: 'elasticsearch-711'
          no_check: true
        - name: 'elasticsearch-712'
          no_check: true
        - name: 'elasticsearch-713'
          no_check: true
        - name: 'elasticsearch-714'
          no_check: true
        - name: 'elasticsearch-715'
          no_check: true
        - name: 'elasticsearch-716'
          no_check: true
        - name: 'elasticsearch-717'
          no_check: true
        - name: 'sqlserver-2016sp2dev'
          no_check: true
          os: windows
        - name: 'sqlserver-2016sp2std'
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbstdmssql']
        - name: 'sqlserver-2016sp2ent'
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbentmssql']
        - name: 'sqlserver-2017dev'
          no_check: true
          os: windows
        - name: 'sqlserver-2017std'
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbstdmssql']
        - name: 'sqlserver-2017ent'
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbentmssql']
        - name: 'sqlserver-2019dev'
          no_check: true
          os: windows
        - name: 'sqlserver-2019std'
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbstdmssql']
        - name: 'sqlserver-2019ent'
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbentmssql']
        - name: 'windows-witness'
          no_check: true
          os: windows
          product_ids: ['dqnproducmdbwinwtndc']
        - name: 'windows-witness'
          alias: 'windows-witness-infratest'
          no_check: true
          os: windows
          product_ids: ['dqnproducmdbwinwtndc']
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'sqlserver-2016sp2std'
          alias: sqlserver-2016sp2std-infratest
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbstdmssql']
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'sqlserver-2016sp2ent'
          alias: sqlserver-2016sp2ent-infratest
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbentmssql']
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'sqlserver-2017std'
          alias: 'sqlserver-2017std-infratest'
          no_check: true
          os: windows
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'sqlserver-2017ent'
          alias: 'sqlserver-2017ent-infratest'
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbentmssql']
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'sqlserver-2019std'
          alias: 'sqlserver-2019std-infratest'
          no_check: true
          os: windows
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'sqlserver-2019ent'
          alias: 'sqlserver-2019ent-infratest'
          no_check: true
          os: windows
          product_ids: ['dqnversiomdbentmssql']
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'kafka'
          no_check: true
        - name: 'kafka'
          alias: kafka-infratest
          no_check: true
          folder_id: {{ dataproc_infratest_folder }}
        - name: 'greenplum'
          no_check: true
        - name: 'common-1if'
          no_check: true
          folder_id: {{ control_plane_folder }}
        - name: 'common-2if'
          no_check: true
          folder_id: {{ control_plane_folder }}
