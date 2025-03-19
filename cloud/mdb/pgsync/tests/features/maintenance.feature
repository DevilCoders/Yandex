Feature: Check maintenance mode

    Scenario Outline: Check container stop in maintanence mode
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
                config:
                    pgsync.conf:
                        global:
                            priority: 2
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 1
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" is in <replication_type> group
        And container "postgresql2" is a replica of container "postgresql1"
        And container "postgresql3" is a replica of container "postgresql1"
        When we set value "enable" for key "/pgsync/postgresql/maintenance" in <lock_type> "<lock_host>"
        And we stop container "postgresql1"
        And we wait "10.0" seconds
        And we start container "postgresql1"
        And we start "postgres" in container "postgresql1"
        And we wait "10.0" seconds
        When we set value "disable" for key "/pgsync/postgresql/maintenance" in <lock_type> "<lock_host>"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql1" became a master
        Then container "postgresql2" is in <replication_type> group
        And container "postgresql2" is a replica of container "postgresql1"
        And container "postgresql3" is a replica of container "postgresql1"
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |      yes      |      quorum      |
        | zookeeper | zookeeper1 |      no       |       sync       |



    Scenario Outline: Check pgbouncer is untouchable in maintenance mode
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
                config:
                    pgsync.conf:
                        global:
                            priority: 2
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 1
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is a replica of container "postgresql1"
        And container "postgresql3" is a replica of container "postgresql1"
        When we set value "enable" for key "/pgsync/postgresql/maintenance" in <lock_type> "<lock_host>"
        And we wait "10.0" seconds
        Then pgbouncer is running in container "postgresql1"
        When we disconnect from network container "postgresql1"
        And we wait "10.0" seconds
        Then pgbouncer is running in container "postgresql1"
        When we connect to network container "postgresql1"
        And we wait "10.0" seconds
        Then pgbouncer is running in container "postgresql1"
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | quorum_commit |
        | zookeeper | zookeeper1 |      yes      |
        | zookeeper | zookeeper1 |      no       |



    Scenario Outline: Sync replication turns off in maintenance
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
                    sync_replication_in_maintenance: 'no'
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
                            priority: 2
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 1
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" is in <replication_type> group
        And container "postgresql2" is a replica of container "postgresql1"
        And container "postgresql3" is a replica of container "postgresql1"
        When we set value "enable" for key "/pgsync/postgresql/maintenance" in <lock_type> "<lock_host>"
        And we wait "10.0" seconds
        Then container "postgresql1" replication state is "async"
        And  postgresql in container "postgresql1" has empty option "synchronous_standby_names"
        When we set value "disable" for key "/pgsync/postgresql/maintenance" in <lock_type> "<lock_host>"
        And we wait "10.0" seconds
        Then container "postgresql2" is in <replication_type> group
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |      yes      |      quorum      |
        | zookeeper | zookeeper1 |      no       |       sync       |


	@maintenance_master
    Scenario Outline: Node with current master exists in maintenance
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
                    sync_replication_in_maintenance: 'no'
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
                            priority: 2
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 1
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is a replica of container "postgresql1"
        And container "postgresql3" is a replica of container "postgresql1"
        And container "postgresql2" is in quorum group
        And container "postgresql3" is in quorum group
        When we set value "enable" for key "/pgsync/postgresql/maintenance" in <lock_type> "<lock_host>"
        Then <lock_type> "<lock_host>" has value "pgsync_postgresql1_1.pgsync_pgsync_net" for key "/pgsync/postgresql/maintenance/master"
        When we set value "disable" for key "/pgsync/postgresql/maintenance" in <lock_type> "<lock_host>"
        Then <lock_type> "<lock_host>" has no value for key "/pgsync/postgresql/maintenance/master"
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |
