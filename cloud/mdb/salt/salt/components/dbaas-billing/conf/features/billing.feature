Feature: Billing should works
    Background: Base setup
       Given empty monrun cache
         And billing context
         """
          {
              "cluster_id": "cid1",
              "folder_id": "folder1",
              "cloud_id": "cloud1",
              "resource_preset_id": "small.test",
              "disk_type_id": "ssd.test",
              "cluster_type": "postgre",
              "assign_public_ip": false,
              "fqdn": "test.host",
              "roles": ["postgres"],
              "disk_size": 1000,
              "platform_id": "mdb-v3",
              "cores": 8,
              "core_fraction": 100,
              "memory": 64,
              "io_cores_limit": 0,
              "compute_instance_id": "123",
              "on_dedicated_host": 0
          }
         """
         And billing checks
         """
          {"pg_ping": 100, "bouncer_ping": 100}
         """


    Scenario Outline: Billing reports when all checks are ok
       Given "foo_db.net_pg_ping" check is "0;OK"
         And "foo.db.net_bouncer_ping" check is "0;OK"
         And billing state
         """
         {"alive": true, "last_ts": 1500000000}
         """
         And current time is "<current_time>"
        When I call billing
        Then billing log appears
         And billing log contains "1" lines
         And "0" line in billing log matches
         """
         {
            "folder_id": "folder1",
            "tags": {
                "disk_type_id": "ssd.test",
                "public_ip": 0,
                "resource_preset_id": "small.test",
                "cluster_type": "postgre",
                "cluster_id": "cid1",
                "online": 1,
                "roles": ["postgres"],
                "disk_size": 1000,
                "platform_id": "mdb-v3",
                "cores": 8,
                "core_fraction": 100,
                "memory": 64,
                "software_accelerated_network_cores": 0,
                "compute_instance_id": "123",
                "on_dedicated_host": 0
            },
            "source_wt": <current_time>,
            "usage": {
                "unit": "seconds",
                "quantity": <usage_seconds>,
                "type": "delta",
                "start": 1500000000,
                "finish": <current_time>
            },
            "cloud_id": "cloud1",
            "source_id": "test.host",
            "schema": "mdb.db.generic.v1",
            "resource_id": "test.host"
         }
         """
         And service log is empty

      Examples:
        | current_time | usage_seconds |
        | 1500000030   | 30            |
        | 1500000060   | 60            |
        | 1500000080   | 80            |


    Scenario: Billing split report
       . when usage interval include hour switch
       . 1500001160 -> 2017-07-14 05:59:20+03
       . 1500001200 -> 2017-07-14 06:00:00+03
       . 1500001222 -> 2017-07-14 06:00:22+03

       Given "foo_db.net_pg_ping" check is "0;OK"
         And "foo.db.net_bouncer_ping" check is "0;OK"
         And billing state
         """
         {"alive": true, "last_ts": 1500001160}
         """
         And current time is "1500001222"
        When I call billing
        Then billing log appears
         And billing log contains "2" lines
         And "0" line in billing log matches
         """
         {
            "source_wt": 1500001200,
            "usage": {
                "unit": "seconds",
                "quantity": 40,
                "type": "delta",
                "start": 1500001160,
                "finish": 1500001200
            }
         }
         """
         And "1" line in billing log matches
         """
         {
            "source_wt": 1500001222,
            "usage": {
                "unit": "seconds",
                "quantity": 22,
                "type": "delta",
                "start": 1500001200,
                "finish": 1500001222
            }
         }
         """
         And service log is empty


    Scenario: No billing reports when one check failed
       Given "foo_db.net_pg_ping" check is "1;dead"
         And "foo.db.net_bouncer_ping" check is "0;OK"
         And billing state
         """
         {"alive": true, "last_ts": 1500000000}
         """
         And current time is "1500000060"
        When I call billing
        Then billing log appears
         And billing log is empty
         And service log is
         """
         2017-07-14 07:41:00,000 [WARNING]: pg_ping check failed: 1;dead
         2017-07-14 07:41:00,000 [INFO]: Not billed cause: currently alive: False, previously alive: True
         """


    Scenario: No billing when previous state is dead
       Given "foo_db.net_pg_ping" check is "0;OK"
         And "foo.db.net_bouncer_ping" check is "0;OK"
         And billing state
         """
         {"alive": false, "last_ts": 1500000000}
         """
         And current time is "1500000060"
        When I call billing
        Then billing log appears
         And billing log is empty
         And service log is
         """
         2017-07-14 07:41:00,000 [INFO]: Not billed cause: currently alive: True, previously alive: False
         """

        When time ticks to "1500000080"
         And I call billing
        Then billing log contains "1" lines


    Scenario: No billing reports when state outdated
       Given "foo_db.net_pg_ping" check is "0;OK"
         And "foo.db.net_bouncer_ping" check is "0;OK"
         And billing state
         """
         {"alive": true, "last_ts": 1500000000}
         """
         And current time is "1500600000"
        When I call billing
        Then billing log appears
         But billing log is empty
         And service log is
         """
         2017-07-21 06:20:00,000 [WARNING]: Billing state outdated: BillingState(last_ts=1500000000, alive=True), current_ts: 1500600000
         2017-07-21 06:20:00,000 [INFO]: Not billed cause: currently alive: True, previously alive: False
         """


    Scenario: No billing reports when billing state not exists
       Given "foo_db.net_pg_ping" check is "0;OK"
         And "foo.db.net_bouncer_ping" check is "0;OK"
         And billing state not exists
         And current time is "1500000000"
        When I call billing
        Then billing log appears
         But billing log is empty
         And service log matches
         """
         .*: Unable to get_state, cause: \[Errno 2\] No such file or directory: '.*billing.state'
         .*: Not billed cause: currently alive: True, previously alive: False
         """


    Scenario: No billing reports when check old mtime
       Given "foo_db.net_pg_ping" check is "0;OK" modified at "1000000000"
         And "foo.db.net_bouncer_ping" check is "0;OK"
         And billing state
         """
         {"alive": true, "last_ts": 1500000000}
         """
         And current time is "1500000060"
        When I call billing
        Then billing log appears
         And billing log is empty
         And service log is
         """
         2017-07-14 07:41:00,000 [WARNING]: pg_ping check failed: no fresh check found
         2017-07-14 07:41:00,000 [INFO]: Not billed cause: currently alive: False, previously alive: True
         """
