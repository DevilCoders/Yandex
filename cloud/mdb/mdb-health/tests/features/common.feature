Feature: Health common feature

    Scenario: Health ping
        When we GET "/v1/ping"
        Then we get response with status 200

    Scenario: List health of unknown host
        When we POST "/v1/listhostshealth"
        """
        {"hosts": ["myt-1.db.yandex.net"]}
        """
        Then we get response with status 200
        And we get response with content
        """
        {
            "hosts": [
                {
                    "fqdn": "myt-1.db.yandex.net",
                    "status": "Unknown",
                    "services": "**IGNORE**",
                    "cid": "**IGNORE**"
                }
            ]
        }
        """

    Scenario: Put health of new host
        When we GET "/v1/hostshealth?fqdns=myt-1.db.yandex.net"
        Then we get response with status 200
        And we get response with content
        """
        {
            "hosts": [
                {
                    "fqdn": "myt-1.db.yandex.net",
                    "services": [],
                    "cid": "**IGNORE**",
                    "status": "**IGNORE**"
                }
            ]
        }
        """
        When create postgres cluster with name "cid1"
        And force processing for cluster type "postgresql_cluster"
        And we PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "myt-1.db.yandex.net",
                "services": [{
                    "name": "pg_replication",
                    "timestamp": 1629121684,
                    "status": "Alive",
                    "role": "Master"
                }],
                "system": {},
                "mode": {
                    "timestamp": 1629121685,
                    "write": true,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        Then we get response with status 200
        When we GET "/v1/hostshealth?fqdns=myt-1.db.yandex.net"
        Then we get response with status 200
        # We don't load mode field https://a.yandex-team.ru/arc_vcs/cloud/mdb/mdb-health/pkg/datastore/redis/redis.go?rev=r8483023#L51
        And we get response with content
        """
        {
            "hosts": [
                {
                    "fqdn": "myt-1.db.yandex.net",
                    "services": [],
                    "cid": "**IGNORE**",
                    "status": "**IGNORE**",
                    "services": [{
                        "name": "pg_replication",
                        "role": "Master",
                        "status": "Alive",
                        "timestamp": 1629121684
                    }]
                }
            ]
        }
        """

    Scenario: Create cluster and update health of hosts step-by-step
        When create postgres cluster with name "cid1"
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
                    "userfaultBroken": false
                }],
                "by_health": [{
                    "count": 1,
                    "examples": ["cid1"],
                    "status": "Unknown"
                }]
            }
        }
        """
        When we PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "myt-1.db.yandex.net",
                "status": "Alive",
                "services": [{
                    "name": "pg_replication",
                    "timestamp": 1629121684,
                    "status": "Alive",
                    "role": "Master"
                }, {
                    "name": "pgbouncer",
                    "timestamp": 1629121684,
                    "status": "Alive"
                }],
                "system": {},
                "mode": {
                    "timestamp": 1629121685,
                    "write": true,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        Then we get response with status 200
        When force processing for cluster type "postgresql_cluster"
        And we GET "/v1/unhealthyaggregatedinfo?agg_type=clusters&c_type=postgresql_cluster&env=qa"
        Then we get response with status 200
        And we get response with content
        """
        {
            "no_sla" : {},
            "sla": {
                "by_availability":[{
                    "count": 1,
                    "examples": [],
                    "no_read_count": 0,
                    "no_write_count": 0,
                    "readable": true,
                    "writable": true,
                    "userfaultBroken": false
                }],
                "by_health": [{
                    "count": 1,
                    "examples": ["cid1"],
                    "status": "Degraded"
                }],
                "by_warning_geo": [{
                    "count": 1,
                    "examples": ["cid1"],
                    "geo": "myt"
                }]
            }
        }
        """
        When we PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "sas-1.db.yandex.net",
                "status": "Alive",
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
                    "write": true,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        Then we get response with status 200
        When force processing for cluster type "postgresql_cluster"
        And we GET "/v1/unhealthyaggregatedinfo?agg_type=clusters&c_type=postgresql_cluster&env=qa"
        Then we get response with status 200
        And we get response with content
        """
        {
            "no_sla" : {},
            "sla": {
                "by_availability":[{
                    "count": 1,
                    "examples": [],
                    "no_read_count": 0,
                    "no_write_count": 0,
                    "readable": true,
                    "writable": true,
                    "userfaultBroken": false
                }],
                "by_health": [{
                    "count": 1,
                    "examples": ["cid1"],
                    "status": "Degraded"
                }],
                "by_warning_geo": [{
                    "count": 1,
                    "examples": ["cid1"],
                    "geo": "**IGNORE**"
                },{
                    "count": 1,
                    "examples": ["cid1"],
                    "geo": "**IGNORE**"
                }]
            }
        }
        """
        When we PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "iva-1.db.yandex.net",
                "status": "Alive",
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
                    "write": true,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        Then we get response with status 200
        When force processing for cluster type "postgresql_cluster"
        And we GET "/v1/unhealthyaggregatedinfo?agg_type=clusters&c_type=postgresql_cluster&env=qa"
        Then we get response with status 200
        And we get response with content
        """
        {
            "no_sla" : {},
            "sla": {
                "by_availability":[{
                    "count": 1,
                    "examples": [],
                    "no_read_count": 0,
                    "no_write_count": 0,
                    "readable": true,
                    "writable": true,
                    "userfaultBroken": false
                }],
                "by_health": [{
                    "count": 1,
                    "examples": [],
                    "status": "Alive"
                }]
            }
        }
        """
