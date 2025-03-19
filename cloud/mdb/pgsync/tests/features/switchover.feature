Feature: Check switchover

    @switchover
    Scenario Outline: Check switchover <restart> restart
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: '<quorum_commit>'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 3
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 3
                    min_failover_timeout: 120
                    master_unavailability_timeout: 2
                    remaster_restart: '<remaster_restart>'
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
        When we remember postgresql start time in container "postgresql1"
        When we remember postgresql start time in container "postgresql2"
        When we remember postgresql start time in container "postgresql3"
        Then container "postgresql3" is in <replication_type> group
        When we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": "pgsync_postgresql1_1.pgsync_pgsync_net","timeline": 1}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        Then container "postgresql3" became a master
        And container "postgresql2" is a replica of container "postgresql3"
        And container "postgresql1" is a replica of container "postgresql3"
        And postgresql in container "postgresql3" was not restarted
        And postgresql in container "postgresql2" <restarted> restarted
        And postgresql in container "postgresql1" was restarted
        Then container "postgresql1" is in <replication_type> group
        When we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": "pgsync_postgresql3_1.pgsync_pgsync_net","timeline": 2}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        Then container "postgresql1" became a master
        And container "postgresql3" is a replica of container "postgresql1"
        And container "postgresql2" is a replica of container "postgresql1"
        And postgresql in container "postgresql3" was not rewinded
        And postgresql in container "postgresql2" was not rewinded
        When we stop container "postgresql2"
        And we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": "pgsync_postgresql1_1.pgsync_pgsync_net","timeline": 3}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we wait "30.0" seconds
        Then container "postgresql1" is master
        When we wait "90.0" seconds
        Then container "postgresql3" became a master
        And container "postgresql1" is a replica of container "postgresql3"

    Examples: <lock_type>, <lock_host>
        |   lock_type   |   lock_host    | quorum_commit | replication_type | restart | remaster_restart | restarted |
        |   zookeeper   |   zookeeper1   |      yes      |      quorum      |  with   |        yes       |    was    |
        |   zookeeper   |   zookeeper1   |      no       |       sync       |  with   |        yes       |    was    |
        |   zookeeper   |   zookeeper1   |      yes      |      quorum      | without |        no        |  was not  |
        |   zookeeper   |   zookeeper1   |      no       |       sync       | without |        no        |  was not  |


    Scenario Outline: Check failed promote on switchover
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    postgres_timeout: 5
                    quorum_commit: '<quorum_commit>'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 3
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 3
                    min_failover_timeout: 120
                    master_unavailability_timeout: 2
                commands:
                    promote: sleep 3 && false
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
        When we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": "pgsync_postgresql1_1.pgsync_pgsync_net","timeline": 1}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        When we wait "30.0" seconds
        Then container "postgresql1" is master
        And container "postgresql2" is a replica of container "postgresql1"
        And container "postgresql3" is a replica of container "postgresql1"
        And container "postgresql3" is in <replication_type> group

    Examples: <lock_type>, <lock_host>
        |   lock_type   |   lock_host    | quorum_commit | replication_type |
        |   zookeeper   |   zookeeper1   |      yes      |      quorum      |
        |   zookeeper   |   zookeeper1   |      no       |       sync       |


    @switchover_drop
    Scenario Outline: Incorrect switchover nodes being dropped
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 3
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 3
                    min_failover_timeout: 120
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
        When we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": null,"timeline": null}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        Then <lock_type> "zookeeper1" has value "None" for key "/pgsync/postgresql/switchover/master"
        Then <lock_type> "zookeeper1" has value "None" for key "/pgsync/postgresql/switchover/state"
        Then <lock_type> "zookeeper1" has value "None" for key "/pgsync/postgresql/switchover/lsn"
        Then <lock_type> "zookeeper1" has value "None" for key "/pgsync/postgresql/failover_state"
        Then container "postgresql1" is master
        And container "postgresql2" is a replica of container "postgresql1"
        And container "postgresql3" is a replica of container "postgresql1"

    Examples: <lock_type>, <lock_host>
        |   lock_type   |   lock_host    |
        |   zookeeper   |   zookeeper1   |
