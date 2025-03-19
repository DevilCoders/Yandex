# Feature skipped because hive-3.1.2 doesn't work with hadoop-3.3
# and new hadoop-aws client
# MDB-17220: apply patches to hive-3.1.2 for support hadoop-3.3s
@hive @yarn @tez @s3 @skip
Feature: Hive on dataproc cluster works
    Scenario: Cluster with hive creates
        Given cluster name "hive"
        And cluster with services: hdfs, yarn, mapreduce, hive, tez
        And NAT network
        When cluster created within 10 minutes

    Scenario Outline: Service on masternodes works
        Then service <service> on masternodes is running
        Examples: masternode services
            | service         |
            | postgresql      |
            | hive-metastore  |
            | hive-server2    |

    @hivecli
    Scenario: hive cli shows tables
          When execute command
          """
          sudo -u hive hive cli -v -e 'show tables;'
          """
          Then command finishes
          And command stderr contains
          """
          OK
          Time taken:
          """

    @beeline
    Scenario: beeline shows tables
          When execute command
          """
          sudo -u hive beeline -n hive -u jdbc:hive2://localhost:10000/ -e "show tables;"
          """
          Then command finishes
          And command stdout contains
          """
          +-----------+
          | tab_name  |
          +-----------+
          +-----------+
          """

    @hivecli
    Scenario: hive cli creates table
          When execute command
          """
          sudo -u hive hive cli -v -e 'create table t100 (c1 int) stored as parquet;'
          """
          Then command finishes
          And command stderr contains
          """
          OK
          Time taken:
          """

    @hivecli
    Scenario: hive cli inserts value to table
          When execute command
          """
          sudo -u hive beeline -n hive -u jdbc:hive2://localhost:10000/ -e "insert into t100 select 100;"
          """
          Then command finishes within 120 seconds
          And command stderr contains
          """
          Completed executing command
          """
