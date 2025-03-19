Feature: Deletion Protection
    Background:
        Given default headers
        When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
        """
        {
            "name": "test",
            "environment": "PRESTABLE",
            "configSpec": {
                "mysqlConfig_5_7": {
                },
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "databaseSpecs": [{
                "name": "testdb"
            }],
            "userSpecs": [{
                "name": "test",
                "password": "test_password"
            }],
            "hostSpecs": [{
                "zoneId": "myt"
            }, {
                "zoneId": "iva"
            }, {
                "zoneId": "sas"
            }],
            "description": "test cluster",
            "networkId": "IN-PORTO-NO-NETWORK-API",
            "deletionProtection": false
        }
        """
        Then we get response with status 200
        And "worker_task_id1" acquired and finished by worker

        When we GET "/mdb/mysql/1.0/clusters/cid1"
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
        When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
        """
        {
            "deletionProtection": true
        }
        """
        Then we get response with status 200 and body contains
        """
        {
            "createdBy": "user",
            "description": "Modify MySQL cluster",
            "done": true
        }
        """
        When we GET "/mdb/mysql/1.0/clusters/cid1"
        Then we get response with status 200 and body contains
        """
        {
            "deletionProtection": true
        }
        """
        #
        # TEST deletion_protection
        #
        When we DELETE "/mdb/mysql/1.0/clusters/cid1"
        Then we get response with status 422
        #
        # DISABLE deletion_protection
        #
        When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
        """
        {
            "deletionProtection": false
        }
        """
        Then we get response with status 200 and body contains
        """
        {
            "createdBy": "user",
            "description": "Modify MySQL cluster",
            "done": true
        }
        """
        When we GET "/mdb/mysql/1.0/clusters/cid1"
        Then we get response with status 200 and body contains
        """
        {
            "deletionProtection": false
        }
        """
        #
        # TEST deletion_protection
        #
        When we DELETE "/mdb/mysql/1.0/clusters/cid1"
        Then we get response with status 200


    Scenario: Resource Reaper can overcome delete_protection
        #
        # ENABLE deletion_protection
        #
        When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
        """
        {
            "deletionProtection": true
        }
        """
        Then we get response with status 200 and body contains
        """
        {
            "createdBy": "user",
            "description": "Modify MySQL cluster",
            "done": true
        }
        """
        When we GET "/mdb/mysql/1.0/clusters/cid1"
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
        And we DELETE "/mdb/mysql/1.0/clusters/cid1"
        Then we get response with status 200
