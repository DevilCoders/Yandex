Feature: Deletion Protection
    Background:
        Given feature flags
        """
        ["MDB_REDIS_ALLOW_DEPRECATED_5"]
        """
        And default headers
        When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
        """
        {
            "name": "test",
            "environment": "PRESTABLE",
            "configSpec": {
                "redisConfig_5_0": {
                    "password": "p@ssw#$rd!?",
                    "databases": 15
                },
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskSize": 17179869184
                }
            },
            "hostSpecs": [{
                "zoneId": "myt",
                "replicaPriority": 50
            }, {
                "zoneId": "iva"
            }, {
                "zoneId": "sas"
            }],
            "description": "test cluster",
            "networkId": "IN-PORTO-NO-NETWORK-API"
        }
        """
        Then we get response with status 200
        And "worker_task_id1" acquired and finished by worker

        When we GET "/mdb/redis/1.0/clusters/cid1"
        Then we get response with status 200 and body contains
        """
        {
            "deletionProtection": false
        }
        """

    Scenario: delete_protection works
        #
        # ENABLE deletion_protection
        #
        When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
        """
        {
            "deletionProtection": true
        }
        """
        Then we get response with status 200 and body contains
        """
        {
            "createdBy": "user",
            "description": "Modify Redis cluster",
            "done": true
        }
        """
        When we GET "/mdb/redis/1.0/clusters/cid1"
        Then we get response with status 200 and body contains
        """
        {
            "deletionProtection": true
        }
        """
        #
        # TEST deletion_protection
        #
        When we DELETE "/mdb/redis/1.0/clusters/cid1"
        Then we get response with status 422
        #
        # DISABLE deletion_protection
        #
        When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
        """
        {
            "deletionProtection": false
        }
        """
        Then we get response with status 200 and body contains
        """
        {
            "createdBy": "user",
            "description": "Modify Redis cluster",
            "done": true
        }
        """
        When we GET "/mdb/redis/1.0/clusters/cid1"
        Then we get response with status 200 and body contains
        """
        {
            "deletionProtection": false
        }
        """
        #
        # TEST deletion_protection
        #
        When we DELETE "/mdb/redis/1.0/clusters/cid1"
        Then we get response with status 200

    Scenario: Resource Reaper can overcome delete_protection
        #
        # ENABLE deletion_protection
        #
        When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
        """
        {
            "deletionProtection": true
        }
        """
        Then we get response with status 200 and body contains
        """
        {
            "createdBy": "user",
            "description": "Modify Redis cluster",
            "done": true
        }
        """
        When we GET "/mdb/redis/1.0/clusters/cid1"
        Then we get response with status 200 and body contains
        """
        {
            "deletionProtection": true
        }
        """
        #
        # TEST deletion_protection
        #
        When default headers with "resource-reaper-service-token" token
        And we DELETE "/mdb/redis/1.0/clusters/cid1"
        Then we get response with status 200
