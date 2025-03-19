@oozie
Feature: Cluster with oozie works
    Scenario: Hadoop cluster creates and working
        Given cluster name "oozie"
        And cluster topology is 1 1 0
        And cluster with services: hdfs, oozie
        When cluster created within 10 minutes

    Scenario: Service on masternodes works
        Then service oozie on masternodes is running

    Scenario: Oozie admin works
        When execute command
        """
        sudo -u oozie oozie admin -status
        """
        Then command finishes
        And command stdout contains
        """
        System mode: NORMAL
        """
