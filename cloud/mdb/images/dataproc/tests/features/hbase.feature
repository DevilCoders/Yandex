@hbase @zookeeper
Feature: Cluster with HBase works
    Scenario: Cluster with HBase and ZK creates
        Given cluster name "hbase"
        Given cluster topology is 1 1 0
        And cluster with services: hdfs, mapreduce, yarn, hbase, zookeeper
        When cluster created within 15 minutes

    Scenario Outline: Service on masternodes works
        Then service <service> on masternodes is running
        Examples: masternode services
            | service             |
            | zookeeper-server    |
            | hbase@master        |
            | hbase@rest          |
            | hbase@thrift        |

    Scenario: regionservers is running
        Then service hbase@regionserver on datanodes is running
        Then service hbase@regionserver on computenodes is running

    @zookeeper
    Scenario: zookeeper is up and running
        When execute command
        """
        /usr/lib/zookeeper/bin/zkServer.sh status
        """
        Then command finishes
        And command stdout contains
        """
        Mode: standalone
        """

    @hbase
    Scenario: HBase has alive region, master and rest servers
        When execute command
        """
        curl -s "http://localhost:8070/status/cluster"
        """
        Then command finishes
        And command stdout contains
        """
        1 live servers, 0 dead servers
        """

    @hbase
    Scenario: HBase DDL works
        When execute command
        """
        echo "create 'bar', 'foo'" | hbase shell -n
        """
        Then command finishes
        And command stdout contains
        """
        Created table bar
        """
        When execute command
        """
        echo "list" | hbase shell -n
        """
        Then command finishes
        And command stdout contains
        """
        TABLE
        bar
        1 row(s)
        """
