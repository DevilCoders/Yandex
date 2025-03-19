Feature: project_id and managing_project_id work correctly

    Scenario: Launch of single container with standalone project_id and managing_project_id properties works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we issue "PUT /api/v2/containers/vla-1589.db.yandex.net" with:
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
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
                cpu_guarantee: 1
                cpu_limit: 1
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                project_id: '0:1111'
                managing_project_id: '0:aaaa'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-1589.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /data/vla-1589.db.yandex.net/data
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

         When we issue "GET /api/v2/containers/vla-1589.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            fqdn: vla-1589.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
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
            extra_properties: null
            project_id: '0:1111'
            managing_project_id: '0:aaaa'
            """

        When we issue "GET /api/v2/pillar/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            data:
                porto:
                    vla-1589.db.yandex.net:
                        container_options:
                            cpu_guarantee: 1c
                            cpu_limit: 1c
                            memory_guarantee: '4294967296'
                            memory_limit: '4294967296'
                            net_guarantee: 'default: 16777216'
                            net_limit: 'default: 16777216'
                            io_limit: '104857600'
                            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
                            pending_delete: False
                            project_id: '0:1111'
                            managing_project_id: '0:aaaa'
                        volumes:
                          - backend: native
                            path: /
                            dom0_path: /data/vla-1589.db.yandex.net/rootfs
                            space_limit: 10737418240
                            pending_backup: False
                            read_only: False
                          - backend: native
                            path: /var/lib/postgresql
                            dom0_path: /data/vla-1589.db.yandex.net/data
                            space_limit: 107374182400
                            pending_backup: False
                            read_only: False
                use_vlan688: True
            """

    Scenario: Launch of single container with standalone project_id and extra_properties works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we issue "PUT /api/v2/containers/vla-1589.db.yandex.net" with:
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
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
                cpu_guarantee: 1
                cpu_limit: 1
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                extra_properties:
                    project_id: '0:1589'
                project_id: '0:1111'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-1589.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /data/vla-1589.db.yandex.net/data
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

         When we issue "GET /api/v2/containers/vla-1589.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            fqdn: vla-1589.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
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
            project_id: '0:1111'
            managing_project_id: null
            """

        When we issue "GET /api/v2/pillar/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            data:
                porto:
                    vla-1589.db.yandex.net:
                        container_options:
                            cpu_guarantee: 1c
                            cpu_limit: 1c
                            memory_guarantee: '4294967296'
                            memory_limit: '4294967296'
                            net_guarantee: 'default: 16777216'
                            net_limit: 'default: 16777216'
                            io_limit: '104857600'
                            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
                            pending_delete: False
                            project_id: '0:1111'
                        volumes:
                          - backend: native
                            path: /
                            dom0_path: /data/vla-1589.db.yandex.net/rootfs
                            space_limit: 10737418240
                            pending_backup: False
                            read_only: False
                          - backend: native
                            path: /var/lib/postgresql
                            dom0_path: /data/vla-1589.db.yandex.net/data
                            space_limit: 107374182400
                            pending_backup: False
                            read_only: False
                use_vlan688: True
            """

    Scenario: Launch of single container with standalone project_id property works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we issue "PUT /api/v2/containers/vla-1589.db.yandex.net" with:
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
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
                cpu_guarantee: 1
                cpu_limit: 1
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                project_id: '0:1111'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-1589.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /data/vla-1589.db.yandex.net/data
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

         When we issue "GET /api/v2/containers/vla-1589.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            fqdn: vla-1589.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
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
            extra_properties: null
            project_id: '0:1111'
            managing_project_id: null
            """

        When we issue "GET /api/v2/pillar/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            data:
                porto:
                    vla-1589.db.yandex.net:
                        container_options:
                            cpu_guarantee: 1c
                            cpu_limit: 1c
                            memory_guarantee: '4294967296'
                            memory_limit: '4294967296'
                            net_guarantee: 'default: 16777216'
                            net_limit: 'default: 16777216'
                            io_limit: '104857600'
                            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
                            pending_delete: False
                            project_id: '0:1111'
                        volumes:
                          - backend: native
                            path: /
                            dom0_path: /data/vla-1589.db.yandex.net/rootfs
                            space_limit: 10737418240
                            pending_backup: False
                            read_only: False
                          - backend: native
                            path: /var/lib/postgresql
                            dom0_path: /data/vla-1589.db.yandex.net/data
                            space_limit: 107374182400
                            pending_backup: False
                            read_only: False
                use_vlan688: True
            """

    Scenario: Launch of single container with standalone managing_project_id and extra_properties works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we issue "PUT /api/v2/containers/vla-1589.db.yandex.net" with:
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
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
                cpu_guarantee: 1
                cpu_limit: 1
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                extra_properties:
                    project_id: '0:1589'
                managing_project_id: '0:aaaa'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/vla-1589.db.yandex.net/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /data/vla-1589.db.yandex.net/data
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

         When we issue "GET /api/v2/containers/vla-1589.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            fqdn: vla-1589.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
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
            managing_project_id: '0:aaaa'
            """

        When we issue "GET /api/v2/pillar/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            data:
                porto:
                    vla-1589.db.yandex.net:
                        container_options:
                            cpu_guarantee: 1c
                            cpu_limit: 1c
                            memory_guarantee: '4294967296'
                            memory_limit: '4294967296'
                            net_guarantee: 'default: 16777216'
                            net_limit: 'default: 16777216'
                            io_limit: '104857600'
                            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-1589.db.yandex.net
                            pending_delete: False
                            project_id: '0:1589'
                            managing_project_id: '0:aaaa'
                        volumes:
                          - backend: native
                            path: /
                            dom0_path: /data/vla-1589.db.yandex.net/rootfs
                            space_limit: 10737418240
                            pending_backup: False
                            read_only: False
                          - backend: native
                            path: /var/lib/postgresql
                            dom0_path: /data/vla-1589.db.yandex.net/data
                            space_limit: 107374182400
                            pending_backup: False
                            read_only: False
                use_vlan688: True
            """
