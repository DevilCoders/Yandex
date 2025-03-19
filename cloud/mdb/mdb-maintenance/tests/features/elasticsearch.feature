Feature: Elasticsearch maintenance tasks clusters selection
    Scenario: elasticsearch_update_tls_certs config selects properly
        Given elasticsearch cluster with name "test"
        And worker task "worker_task_id1" is acquired and completed by worker
        When set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        And I successfully load config with name "elasticsearch_update_tls_certs"
        Then cluster selection successfully returns
        """
        [cid1]
        """

    Scenario: elasticsearch_migrate_s3_secure_backups config selects properly
        Given elasticsearch cluster with name "test"
        And worker task "worker_task_id1" is acquired and completed by worker
        When In cluster "cid1" I set pillar path "{s3_migrate}" to bool "true"
        And I successfully load config with name "elasticsearch_migrate_s3_secure_backups"
        Then cluster selection successfully returns
        """
        [cid1]
        """        

    Scenario: elasticsearch_gold_to_platinum config selects properly
        Given elasticsearch cluster with name "test"
        And worker task "worker_task_id1" is acquired and completed by worker
        When In cluster "cid1" I set pillar path "{data,elasticsearch,edition}" to string "gold"
        And I successfully load config with name "elasticsearch_gold_to_platinum"
        Then cluster selection successfully returns
        """
        [cid1]
        """        