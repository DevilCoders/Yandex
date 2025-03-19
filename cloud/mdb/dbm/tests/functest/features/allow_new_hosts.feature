Feature: Managing allow_new_hosts for dom0

    Scenario: Setting allow_new_hosts works
      Given a deployed DBM
        And empty DB
       When we issue heartbeat for vla1-0001.tst.yandex.net dom0
        And we issue "PUT /api/v2/dom0/allow_new_hosts/vla1-0001.tst.yandex.net" with:
              """
              timeout: 5
              headers:
                  Authorization: OAuth 11111111-1111-1111-1111-111111111111
                  Accept: application/json
                  Content-Type: application/json
              body:
                  allow_new_hosts: false
              """
       Then we get response with code 200
        And body contains:
              """
              allow_new_hosts: false
              allow_new_hosts_updated_by: alice
              """
       When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
              """
              timeout: 5
              headers:
                  Authorization: OAuth 11111111-1111-1111-1111-111111111111
              """
       Then we get response with code 200
        And body contains:
              """
              allow_new_hosts: false
              allow_new_hosts_updated_by: alice
              """
       When we issue "PUT /api/v2/dom0/allow_new_hosts/vla1-0001.tst.yandex.net" with:
              """
              timeout: 5
              headers:
                  Authorization: OAuth 11111111-1111-1111-1111-111111111111
                  Accept: application/json
                  Content-Type: application/json
              body:
                  allow_new_hosts: true
              """
       Then we get response with code 200
        And body contains:
              """
              allow_new_hosts: true
              allow_new_hosts_updated_by: null
              """

    Scenario: Setting allow_new_hosts return 404 on nonexistent dom0
      Given a deployed DBM
        And empty DB
       When we issue "PUT /api/v2/dom0/allow_new_hosts/vla1-0001.tst.yandex.net" with:
              """
              timeout: 5
              headers:
                  Authorization: OAuth 11111111-1111-1111-1111-111111111111
                  Accept: application/json
                  Content-Type: application/json
              body:
                  allow_new_hosts: true
              """
       Then we get response with code 404
