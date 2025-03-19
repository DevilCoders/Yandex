Feature: Volume backups handles work correctly

    Scenario: Volume backups after container deletion
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
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

         # Check for deletion idempotence
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

         When we run query
            """
            UPDATE mdb.volume_backups SET delete_token = 'ffffffff-ffff-ffff-ffff-ffffffffffff'
            """
          And we issue "DELETE /api/v2/volume_backups_with_token/vla1-0001.tst.yandex.net/vla-123.db.yandex.net?path=/" with:
            """
            timeout: 5
            headers:
                Accept: application/json
                Content-Type: application/json
            body:
                token: ffffffff-ffff-ffff-ffff-ffffffffffff
            """
         Then we get response with code 400
          And body is:
            """
            error: Container exists
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

         When we issue "DELETE /api/v2/volume_backups_with_token/vla1-0001.tst.yandex.net/vla-123.db.yandex.net?path=/" with:
            """
            timeout: 5
            headers:
                Accept: application/json
                Content-Type: application/json
            body:
                token: 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 400
          And body is:
            """
            error: Invalid token
            """

         When we issue "POST /api/v2/dom0/volume-backup-delete-report/vla1-0001.tst.yandex.net?path=/data/vla-123.db.yandex.net/rootfs" with:
            """
            timeout: 5
            headers:
                Accept: application/json
                Content-Type: application/json
            body:
                token: ffffffff-ffff-ffff-ffff-ffffffffffff
            """
         Then we get response with code 400
          And body is:
            """
            error: Invalid token
            """

         When we issue "DELETE /api/v2/volume_backups_with_token/vla1-0001.tst.yandex.net/vla-123.db.yandex.net?path=/" with:
            """
            timeout: 5
            headers:
                Accept: application/json
                Content-Type: application/json
            body:
                token: ffffffff-ffff-ffff-ffff-ffffffffffff
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

         When we issue "POST /api/v2/dom0/volume-backup-delete-report/vla1-0001.tst.yandex.net?path=/data/vla-123.db.yandex.net/rootfs" with:
            """
            timeout: 5
            headers:
                Accept: application/json
                Content-Type: application/json
            body:
                token: ffffffff-ffff-ffff-ffff-ffffffffffff
            """
         Then we get response with code 200

         When we issue "DELETE /api/v2/volume_backups/vla1-0001.tst.yandex.net/vla-123.db.yandex.net?path=/var/lib/postgresql" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
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

         When we issue "POST /api/v2/dom0/volume-backup-delete-report/vla1-0001.tst.yandex.net?path=/data/vla-123.db.yandex.net/data" with:
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
          And body is:
            """
            []
            """

    Scenario: Volume backups after container transfer
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net
          And we issue heartbeat for vla2-0002.tst.yandex.net dom0
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
              dom0: vla1-0001.tst.yandex.net
              path: /
            - container: vla-123.db.yandex.net
              dom0: vla1-0001.tst.yandex.net
              path: /var/lib/postgresql
            """

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

         When we issue "GET /api/v2/volume_backups/?query=container=vla-123.db.yandex.net" with:
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

    Scenario: Volume backups after container transfer with volume resize
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-123.db.yandex.net in cluster 111
          And we launch container vla-456.db.yandex.net in cluster 222
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
              dom0: vla1-0001.tst.yandex.net
              path: /
              space_limit: 10737418240
            - container: vla-123.db.yandex.net
              dom0: vla1-0001.tst.yandex.net
              path: /var/lib/postgresql
              space_limit: 107374182400
            """
