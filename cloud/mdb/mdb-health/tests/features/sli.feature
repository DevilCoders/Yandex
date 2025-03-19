Feature: SLI

    @MDB-14522
    Scenario: E2E cluster that doesn't send metrics
    . shouldn't be shown in unhealthyaggregatedinfo
    . but it's health should be calculated
        When we in "folder3" create postgres cluster with name "dbaas_e2e_func_tests" using "rw-e2e-token" token
        And force processing for cluster type "postgresql_cluster"
        And we GET "/v1/unhealthyaggregatedinfo?agg_type=clusters&c_type=postgresql_cluster&env=qa"
        Then we get response with status 200
        And we get response with content
        """
        {
            "no_sla" : {},
            "sla": {}
        }
        """
        When we PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "myt-1.db.yandex.net",
                "services": [{
                    "name": "pg_replication",
                    "timestamp": 1629121999,
                    "status": "Dead",
                    "role": "Master"
                }, {
                    "name": "pgbouncer",
                    "timestamp": 1629121999,
                    "status": "Dead"
                }]
            }
        }
        """
        Then we get response with status 200
        When force processing for cluster type "postgresql_cluster"
        And we GET "/v1/clusterhealth?cid=cid1"
        Then we get response with status 200
        And we get a response that contains
        """
        {
            "status": "Degraded"
        }
        """
        When we GET "/v1/unhealthyaggregatedinfo?agg_type=clusters&c_type=postgresql_cluster&env=qa"
        Then we get response with status 200
        And we get response with content
        """
        {
            "no_sla" : {},
            "sla": {}
        }
        """
