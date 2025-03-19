@hadoop
Feature: Minimal test with mapreduce and spark over yarn
    Scenario: Hadoop cluster creates and working
        Given cluster name "hadoop-lw"
        And cluster topology is lightweight
        Given NAT network
        And cluster with services: mapreduce, yarn, spark
        When cluster created within 10 minutes

    Scenario Outline: Service on masternodes works
        Then service <service> on masternodes is running
        Examples: masternode services
            | service                        |
            | hadoop-yarn@resourcemanager    |
            | hadoop-yarn@timelineserver     |
            | hadoop-mapred@historyserver    |
            | spark@history-server           |
            | dataproc-agent                 |
            | telegraf                       |

    Scenario Outline: Service on computenodes works
        Then service <service> on computenodes is running
        Examples: computenode services
            | service                        |
            | hadoop-yarn@nodemanager        |

    # MDB-17198: dataproc-2.1: mapreduce doesn't works without hdfs datanodes
    # So, temporarly skip this unusal feature.    
    @mapreduce @skip
    Scenario: mapreduce example org.apache.hadoop.examples.QuasiMonteCarlo works
        When execute command
        """
        yarn jar /usr/lib/hadoop-mapreduce/hadoop-mapreduce-examples.jar pi 2 100000
        """
        Then command finishes
        And command stdout contains
        """
        Pi is 3.14
        """

    @spark
    Scenario: spark example org.apache.spark.examples.SparkPi works
        When execute command
        """
        spark-submit --class org.apache.spark.examples.SparkPi /usr/lib/spark/examples/jars/spark-examples.jar 1000
        """
        Then command finishes
        And command stdout contains
        """
        Pi is roughly 3.14
        """
