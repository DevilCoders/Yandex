Feature: Ext_pillar handler works correctly

    Scenario: Ext_pillar handler works as expected
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
          And we issue "GET /api/v2/pillar/vla1-0001.tst.yandex.net" with:
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
                    vla-123.db.yandex.net:
                        container_options:
                            cpu_guarantee: 1c
                            cpu_limit: 1c
                            memory_guarantee: '4294967296'
                            memory_limit: '4294967296'
                            net_guarantee: 'default: 16777216'
                            net_limit: 'default: 16777216'
                            io_limit: '104857600'
                            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
                            pending_delete: False
                            project_id: '0:1589'
                        volumes:
                          - backend: native
                            path: /
                            dom0_path: /data/vla-123.db.yandex.net/rootfs
                            space_limit: 10737418240
                            pending_backup: False
                            read_only: False
                          - backend: native
                            path: /var/lib/postgresql
                            dom0_path: /data/vla-123.db.yandex.net/data
                            space_limit: 107374182400
                            pending_backup: False
                            read_only: False
                use_vlan688: True
            """

         When we double CPU for container vla-123.db.yandex.net
          And we issue "GET /api/v2/pillar/vla1-0001.tst.yandex.net" with:
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
                    vla-123.db.yandex.net:
                        container_options:
                            cpu_guarantee: 2c
                            cpu_limit: 2c
                            memory_guarantee: '4294967296'
                            memory_limit: '4294967296'
                            net_guarantee: 'default: 16777216'
                            net_limit: 'default: 16777216'
                            io_limit: '104857600'
                            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
                            pending_delete: False
                            project_id: '0:1589'
                        volumes:
                          - backend: native
                            path: /
                            dom0_path: /data/vla-123.db.yandex.net/rootfs
                            space_limit: 10737418240
                            pending_backup: False
                            read_only: False
                          - backend: native
                            path: /var/lib/postgresql
                            dom0_path: /data/vla-123.db.yandex.net/data
                            space_limit: 107374182400
                            pending_backup: False
                            read_only: False
                use_vlan688: True
            """

         When we delete container vla-123.db.yandex.net
          And we issue "GET /api/v2/pillar/vla1-0001.tst.yandex.net" with:
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
                            dom0_path: /data/vla-123.db.yandex.net/rootfs
                            pending_backup: False
                          - path: /var/lib/postgresql
                            pending_backup: False
                use_vlan688: True
            """

    Scenario: Omitted container limits are not returned
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
                cpu_limit: 0.25
                memory_limit: 4 GB
                hugetlb_limit: 1 GB
                net_limit: 16 MB
                io_limit: 100 MB
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
                    vla-123.db.yandex.net:
                        container_options:
                            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
                            cpu_limit: 0.25c
                            memory_limit: '4294967296'
                            hugetlb_limit: '1073741824'
                            net_limit: 'default: 16777216'
                            io_limit: '104857600'
                            pending_delete: False
                        volumes:
                          - backend: native
                            path: /
                            dom0_path: /data/vla-123.db.yandex.net/rootfs
                            space_limit: 10737418240
                            pending_backup: False
                            read_only: False
                          - backend: native
                            path: /var/lib/postgresql
                            dom0_path: /data/vla-123.db.yandex.net/data
                            space_limit: 107374182400
                            pending_backup: False
                            read_only: False
                use_vlan688: True
            """
         When we double CPU for container vla-123.db.yandex.net
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
                    vla-123.db.yandex.net:
                        container_options:
                            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-123.db.yandex.net
                            cpu_guarantee: 0.5c
                            cpu_limit: 0.5c
                            memory_limit: '4294967296'
                            hugetlb_limit: '1073741824'
                            net_limit: 'default: 16777216'
                            io_limit: '104857600'
                            pending_delete: False
                        volumes:
                          - backend: native
                            path: /
                            dom0_path: /data/vla-123.db.yandex.net/rootfs
                            space_limit: 10737418240
                            pending_backup: False
                            read_only: False
                          - backend: native
                            path: /var/lib/postgresql
                            dom0_path: /data/vla-123.db.yandex.net/data
                            space_limit: 107374182400
                            pending_backup: False
                            read_only: False
                use_vlan688: True
            """
