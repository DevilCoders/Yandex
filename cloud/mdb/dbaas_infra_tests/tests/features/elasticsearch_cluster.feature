@dependent-scenarios
Feature: Internal API ElasticSearch cluster lifecycle

  Background: Wait until internal api is ready
    Given Go Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard ElasticSearch cluster
    And set user defined Elasticsearch version

  @setup
  Scenario: Create ElasticSearch cluster
    When we create "ElasticSearch" cluster "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    And cluster has no pending changes
    And cluster environment is "PRESTABLE"
    And s3 has bucket for cluster
    When we index the following documents to "test-index"
    """
    [
        {
            "_id": "0",
            "name": "postgresql",
            "type": "regular"
        },
        {
            "_id": "1",
            "name": "mysql",
            "type": "regular"
        },
        {
            "_id": "2",
            "name": "mongodb",
            "type": "spec"
        },
        {
            "_id": "3",
            "name": "clickhouse",
            "type": "spec"
        }
    ]
    """
    Then number of documents in "test-index" equals to "4"
    When we search "test-index" with the following query on all hosts
    """
    {
        "query": {
            "match": {
                "type": "regular"
            }
        }
    }
    """
    Then result contains the following documents on all hosts
    """
    [
        {
            "_id": "0",
            "name": "postgresql",
            "type": "regular"
        },
        {
            "_id": "1",
            "name": "mysql",
            "type": "regular"
        }
    ]
    """

  @backups
  Scenario: Create ElasticSearch backup
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we get ElasticSearch backups for "test_cluster" [grpc]
    Then we remember current response as "initial_backups"
    When we index the following documents to "test-backup"
    """
    [
        {
            "_id": "0",
            "name": "postgresql",
            "type": "regular"
        },
        {
            "_id": "1",
            "name": "mysql",
            "type": "regular"
        },
        {
            "_id": "2",
            "name": "mongodb",
            "type": "spec"
        },
        {
            "_id": "3",
            "name": "clickhouse",
            "type": "spec"
        }
    ]
    """
    And we create backup for ElasticSearch "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes" [grpc]
    When we get ElasticSearch backups for "test_cluster" [grpc]
    Then we remember current response as "backups"
    And message with "backups" list is larger than in "initial_backups"

  @backups
  Scenario: Restore cluster from backup
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we restore ElasticSearch using latest "backups" and config [grpc]
    """
    {
        "environment": "PRESTABLE",
        "name": "test_cluster_at_last_backup",
        "config_spec": {
            "admin_password": "password",
            "version": "{{ config.userdata.version }}",
            "elasticsearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "db1.micro",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240,
                    },
                },
            },
        },
        "hostSpecs": [{
            "zoneId": "man",
            "type": "DATA_NODE",
        }],
    }
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "20 minutes" [grpc]
    And "ElasticSearch" cluster "test_cluster_at_last_backup" is up and running [grpc]
    Then number of documents in "test-backup" equals to "4"
    When we search "test-backup" with the following query on all hosts
    """
    {
        "query": {
            "match_all": {}
        }
    }
    """
    Then result contains the following documents on all hosts
    """
    [
        {
            "_id": "0",
            "name": "postgresql",
            "type": "regular"
        },
        {
            "_id": "1",
            "name": "mysql",
            "type": "regular"
        },
        {
            "_id": "2",
            "name": "mongodb",
            "type": "spec"
        },
        {
            "_id": "3",
            "name": "clickhouse",
            "type": "spec"
        }
    ]
    """
    And cluster has no pending changes

  @backups
  Scenario: Remove restored cluster
    Given "ElasticSearch" cluster "test_cluster_at_last_backup" is up and running [grpc]
    When we attempt to delete "ElasticSearch" cluster "test_cluster_at_last_backup" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes" [grpc]
    And we are unable to find "ElasticSearch" cluster "test_cluster_at_last_backup" [grpc]

  Scenario: Modify ElasticSearch datanode settings
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      elasticsearch_spec:
        data_node:
          elasticsearch_config_7:
            fielddata_cache_size: 10%
            max_clause_count: 2048
    update_mask:
      paths:
        - config_spec.elasticsearch_spec.data_node.elasticsearch_config_7.fielddata_cache_size
        - config_spec.elasticsearch_spec.data_node.elasticsearch_config_7.max_clause_count
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And static setting "indices.fielddata.cache.size" of "data" node equals to "10%"
    And static setting "indices.query.bool.max_clause_count" of "data" node equals to "2048"
    And cluster has no pending changes

  @hosts @create
  Scenario: Add ElasticSearch host to cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to add hosts in "ElasticSearch" cluster "test_cluster"
    """
    host_specs:
      - zone_id: sas
        type: DATA_NODE
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    And  number of "master" nodes equals to "3"
    And  number of "data" nodes equals to "3"
    And cluster has no pending changes

  Scenario: Install plugins to Elasticsearch cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
        elasticsearch_spec:
            plugins:
                - analysis-icu
                - analysis-kuromoji
                - analysis-nori
                - analysis-phonetic
                - analysis-smartcn
                - analysis-stempel
                - analysis-ukrainian
                - ingest-attachment
                - mapper-annotated-text
                - mapper-murmur3
                - mapper-size
                - repository-azure
                - repository-gcs
                - repository-hdfs
                - repository-s3
                - transport-nio
    update_mask:
        paths:
            - config_spec.elasticsearch_spec.plugins
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "25 minutes" [grpc]
    And "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    And cluster has no pending changes
    And specified plugins installed on all nodes
        | plugin_name               |
        | analysis-icu              |
        | analysis-kuromoji         |
        | analysis-nori             |
        | analysis-phonetic         |
        | analysis-smartcn          |
        | analysis-stempel          |
        | analysis-ukrainian        |
        | ingest-attachment         |
        | mapper-annotated-text     |
        | mapper-murmur3            |
        | mapper-size               |
        | repository-azure          |
        | repository-gcs            |
        | repository-hdfs           |
        | repository-s3             |
        | transport-nio             |

  @hosts @delete
  Scenario: Cannot delete ElasticSearch host with unique data from cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When "DATA_NODE" host in "man" has CRIT for "es_unique_data"
    And we attempt to delete "DATA_NODE" host in "man" from "ElasticSearch" cluster "test_cluster"
    Then response status is "OK" [grpc]
    And generated task is finished with error in "7 minutes" with message
    """
    host has unique data for some indices and thus cannot be deleted
    """
    And "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    And  number of "master" nodes equals to "3"
    And  number of "data" nodes equals to "3"
    And cluster has no pending changes

  @hosts @delete
  Scenario: Delete ElasticSearch host from cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we reset all the juggler crits
    And we attempt to delete "DATA_NODE" host in "man" from "ElasticSearch" cluster "test_cluster"
    Then response status is "OK" [grpc]
    And generated task is finished within "7 minutes" [grpc]
    And "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    And  number of "master" nodes equals to "3"
    And  number of "data" nodes equals to "2"
    And cluster has no pending changes

  Scenario: Rename ElasticSearch cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    name: test_cluster_renamed
    update_mask:
      paths:
        - name
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "2 minutes" [grpc]
    And generated task has description "Update ElasticSearch cluster metadata" [grpc]
    And cluster has no pending changes
    When we execute command on host in geo "man"
    """
    grep -q "test_cluster_renamed" /etc/dbaas.conf
    """
    Then command retcode should be "0"

  Scenario: Delete ElasticSearch cluster
    When we attempt to delete "ElasticSearch" cluster "test_cluster_renamed" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And in worker_queue exists "elasticsearch_cluster_delete_metadata" task
    And this task is done
    And we are unable to find "ElasticSearch" cluster "test_cluster_renamed" [grpc]
    But s3 has bucket for cluster
    And in worker_queue exists "elasticsearch_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
