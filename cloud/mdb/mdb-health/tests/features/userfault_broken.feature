Feature: The health of the clusters was broken by a user

    Scenario: A cluster broken by a user should not disturb the on-call
        When create postgres cluster with name "cid1"
        And we successfully PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "myt-1.db.yandex.net",
                "services": [{
                    "name": "pg_replication",
                    "timestamp": 1629121684,
                    "status": "Dead",
                    "role": "Unknown"
                }, {
                    "name": "pgbouncer",
                    "timestamp": 1629121684,
                    "status": "Dead"
                }],
                "system": {},
                "mode": {
                    "timestamp": 1629121685,
                    "write": false,
                    "read": false,
                    "instance_userfault_broken": true
                }
            }
        }
        """
        And we successfully PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "iva-1.db.yandex.net",
                "services": [{
                    "name": "pg_replication",
                    "timestamp": 1629121684,
                    "status": "Alive",
                    "role": "Replica"
                }, {
                    "name": "pgbouncer",
                    "timestamp": 1629121684,
                    "status": "Alive"
                }],
                "system": {},
                "mode": {
                    "timestamp": 1629121685,
                    "write": false,
                    "read": true,
                    "instance_userfault_broken": true
                }
            }
        }
        """
        And force processing for cluster type "postgresql_cluster"
        And we GET "/v1/unhealthyaggregatedinfo?agg_type=clusters&c_type=postgresql_cluster&env=qa"
        Then we get response with status 200
        And we get response with content
        """
        {
            "no_sla" : {},
            "sla": {
                "by_availability":[{
                    "count": 1,
                    "examples": ["cid1"],
                    "no_read_count": 1,
                    "no_write_count": 1,
                    "readable": false,
                    "writable": false,
                    "userfaultBroken": true
                }],
                "by_health": [{
                    "count": 1,
                    "examples": ["cid1"],
                    "status": "Degraded"
                }]
            }
        }
        """

    Scenario: MySQL with master broken by a user
        When create mysql cluster with name "cid1"
        And we successfully PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "myt-1.db.yandex.net",
                "services": [{
                    "name": "mysql",
                    "timestamp": 1629121684,
                    "status": "Dead",
                    "role": "Master"
                }],
                "system": {},
                "mode": {
                    "timestamp": 1629121685,
                    "write": false,
                    "read": false,
                    "instance_userfault_broken": true
                }
            }
        }
        """
        And we successfully PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "iva-1.db.yandex.net",
                "services": [{
                    "name": "mysql",
                    "timestamp": 1629121684,
                    "status": "Alive",
                    "role": "Replica"
                }],
                "system": {},
                "mode": {
                    "timestamp": 1629121685,
                    "write": false,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        And we successfully PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "sas-1.db.yandex.net",
                "services": [{
                    "name": "mysql",
                    "timestamp": 1629121684,
                    "status": "Alive",
                    "role": "Replica"
                }],
                "system": {},
                "mode": {
                    "timestamp": 1629121685,
                    "write": false,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        And force processing for cluster type "mysql_cluster"
        And we GET "/v1/unhealthyaggregatedinfo?agg_type=clusters&c_type=mysql_cluster&env=qa"
        Then we get response with status 200
        # no_read_count = 1 and readable = false is not true,
        # but it is an unhealthy aggregator requirement that a broken cluster shouldn't have RW metrics.
        # https://a.yandex-team.ru/arc/commit/r7450077
        And we get response with content
        """
        {
            "no_sla" : {},
            "sla": {
                "by_availability":[{
                    "count": 1,
                    "examples": ["cid1"],
                    "no_read_count": 1,
                    "no_write_count": 1,
                    "readable": false,
                    "writable": false,
                    "userfaultBroken": true
                }],
                "by_health": [{
                    "count": 1,
                    "examples": ["cid1"],
                    "status": "Degraded"
                }]
            }
        }
        """
