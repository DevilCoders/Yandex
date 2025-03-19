Feature: Containers handlers work correctly

    Scenario: Launch of single container works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                cluster: ffffffff-ffff-ffff-ffff-ffffffffffff
                geo: vla
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
                cpu_guarantee: 1
                cpu_limit: 1
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /data/vla-123.db.yandex.net/data
                      space_limit: 100 GB
            """
         Then we get response with code 200
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """

         When we issue "GET /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            fqdn: vla-123.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
            cpu_guarantee: 1.0
            cpu_limit: 1.0
            generation: 1
            memory_guarantee: 4294967296
            memory_limit: 4294967296
            hugetlb_limit: null
            io_limit: 104857600
            net_guarantee: 16777216
            net_limit: 16777216
            cluster_name: ffffffff-ffff-ffff-ffff-ffffffffffff
            extra_properties:
              project_id: '0:1589'
            project_id: null
            managing_project_id: null
            """

    Scenario: Launch of container in project without resources
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: noresources
                cluster: ffffffff-ffff-ffff-ffff-ffffffffffff
                fqdn: vla-123.db.yandex.net
                geo: vla
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
                cpu_guarantee: 1
                cpu_limit: 1
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
            """
         Then we get response with code 400
          And body is:
            """
            error: Unable to allocate container
            """

    Scenario: Modify of single container works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
          And we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                cpu_guarantee: '2'
                cpu_limit: '2'
                memory_guarantee: 8 GB
                memory_limit: 8 GB
                net_guarantee: 32 MB
                net_limit: 32 MB
                io_limit: 200 MB
                secrets:
                  /tmp/test:
                    mode: '0600'
                    content: test
            """
         Then we get response with code 200
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """

         When we issue "GET /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            fqdn: vla-123.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
            cpu_guarantee: 2.0
            cpu_limit: 2.0
            generation: 1
            memory_guarantee: 8589934592
            memory_limit: 8589934592
            hugetlb_limit: null
            io_limit: 209715200
            net_guarantee: 33554432
            net_limit: 33554432
            cluster_name: ffffffff-ffff-ffff-ffff-ffffffffffff
            extra_properties:
              project_id: '0:1589'
            project_id: null
            managing_project_id: null
            """

    Scenario: Delete of single container works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
          And we issue "DELETE /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                save_paths: /,/var/lib/clickhouse
            """
         Then we get response with code 400
          And body contains:
            """
            error: Unable to save paths
            """

         When we issue "DELETE /api/v2/containers/vla-123.db.yandex.net?save_paths=/,/var/lib/postgresql" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            """
         Then we get response with code 200
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """

         When we issue "GET /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            fqdn: vla-123.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
            cpu_guarantee: 1.0
            cpu_limit: 1.0
            generation: 1
            memory_guarantee: 4294967296
            memory_limit: 4294967296
            hugetlb_limit: null
            io_limit: 104857600
            net_guarantee: 16777216
            net_limit: 16777216
            cluster_name: ffffffff-ffff-ffff-ffff-ffffffffffff
            extra_properties:
              project_id: '0:1589'
            project_id: null
            managing_project_id: null
            """

         When we issue "GET /api/v2/pillar/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            data:
                porto:
                    vla-123.db.yandex.net:
                        container_options:
                            pending_delete: True
                        volumes:
                          - path: /
                            pending_backup: True
                          - path: /var/lib/postgresql
                            pending_backup: True
                use_vlan688: True
            """

         When we run query
            """
            UPDATE mdb.containers SET delete_token = 'ffffffff-ffff-ffff-ffff-ffffffffffff'
            """
          And we issue "POST /api/v2/dom0/delete-report/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Accept: application/json
                Content-Type: application/json
            body:
                token: 00000000-0000-0000-0000-000000000000
            """
         Then we get response with code 400
          And body is:
            """
            error: Invalid token
            """

         When we issue "POST /api/v2/dom0/delete-report/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Accept: application/json
                Content-Type: application/json
            body:
                token: ffffffff-ffff-ffff-ffff-ffffffffffff
            """
         Then we get response with code 200

         When we issue "GET /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 404

         When we issue "GET /api/v2/volume_backups/?query=container=vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            - container: vla-123.db.yandex.net
              path: /
            - container: vla-123.db.yandex.net
              path: /var/lib/postgresql
            """

    Scenario: Launch containers from one cluster
              DBM should launch them on same dom0 if no other options available
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-234.db.yandex.net in cluster ffffffff-ffff-ffff-ffff-ffffffffffff
          And we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                cluster: ffffffff-ffff-ffff-ffff-ffffffffffff
                geo: vla
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
                cpu_guarantee: 1
                cpu_limit: 1
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /data/vla-123.db.yandex.net/data
                      space_limit: 100 GB
            """
         Then we get response with code 200
          But body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """

    Scenario: Launch several containers from different clusters
              Should launch new container on freest dom0

        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we issue heartbeat for vla2-0002.tst.yandex.net dom0

          And we launch container vla-123.db.yandex.net in cluster 111
         Then container info for vla-123.db.yandex.net contains:
            """
            fqdn: vla-123.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            """

         When we launch container vla-456.db.yandex.net in cluster 222
         Then container info for vla-456.db.yandex.net contains:
            """
            fqdn: vla-456.db.yandex.net
            dom0: vla2-0002.tst.yandex.net
            """

         When we launch container vla-789.db.yandex.net in cluster 333
         Then container info for vla-789.db.yandex.net contains:
            """
            fqdn: vla-789.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            """

         When we issue "GET /api/v2/containers/?query=fqdn=vla" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            - fqdn: vla-123.db.yandex.net
              dom0: vla1-0001.tst.yandex.net
            - fqdn: vla-456.db.yandex.net
              dom0: vla2-0002.tst.yandex.net
            - fqdn: vla-789.db.yandex.net
              dom0: vla1-0001.tst.yandex.net
            """

    Scenario: Launch containers with fqdn conflict fails
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net in cluster 111
          And we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                cluster: '222'
                geo: vla
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
                cpu_guarantee: 1
                cpu_limit: 1
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /data/vla-123.db.yandex.net/data
                      space_limit: 100 GB
            """
         Then we get response with code 409
          And body is:
            """
            error: Container already exists
            """

    Scenario: Query handle works correctly
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net in cluster 111
         When we issue "GET /api/v2/containers/?query=cpu_guarantee=a" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            []
            """
         When we issue "GET /api/v2/containers/?query=cpu_guarantee=1" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            - fqdn: vla-123.db.yandex.net
            """
         When we issue "GET /api/v2/containers/?query=memory_limit=a" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            []
            """
         When we issue "GET /api/v2/containers/?query=memory_limit=4294967296" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            - fqdn: vla-123.db.yandex.net
            """

    @concurrency
    Scenario: Launch concurrency
        Given a deployed DBM
        And empty DB
        When we issue heartbeat for vla1-0001.tst.yandex.net dom0
        And we lock vla
        And we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 15
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                cluster: ffffffff-ffff-ffff-ffff-ffffffffffff
                geo: vla
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
                cpu_guarantee: 1
                cpu_limit: 1
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /data/vla-123.db.yandex.net/data
                      space_limit: 100 GB
            """
        Then we get response with code 503
        And body contains error matches:
            """
            Another allocation in progress. Geo vla lock not available. Locked by: dbm@dbm pid:[0-9]+ session.*in 'idle in transaction' state
            """
