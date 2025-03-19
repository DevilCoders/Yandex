Feature: Transfers work correctly

    Scenario: Overcommit without transfer is forbidden
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
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
            dom0: vla1-0001.tst.yandex.net
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
         Then we get response with code 400
          And body is
            """
            error: Unable to reallocate container
            """

    Scenario: Overcommit on resources triggers transfer
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
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
            dom0: vla1-0001.tst.yandex.net
            """
         When we issue heartbeat for vla2-0002.tst.yandex.net dom0
          And we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
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
                    vla-456.db.yandex.net:
                        container_options:
                            cpu_guarantee: 1c
                            cpu_limit: 1c
                            memory_guarantee: '4294967296'
                            memory_limit: '4294967296'
                            net_guarantee: 'default: 16777216'
                            net_limit: 'default: 16777216'
                            io_limit: '104857600'
                            bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh vla-456.db.yandex.net
                            pending_delete: False
                            project_id: '0:1589'
                        volumes:
                          - backend: native
                            path: /
                            dom0_path: /data/vla-456.db.yandex.net/rootfs
                            space_limit: 10737418240
                            pending_backup: False
                            read_only: False
                          - backend: native
                            path: /var/lib/postgresql
                            dom0_path: /data/vla-456.db.yandex.net/data
                            space_limit: 107374182400
                            pending_backup: False
                            read_only: False
                use_vlan688: True
            """
         When we issue "GET /api/v2/pillar/vla2-0002.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            data:
                porto: {}
                use_vlan688: True
            """
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_cores: 56
            free_cores: -1
            """
         When we issue "GET /api/v2/dom0/vla2-0002.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_cores: 56
            free_cores: 0
            """
         When we run query
            """
            UPDATE mdb.transfers SET id = 'ffffffff-ffff-ffff-ffff-ffffffffffff'
            """
         When we issue "GET /api/v2/transfers/3" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 404
         When we issue "GET /api/v2/transfers/?fqdn=nonexistent" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
          """
          {}
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
         When we issue "GET /api/v2/transfers/?fqdn=vla-123.db.yandex.net" with:
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
         When we issue "GET /api/v2/pillar/vla2-0002.tst.yandex.net" with:
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
                            cpu_guarantee: 56c
                            cpu_limit: 56c
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
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_cores: 56
            free_cores: 55
            """
         When we issue "GET /api/v2/dom0/vla2-0002.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_cores: 56
            free_cores: 0
            """
         When we issue "GET /api/v2/transfers/ffffffff-ffff-ffff-ffff-ffffffffffff" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 404

    Scenario: Overcommit on space triggers transfer
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
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
            dom0: vla1-0001.tst.yandex.net
            """
         When we issue heartbeat for vla2-0002.tst.yandex.net dom0
          And we issue "POST /api/v2/volumes/vla-123.db.yandex.net?path=/var/lib/postgresql" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                space_limit: 7000 GB
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

    Scenario: Incompatible generation change triggers transfer
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
                generation: '1'
                disks: []
            """
         Then we get response with code 200
         When we launch container vla-123.db.yandex.net in cluster 111
         Then container info for vla-123.db.yandex.net contains:
            """
            fqdn: vla-123.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            """
         When we launch container vla-456.db.yandex.net in cluster 222
         Then container info for vla-456.db.yandex.net contains:
            """
            fqdn: vla-456.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            """
         When we issue heartbeat for vla2-0002.tst.yandex.net dom0
          And we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                generation: '2'
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

    Scenario: Cancelling transfer works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
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
            dom0: vla1-0001.tst.yandex.net
            """
         When we issue heartbeat for vla2-0002.tst.yandex.net dom0
          And we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
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
         When we issue "POST /api/v2/transfers/ffffffff-ffff-ffff-ffff-ffffffffffff/cancel" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
          """
          {}
          """
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_cores: 56
            free_cores: -1
            """
         When we issue "GET /api/v2/dom0/vla2-0002.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            total_cores: 56
            free_cores: 56
            """
         When we issue "GET /api/v2/transfers/ffffffff-ffff-ffff-ffff-ffffffffffff" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 404

    Scenario: Dom0 change requires transfer
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
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
            dom0: vla1-0001.tst.yandex.net
            """
         When we issue heartbeat for vla2-0002.tst.yandex.net dom0
          And we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                dom0: vla2-0002.tst.yandex.net
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

    Scenario: Dom0 change to null triggers transfer
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
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
            dom0: vla1-0001.tst.yandex.net
            """
         When we issue heartbeat for vla2-0002.tst.yandex.net dom0
          And we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                dom0: null
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

    Scenario: Dom0 change to host without resources fails
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
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
            dom0: vla1-0001.tst.yandex.net
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
                cpu_cores: 0
                memory: 269157273600
                ssd_space: 7542075424768
                sata_space: 0
                max_io: 1468006400
                net_speed: 1310720000
                heartbeat: 1
                generation: 2
                disks: []
            """
         Then we get response with code 200
         When we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                dom0: vla2-0002.tst.yandex.net
            """
         Then we get response with code 400
          And body is
            """
            error: Unable to reallocate container
            """

    Scenario: Container removal with active transfer is not possible
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net in cluster 111
         Then container info for vla-123.db.yandex.net contains:
            """
            fqdn: vla-123.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            """
         When we issue heartbeat for vla2-0002.tst.yandex.net dom0
          And we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                dom0: null
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
         When we issue "DELETE /api/v2/containers/vla-123.db.yandex.net" with
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            """
         Then we get response with code 400
          And body is
            """
            error: 'Active transfer exists: ffffffff-ffff-ffff-ffff-ffffffffffff'
            """

    @concurrency
    Scenario: Transfer concurrency
        Given a deployed DBM
        And empty DB
        When we issue heartbeat for vla1-0001.tst.yandex.net dom0
        And we launch container vla-123.db.yandex.net
        And we lock vla
        And we issue "POST /api/v2/containers/vla-123.db.yandex.net" with:
            """
            timeout: 15
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                cpu_guarantee: 128
                cpu_limit: 128
            """
        Then we get response with code 503
        And body contains error matches:
            """
            Another allocation in progress. Geo vla lock not available. Locked by: dbm@dbm pid:[0-9]+ session.*in 'idle in transaction' state
            """
