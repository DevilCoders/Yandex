Feature: Generations works correctly

    Scenario: Launch a container with 2 generation on dom0 with 4 generation
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
                switch: vla1-s1
                cpu_cores: 56
                memory: 269157273600
                ssd_space: 7542075424768
                sata_space: 0
                max_io: 1468006400
                net_speed: 1310720000
                heartbeat: 1
                generation: 4
                disks: []
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
                generation: 2
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
