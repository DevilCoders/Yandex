@init-action
Feature: initialization_actions work on cluster create and cluster restart correctly
    Scenario: Cluster created with initialization_actions
        Given cluster name "init-actions"
        Given NAT network
        And cluster with services: hdfs, yarn
        Given s3 object init-script-with-vars.sh
        """
        #!/bin/bash

        echo "ARGS $@" >> /var/log/yandex/test.log
        echo "CLUSTER_ID ${CLUSTER_ID}" >> /var/log/yandex/test.log
        echo "S3_BUCKET ${S3_BUCKET}" >> /var/log/yandex/test.log
        echo "ROLE ${ROLE}" >> /var/log/yandex/test.log
        echo "CLUSTER_SERVICES ${CLUSTER_SERVICES}" >> /var/log/yandex/test.log
        echo "MIN_WORKER_COUNT ${MIN_WORKER_COUNT}" >> /var/log/yandex/test.log
        echo "MAX_WORKER_COUNT ${MAX_WORKER_COUNT}" >> /var/log/yandex/test.log
        yc --version >> /var/log/yandex/test.log
        """
        And cluster has initialization_action s3a://{{ bucket }}/init-script-with-vars.sh with args ["arg1", "arg2"]
        When cluster created within 5 minutes


    Scenario: initialization_actions scripts finished successfully with env variables
        When execute command on masternode
        """
        cat /var/log/yandex/test.log
        """
        Then command finishes
        And command stdout contains
        """
        ARGS arg1 arg2
        CLUSTER_ID {{ bucket }}-init-actions
        S3_BUCKET {{ bucket }}
        ROLE masternode
        CLUSTER_SERVICES hdfs yarn
        MIN_WORKER_COUNT 2
        MAX_WORKER_COUNT 2
        Yandex Cloud CLI
        """
        When execute command on datanode
        """
        cat /var/log/yandex/test.log
        """
        Then command finishes
        And command stdout contains
        """
        ARGS arg1 arg2
        CLUSTER_ID {{ bucket }}-init-actions
        S3_BUCKET {{ bucket }}
        ROLE datanode
        CLUSTER_SERVICES hdfs yarn
        MIN_WORKER_COUNT 2
        MAX_WORKER_COUNT 2
        Yandex Cloud CLI
        """
        When execute command on computenode
        """
        cat /var/log/yandex/test.log
        """
        Then command finishes
        And command stdout contains
        """
        ARGS arg1 arg2
        CLUSTER_ID {{ bucket }}-init-actions
        S3_BUCKET {{ bucket }}
        ROLE computenode
        CLUSTER_SERVICES hdfs yarn
        MIN_WORKER_COUNT 2
        MAX_WORKER_COUNT 2
        Yandex Cloud CLI
        """

    Scenario: initialization_actions scripts aren't executed after restart
        Then cluster restarted
        When execute command on masternode
        """
        cat /var/log/yandex/test.log
        """
        Then command finishes
        And command stdout contains
        """
        ARGS arg1 arg2
        CLUSTER_ID {{ bucket }}-init-actions
        S3_BUCKET {{ bucket }}
        ROLE masternode
        CLUSTER_SERVICES hdfs yarn
        MIN_WORKER_COUNT 2
        MAX_WORKER_COUNT 2
        Yandex Cloud CLI
        """
