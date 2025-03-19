# Feature skipped because hive-3.1.2 doesn't work with hadoop-3.3
# and new hadoop-aws client
# MDB-17220: apply patches to hive-3.1.2 for support hadoop-3.3
@metastore @skip
Feature: Metastore works with spark
    Scenario: Cluster with hive and spark creates
        Given cluster name "metastore"
        # NAT needs for working with object storage
        And NAT network
        And cluster with services: hdfs, yarn, hive, spark, livy
        When cluster created within 10 minutes
        Then service livy on masternodes is running
        When tunnel to 8998 is open

    Scenario: database testdb1 created
        When start livy spark session session-1
        Then livy session started
        Given livy code
        """
        spark.sql("drop database if exists testdb1")
        spark.sql("create database testdb1 location 's3a://{{ bucket }}/testdb1'").show()
        """
        When execute livy statement
        Then livy statement finished
        Given livy code
        """
        spark.sql("show databases").show()
        """
        When execute livy statement
        Then livy statement finished
        And  livy output contains
        """
        +---------+
        |namespace|
        +---------+
        |  default|
        |  testdb1|
        +---------+
        """
        Then stop livy session

    Scenario: testdb1 visible from other spark sessions
        When start livy spark session session-2
        Then livy session started
        Given livy code
        """
        spark.sql("show databases").show()
        """
        When execute livy statement
        Then livy statement finished
        And  livy output contains
        """
        +---------+
        |namespace|
        +---------+
        |  default|
        |  testdb1|
        +---------+
        """

