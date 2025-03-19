Feature: Volumes handlers work correctly

    Scenario: Volumes info works correctly
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
          And we issue "GET /api/v2/volumes/?query=container=vla-123.db.yandex.net;" with:
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
              path: /var/lib/postgresql
              dom0: vla1-0001.tst.yandex.net
              dom0_path: /data/vla-123.db.yandex.net/data
              backend: native
              space_guarantee: null
              space_limit: 107374182400
              inode_guarantee: null
              inode_limit: null
              read_only: False
              pending_backup: False
            """
         When we issue "GET /api/v2/volumes/?query=no_field=yes" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 400
          And body is:
            """
            error: 'Unknown key: no_field'
            """
         When we issue "GET /api/v2/volumes/?query=XXX" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 422
          And body is:
            """
            query:
              - "Invalid query: 'XXX'"
            """
         When we issue "GET /api/v2/volumes/?query=a=" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 422
          And body is:
            """
            query:
              - "Invalid pair: 'a='"
            """
         When we issue "GET /api/v2/volumes/?query=read_only=yes" with:
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

    Scenario: Modify of single volume with invalid size fails
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
          And we issue "POST /api/v2/volumes/vla-123.db.yandex.net?path=/var/lib/postgresql" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                space_limit: -3
            """
         Then we get response with code 422
          And body contains:
            """
            space_limit:
                - 'Negative size: -3'
            """
         When we issue "POST /api/v2/volumes/vla-123.db.yandex.net?path=/var/lib/postgresql" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                space_limit: XXX
            """
         Then we get response with code 422
          And body contains:
            """
            space_limit:
                - 'Invalid size: XXX'
            """

    Scenario: Modify of single volume works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
          And we issue "POST /api/v2/volumes/vla-123.db.yandex.net?path=/var/lib/postgresql" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                space_limit: 20 GB
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
              path: /var/lib/postgresql
              dom0: vla1-0001.tst.yandex.net
              dom0_path: /data/vla-123.db.yandex.net/data
              backend: native
              space_guarantee: null
              space_limit: 21474836480
              inode_guarantee: null
              inode_limit: null
              read_only: False
              pending_backup: False
            """

    Scenario: Modify of nonexistent volume fails
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
          And we issue "POST /api/v2/volumes/vla-123.db.yandex.net?path=/var/lib/clickhouse" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                space_limit: 20 GB
            """
         Then we get response with code 404
          And body contains:
            """
            error: No such volume
            """

    Scenario: Modify of single volume without starting deploy works
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
          And we issue "POST /api/v2/volumes/vla-123.db.yandex.net?path=/var/lib/postgresql&init_deploy=false" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                space_limit: 20 GB
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
              path: /var/lib/postgresql
              dom0: vla1-0001.tst.yandex.net
              dom0_path: /data/vla-123.db.yandex.net/data
              backend: native
              space_guarantee: null
              space_limit: 21474836480
              inode_guarantee: null
              inode_limit: null
              read_only: False
              pending_backup: False
            """
