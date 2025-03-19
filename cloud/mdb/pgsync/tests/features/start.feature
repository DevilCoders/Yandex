Feature: Check startup logic

    Scenario Outline: Pgsync restarts without zookeeper
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'no'
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
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_without_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" without replication slots
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
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql3" is in <replication_type> group
        And <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        When we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": "pgsync_postgresql1_1.pgsync_pgsync_net","timeline": 1}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        Then container "postgresql3" became a master
        And container "postgresql2" is a replica of container "postgresql3"
        And container "postgresql1" is a replica of container "postgresql3"
        Then container "postgresql1" is in <replication_type> group
        When we disconnect from network container "postgresql1"
        And we gracefully stop "pgsync" in container "postgresql1"
        And we start "pgsync" in container "postgresql1"
        And we wait "40.0" seconds
        And we connect to network container "postgresql1"
        Then container "postgresql1" is in <replication_type> group
        And <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
        """

    Examples: <lock_type>
        |   lock_type   |   lock_host    | quorum_commit | replication_type |
        |   zookeeper   |   zookeeper1   |      yes      |      quorum      |
        |   zookeeper   |   zookeeper1   |      no       |       sync       |
