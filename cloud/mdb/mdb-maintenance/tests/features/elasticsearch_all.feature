Feature: Elasticsearch maintenance configs
    Background: Create clusters for every config
        Given I load all configs
        And elasticsearch cluster with name "elasticsearch_update_tls_certs"
        And worker task "worker_task_id1" is acquired and completed by worker
        And set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        And I increase cloud quota
        And elasticsearch cluster with name "elasticsearch_migrate_s3_secure_backups"
        And In cluster "cid2" I set pillar path "{s3_migrate}" to bool "true"
        And worker task "worker_task_id2" is acquired and completed by worker
        And elasticsearch cluster with name "elasticsearch_switch_billing"
        And worker task "worker_task_id3" is acquired and completed by worker
        And In cluster "cid3" I set pillar path "{data,billing}"
        And In cluster "cid3" I set pillar path "{data,billing,use_cloud_logbroker}" to bool "false"
        And elasticsearch cluster with name "elasticsearch_gold_to_platinum"
        And worker task "worker_task_id4" is acquired and completed by worker
        And In cluster "cid4" I set pillar path "{data,elasticsearch,edition}" to string "gold"

    Scenario: Cluster revisions don't change
        Given I check that "elasticsearch" cluster revision doesn't change while pillar_change

    Scenario: Maintain all configs
        Given I maintain all "elasticsearch" configs
