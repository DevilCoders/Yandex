Feature: Overcommit skip defined by config works correctly

    Scenario: Overcommit for containers matching regex from config is allowed
        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we launch container vla-mycritical.db.yandex.net in cluster 111
         Then container info for vla-mycritical.db.yandex.net contains:
            """
            fqdn: vla-mycritical.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            """
         When we launch container vla-456.db.yandex.net in cluster 222
         Then container info for vla-456.db.yandex.net contains:
            """
            fqdn: vla-456.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            """
         When we issue "POST /api/v2/containers/vla-mycritical.db.yandex.net" with:
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
          And body contains:
            """
            deploy:
              deploy_id: test-jid
              deploy_version: 2
              deploy_env: dockertest
              host: vla1-0001.tst.yandex.net
            """
