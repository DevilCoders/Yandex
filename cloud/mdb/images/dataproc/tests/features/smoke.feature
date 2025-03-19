@smoke @skip
Feature: Smoke test
    Scenario: Smoke cluster with full service list creates and working
        Given cluster name "smoke"
        And cluster with all services
        When cluster created within 30 minutes

    Scenario Outline: Service on masternodes works
        Then service <service> on masternodes is running
        Examples: masternode services
            | service                        |
            | dataproc-agent                 |
            | telegraf                       |
            | hadoop-hdfs@namenode           |
            | hadoop-hdfs@secondarynamenode  |
            | hadoop-yarn@resourcemanager    |
            | hadoop-yarn@timelineserver     |
            | spark@history-server           |
            | postgresql                     |
            | livy                           |
            | hive-metastore                 |
            | hive-server2                   |
            | hbase@master                   |
            | hbase@rest                     |
            | hbase@thrift                   |
            | oozie                          |
            | zeppelin                       |
            | zookeeper-server               |

    Scenario Outline: Service on datanodes works
        Then service <service> on datanodes is running
        Examples: datanode services
            | service                        |
            | hadoop-hdfs@datanode           |
            | hadoop-yarn@nodemanager        |
            | hbase@regionserver             |

    Scenario Outline: Service on computenodes works
        Then service <service> on computenodes is running
        Examples: computenode services
            | service                        |
            | hadoop-yarn@nodemanager        |
