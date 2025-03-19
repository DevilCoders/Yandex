Feature: Container restore work correctly

    Background: Prepare deleted container
        Given a deployed DBM
          And empty DB
         When we issue "POST /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                geo: vla
                cpu_cores: 56
                memory: 269157273600
                ssd_space: 7542075424768
                sata_space: 0
                max_io: 1468006400
                net_speed: 1310720000
                heartbeat: 1
                generation: 2
                disks:
                    - id: 11111111-1111-1111-1111-111111111111
                      max_space_limit: 10995116277760
                      has_data: false
            """
         Then we get response with code 200
         When we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
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
                      path: /var/lib/clickhouse
                      dom0_path: /disks
                      space_limit: 10 TB
            """
         Then we get response with code 200
         When we issue "DELETE /api/v2/containers/vla-123.db.yandex.net?save_paths=/,/var/lib/clickhouse" with:
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
                token: ffffffff-ffff-ffff-ffff-ffffffffffff
            """
         Then we get response with code 200
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
              path: /var/lib/clickhouse
            """

    Scenario: Restore with correct params works
         When we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
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
                restore: true
                dom0: vla1-0001.tst.yandex.net
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/clickhouse
                      dom0_path: /disks
                      space_limit: 10 TB
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

    Scenario: Restore with backup delete in progress fails
         When we issue "DELETE /api/v2/volume_backups/vla1-0001.tst.yandex.net/vla-123.db.yandex.net?path=/" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
         When we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
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
                restore: true
                dom0: vla1-0001.tst.yandex.net
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/clickhouse
                      dom0_path: /disks
                      space_limit: 10 TB
            """
         Then we get response with code 400
          And body is:
            """
            error: Backup for volume at / is undergoing deletion process on dom0
            """

    Scenario: Restore with incomplete volumes spec fails
         When we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
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
                restore: true
                dom0: vla1-0001.tst.yandex.net
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
            """
         Then we get response with code 400
          And body is:
            """
            error: Number of restore (1) and backup volumes (2) are different
            """

    Scenario: Restore with not matching path fails
         When we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
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
                restore: true
                dom0: vla1-0001.tst.yandex.net
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /disks
                      space_limit: 10 TB
            """
         Then we get response with code 400
          And body is:
            """
            error: Missing backup for volume at /var/lib/postgresql
            """

    Scenario: Restore with not matching size fails
         When we issue "PUT /api/v2/containers/vla-123.db.yandex.net" with:
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
                restore: true
                dom0: vla1-0001.tst.yandex.net
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-123.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/clickhouse
                      dom0_path: /disks
                      space_limit: 9 TB
            """
         Then we get response with code 400
          And body is:
            """
            error: Volume at /var/lib/clickhouse space_limit has value 9895604649984 but backup has 10995116277760
            """
