@restart @trunk
Feature: Cluster works after restart
    Scenario: Clusters created and restarted
        Given cluster name "restart"
        And cluster with services: hdfs, mapreduce, yarn, spark, hive
        When cluster created within 10 minutes
        Then cluster restarted

    Scenario Outline: services works after restart
        Then service <service> on <role> is running
        Examples: masternode services
            | service                        | role         |
            | hadoop-hdfs@namenode           | masternodes  |
            | hadoop-hdfs@secondarynamenode  | masternodes  |
            | hadoop-yarn@resourcemanager    | masternodes  |
            | hadoop-yarn@timelineserver     | masternodes  |
            | hadoop-mapred@historyserver    | masternodes  |
            | spark@history-server           | masternodes  |
            | postgresql                     | masternodes  |
            | dataproc-agent                 | masternodes  |
            | telegraf                       | masternodes  |
            | hive-metastore                 | masternodes  |
            | hive-server2                   | masternodes  |
            | hadoop-hdfs@datanode           | datanodes    |
            | hadoop-yarn@nodemanager        | datanodes    |
            | hadoop-yarn@nodemanager        | computenodes |

    Scenario: HDFS is fine
        When execute command
        """
        sudo -u hdfs hdfs dfsadmin -safemode get
        """
        Then command finishes
        And command stdout contains
        """
        Safe mode is OFF
        """
        When execute command
        """
        sudo -u hdfs hdfs dfsadmin -report -dead
        """
        Then command finishes
        And command stdout contains
        """
        Dead datanodes (0)
        """
        When execute command
        """
        sudo -u hdfs hdfs dfsadmin -report
        """
        Then command finishes
        And command stdout contains
        """
        Under replicated blocks: 0
        """
        And command stdout contains
        """
        Missing blocks: 0
        """
        And command stdout contains
        """
        Blocks with corrupt replicas: 0
        """

    Scenario: YARN is fine
        When execute command
        """
        sudo -u yarn yarn node -list -states UNHEALTHY,DECOMMISSIONED,LOST,REBOOTED,DECOMMISSIONING,SHUTDOWN
        """
        Then command finishes
        And command stdout contains
        """
        Total Nodes:0
        """
