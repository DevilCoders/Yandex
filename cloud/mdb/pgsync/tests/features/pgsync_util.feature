Feature: Check pgsync-util features

    @pgsync_util_maintenance
    Scenario Outline: Check pgsync-util maintenance works
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m show
        """
        Then command exit with return code "0"
        And command result is following output
        """
        disabled
        """
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m enable
        """
        Then command exit with return code "0"
        And command result is following output
        """
        """
        And <lock_type> "<lock_host>" has value "enable" for key "/pgsync/postgresql/maintenance"
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m show
        """
        Then command exit with return code "0"
        And command result is following output
        """
        enabled
        """
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m disable
        """
        Then command exit with return code "0"
        And command result is following output
        """
        """
        And <lock_type> "<lock_host>" has value "None" for key "/pgsync/postgresql/maintenance"
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m show
        """
        Then command exit with return code "0"
        And command result is following output
        """
        disabled
        """
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |


    @pgsync_util_maintenance
    Scenario Outline: Check pgsync-util maintenance enable with wait_all option works fails
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is in quorum group
        When we gracefully stop "pgsync" in container "postgresql2"
        And we gracefully stop "pgsync" in container "postgresql1"
        And we lock "/pgsync/postgresql/alive/pgsync_postgresql1_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we lock "/pgsync/postgresql/alive/pgsync_postgresql2_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m enable --wait_all --timeout 10
        """
        Then command exit with return code "1"
        And command result contains following output
        """
        TimeoutError
        """
        When we release lock "/pgsync/postgresql/alive/pgsync_postgresql1_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/alive/pgsync_postgresql2_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |


    @pgsync_util_maintenance
    Scenario Outline: Check pgsync-util maintenance with wait_all option works works
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is in quorum group
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m enable --wait_all --timeout 10
        """
        Then command exit with return code "0"
        And command result contains following output
        """
        Success
        """
        And <lock_type> "<lock_host>" has value "enable" for key "/pgsync/postgresql/maintenance"
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m disable --wait_all --timeout 10
        """
        Then command exit with return code "0"
        And command result contains following output
        """
        Success
        """
        And <lock_type> "<lock_host>" has value "None" for key "/pgsync/postgresql/maintenance"
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |


    @pgsync_util_maintenance
    Scenario Outline: Check pgsync-util maintenance disable with wait_all option works fails
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is in quorum group
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m enable --wait_all --timeout 10
        """
        Then command exit with return code "0"
        And command result contains following output
        """
        Success
        """
        And <lock_type> "<lock_host>" has value "enable" for key "/pgsync/postgresql/maintenance"
        When we gracefully stop "pgsync" in container "postgresql2"
        And we gracefully stop "pgsync" in container "postgresql1"
        And we lock "/pgsync/postgresql/alive/pgsync_postgresql1_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we lock "/pgsync/postgresql/alive/pgsync_postgresql2_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        When we run following command on host "postgresql1"
        """
        pgsync-util maintenance -m disable --wait_all --timeout 10
        """
        Then command exit with return code "1"
        And command result contains following output
        """
        TimeoutError
        """
        When we release lock "/pgsync/postgresql/alive/pgsync_postgresql1_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/alive/pgsync_postgresql2_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |


    @pgsync_util_switchover_single
    Scenario Outline: Check pgsync-util switchover single-node cluster works as expected
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        When we run following command on host "postgresql1"
        """
        pgsync-util switchover --yes --block
        """
        Then command exit with return code "1"
        And command result contains following output
        """
        Switchover is impossible now
        """
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |


    @pgsync_util_switchover_stream_from
    Scenario Outline: Check pgsync-util switchover single-node cluster works as expected
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            stream_from: pgsync_postgresql1_1.pgsync_pgsync_net
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        When we run following command on host "postgresql1"
        """
        pgsync-util switchover --yes --block
        """
        Then command exit with return code "1"
        And command result contains following output
        """
        Switchover is impossible now
        """
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |


    @pgsync_util_switchover
    Scenario Outline: Check pgsync-util switchover works as expected
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: '<quorum_commit>'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" is in <replication_type> group
        When we run following command on host "postgresql1"
        """
        pgsync-util switchover --yes --block
        """
        Then command exit with return code "0"
        And command result contains following output
        """
        switchover finished, zk status "None"
        """
        Then container "postgresql2" became a master
        And container "postgresql1" is a replica of container "postgresql2"
        And container "postgresql1" is in <replication_type> group
        And postgresql in container "postgresql1" was not rewinded
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | replication_type | quorum_commit |
        | zookeeper | zookeeper1 |      sync        |       no      |
        | zookeeper | zookeeper1 |      quorum      |      yes      |


    @pgsync_util_switchover
    Scenario Outline: Check pgsync-util targeted switchover works as expected
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: '<quorum_commit>'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
                config:
                    pgsync.conf:
                        global:
                            priority: 1
            postgresql2:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 2
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 3
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql3" is in <replication_type> group
        When we run following command on host "postgresql1"
        """
        pgsync-util switchover --yes --block --destination pgsync_postgresql2_1.pgsync_pgsync_net
        """
        Then command exit with return code "0"
        And command result contains following output
        """
        switchover finished, zk status "None"
        """
        Then container "postgresql2" became a master
        And container "postgresql1" is a replica of container "postgresql2"
        And container "postgresql3" is a replica of container "postgresql2"
        And container "postgresql3" is in <replication_type> group
        And postgresql in container "postgresql1" was not rewinded
        And postgresql in container "postgresql3" was not rewinded
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | replication_type | quorum_commit |
        | zookeeper | zookeeper1 |      sync        |       no      |
        | zookeeper | zookeeper1 |      quorum      |      yes      |


    @pgsync_util_switchover_reset
    Scenario Outline: Check pgsync-util switchover reset works as expected
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
                config:
                    pgsync.conf:
                        global:
                            priority: 1
            postgresql2:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 2
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 3
        """
        When we gracefully stop "pgsync" in container "postgresql1"
        And we gracefully stop "pgsync" in container "postgresql2"
        And we gracefully stop "pgsync" in container "postgresql3"
        And we lock "/pgsync/postgresql/alive/pgsync_postgresql1_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we lock "/pgsync/postgresql/alive/pgsync_postgresql2_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we lock "/pgsync/postgresql/alive/pgsync_postgresql3_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we lock "/pgsync/postgresql/leader" in <lock_type> "<lock_host>" with value "pgsync_postgresql1_1.pgsync_pgsync_net"
        And we run following command on host "postgresql1"
        """
        pgsync-util switchover --yes
        """
        Then command exit with return code "0"
        And command result contains following output
        """
        scheduled
        """
        Then <lock_type> "<lock_host>" has value "scheduled" for key "/pgsync/postgresql/switchover/state"
        And <lock_type> "<lock_host>" has value "{"hostname": "pgsync_postgresql1_1.pgsync_pgsync_net", "timeline": 1, "destination": null}" for key "/pgsync/postgresql/switchover/master"
        When we run following command on host "postgresql1"
        """
        pgsync-util switchover --reset
        """
        Then command exit with return code "0"
        And command result contains following output
        """
        resetting ZK switchover nodes
        """
        Then <lock_type> "<lock_host>" has value "failed" for key "/pgsync/postgresql/switchover/state"
        And <lock_type> "<lock_host>" has value "{}" for key "/pgsync/postgresql/switchover/master"
        When we run following command on host "postgresql1"
        """
        pgsync-util switchover --yes
        """
        Then command exit with return code "0"
        And command result contains following output
        """
        scheduled
        """
        Then <lock_type> "<lock_host>" has value "scheduled" for key "/pgsync/postgresql/switchover/state"
        And <lock_type> "<lock_host>" has value "{"hostname": "pgsync_postgresql1_1.pgsync_pgsync_net", "timeline": 1, "destination": null}" for key "/pgsync/postgresql/switchover/master"
        When we release lock "/pgsync/postgresql/alive/pgsync_postgresql1_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/alive/pgsync_postgresql2_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/alive/pgsync_postgresql3_1.pgsync_pgsync_net" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/leader" in <lock_type> "<lock_host>" with value "pgsync_postgresql1_1.pgsync_pgsync_net"
        And we start "pgsync" in container "postgresql1"
        And we start "pgsync" in container "postgresql2"
        And we start "pgsync" in container "postgresql3"
        Then <lock_type> "<lock_host>" has no value for key "/pgsync/postgresql/switchover/state"
        And <lock_type> "<lock_host>" has no value for key "/pgsync/postgresql/switchover/master"
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |
