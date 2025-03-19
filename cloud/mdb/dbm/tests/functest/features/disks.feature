Feature: Operations with disks work correctly

    Scenario: Creating container with raw disk works
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
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_raw_disks: 1
            free_raw_disks_space: 0
            """
         When we issue "GET /api/v2/volumes/vla-123.db.yandex.net" with:
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
              dom0: vla1-0001.tst.yandex.net
              dom0_path: /data/vla-123.db.yandex.net/rootfs
              backend: native
              space_guarantee: null
              space_limit: 10737418240
              inode_guarantee: null
              inode_limit: null
              read_only: False
              pending_backup: False
            - container: vla-123.db.yandex.net
              path: /var/lib/clickhouse
              dom0: vla1-0001.tst.yandex.net
              dom0_path: /disks/11111111-1111-1111-1111-111111111111/vla-123.db.yandex.net
              backend: native
              space_guarantee: null
              space_limit: 10995116277760
              inode_guarantee: null
              inode_limit: null
              read_only: False
              pending_backup: False
            """

    Scenario: Deploy and heartbeat race condition handled correctly
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
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """
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
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_raw_disks: 1
            free_raw_disks_space: 0
            """

    Scenario: Disks are reclaimed after data removal
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
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """
         When we issue "DELETE /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                save_paths: /,/var/lib/clickhouse
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
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_raw_disks: 1
            free_raw_disks: 1
            """

    Scenario: Raw disk inplace resize works
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
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """
         When we issue "POST /api/v2/volumes/vla-123.db.yandex.net?path=/var/lib/clickhouse" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            body:
                space_limit: 9 TB
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

    Scenario: Raw disk inplace resize with overcommit fails
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
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """
         When we issue "POST /api/v2/volumes/vla-123.db.yandex.net?path=/var/lib/clickhouse" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                space_limit: 11 TB
            """
         Then we get response with code 400
          And body contains:
            """
            error: Unable to reallocate container
            """

    Scenario: Container resize with raw disk works
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
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """
         When we launch container vla-456.db.yandex.net in cluster 222
          And we issue "POST /api/v2/dom0/vla2-0002.tst.yandex.net" with:
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
                    - id: 22222222-2222-2222-2222-222222222222
                      max_space_limit: 10995116277760
                      has_data: false
            """
         When we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                cpu_guarantee: 56
                cpu_limit: 56
            """
         Then we get response with code 200
         When we run query
            """
            UPDATE mdb.transfers SET id = 'ffffffff-ffff-ffff-ffff-ffffffffffff'
            """
         When we issue "GET /api/v2/transfers/ffffffff-ffff-ffff-ffff-ffffffffffff" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
          """
          id: ffffffff-ffff-ffff-ffff-ffffffffffff
          container: vla-123.db.yandex.net
          src_dom0: vla1-0001.tst.yandex.net
          dest_dom0: vla2-0002.tst.yandex.net
          """
         When we issue "POST /api/v2/transfers/ffffffff-ffff-ffff-ffff-ffffffffffff/finish" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
          """
          {}
          """
         When we issue "GET /api/v2/volumes/vla-123.db.yandex.net" with:
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
              dom0: vla2-0002.tst.yandex.net
              dom0_path: /data/vla-123.db.yandex.net/rootfs
              backend: native
              space_guarantee: null
              space_limit: 10737418240
              inode_guarantee: null
              inode_limit: null
              read_only: False
              pending_backup: False
            - container: vla-123.db.yandex.net
              path: /var/lib/clickhouse
              dom0: vla2-0002.tst.yandex.net
              dom0_path: /disks/22222222-2222-2222-2222-222222222222/vla-123.db.yandex.net
              backend: native
              space_guarantee: null
              space_limit: 10995116277760
              inode_guarantee: null
              inode_limit: null
              read_only: False
              pending_backup: False
            """
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_raw_disks: 1
            free_raw_disks_space: 0
            """
         When we issue "GET /api/v2/dom0/vla2-0002.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_raw_disks: 1
            free_raw_disks_space: 0
            """
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
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_raw_disks: 1
            free_raw_disks: 1
            """

    Scenario: Raw disk resize with transfer works
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
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """
         When we issue "POST /api/v2/dom0/vla2-0002.tst.yandex.net" with:
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
                    - id: 22222222-2222-2222-2222-222222222222
                      max_space_limit: 21990232555520
                      has_data: false
            """
         When we issue "POST /api/v2/volumes/vla-123.db.yandex.net?path=/var/lib/clickhouse" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                space_limit: 20 TB
            """
         Then we get response with code 200
         When we run query
            """
            UPDATE mdb.transfers SET id = 'ffffffff-ffff-ffff-ffff-ffffffffffff'
            """
         When we issue "GET /api/v2/transfers/ffffffff-ffff-ffff-ffff-ffffffffffff" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
          """
          id: ffffffff-ffff-ffff-ffff-ffffffffffff
          container: vla-123.db.yandex.net
          src_dom0: vla1-0001.tst.yandex.net
          dest_dom0: vla2-0002.tst.yandex.net
          """
