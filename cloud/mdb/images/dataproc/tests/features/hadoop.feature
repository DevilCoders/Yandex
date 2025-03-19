@hadoop
Feature: Minimal test with hdfs, mapreduce and yarn
    @create-hadoop
    Scenario: Hadoop cluster creates and working
        Given cluster name "hadoop"
        Given NAT network
        Given property monitoring:all_nodes = True
        And cluster with services: hdfs, mapreduce, yarn, spark
        And property dataproc:repo = https://dataproc.storage.yandexcloud.net/agent
        And property dataproc:version = testing
        When cluster created within 10 minutes

    Scenario Outline: Service on masternodes works
        Then service <service> on masternodes is running
        Examples: masternode services
            | service                        |
            | hadoop-hdfs@namenode           |
            | hadoop-hdfs@secondarynamenode  |
            | hadoop-yarn@resourcemanager    |
            | hadoop-yarn@timelineserver     |
            | hadoop-mapred@historyserver    |
            | spark@history-server           |
            | dataproc-agent                 |
            | telegraf                       |

    Scenario Outline: Service on datanodes works
        Then service <service> on datanodes is running
        Examples: datanode services
            | service                        |
            | hadoop-hdfs@datanode           |
            | hadoop-yarn@nodemanager        |
            | telegraf                       |

    Scenario Outline: Service on computenodes works
        Then service <service> on computenodes is running
        Examples: computenode services
            | service                        |
            | hadoop-yarn@nodemanager        |
            | telegraf                       |

    @s3
    Scenario: s3 listing and getting works
        When execute command
        """
        hadoop fs -ls s3a://{{ bucket }}/
        """
        Then command finishes
        And command stdout contains
        """
        s3a://{{ bucket }}/labels.json
        """

        When execute command
        """
        hadoop fs -cat s3a://{{ bucket }}/labels.json
        """
        Then command finishes
        And command stdout contains
        """
        {"owner": "{{ owner }}", "commit": "{{ commit }}", "branch": "{{ branch }}", "pr": "{{ pr }}", "created_at": "{{ created_at }}"}
        """

    @mapreduce
    Scenario: mapreduce example org.apache.hadoop.examples.QuasiMonteCarlo works
        When execute command
        """
        hadoop jar /usr/lib/hadoop-mapreduce/hadoop-mapreduce-examples.jar pi 2 100000
        """
        Then command finishes within 120 seconds
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
        Then command finishes within 120 seconds
        And command stdout contains
        """
        Pi is roughly 3.14
        """

    @scala
    Scenario: Scala version is 2.12.10  
        When execute command
        """
        scala -version
        """
        Then command finishes
        And command stderr contains
        """
        Scala code runner version 2.12.10
        """
        When execute command
        """
        spark-submit --version
        """
        Then command finishes
        And command stderr contains
        """
        Using Scala version 2.12.10
        """

    @yc-cli
    Scenario: yc cli is installed on cluster
        When execute command on masternode
        """
        source /etc/profile.d/yc_cli.sh && yc --version
        """
        Then command finishes
        And command stdout contains
        """
        Yandex Cloud CLI
        """

    @check
    Scenario: Syslog is present in cloud logging
        Then log has syslog entries within 120 seconds

    @check
    Scenario: Job log is present in cloud logging
        Then log has job log entries within 120 seconds
