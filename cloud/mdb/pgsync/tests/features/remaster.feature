Feature: Check remaster logic

    @switchover
    Scenario Outline: Correct remaster from shut down after switchover
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
                    min_failover_timeout: 120
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
                            priority: 2
            postgresql2:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 1
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 3

        """
        Then container "postgresql3" is in <replication_type> group
        And container "postgresql2" is a replica of container "postgresql1"
        When we gracefully stop "pgsync" in container "postgresql2"
        And we kill "postgres" in container "postgresql2" with signal "9"
        And we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": "pgsync_postgresql1_1.pgsync_pgsync_net","timeline": 1}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        Then container "postgresql3" became a master
        And container "postgresql1" is a replica of container "postgresql3"
        And postgresql in container "postgresql1" was not rewinded
        When we start "pgsync" in container "postgresql2"
        Then <lock_type> "<lock_host>" has value "yes" for key "/pgsync/postgresql/all_hosts/pgsync_postgresql2_1.pgsync_pgsync_net/tried_remaster"
        And container "postgresql2" is a replica of container "postgresql3"
        And postgresql in container "postgresql2" was not rewinded

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |      yes      |      quorum      |
        | zookeeper | zookeeper1 |      no       |       sync       |
