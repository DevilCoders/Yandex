Feature: Check WAL archiving works correctly

    @archiving
    Scenario Outline: Check that archive enabled after restart postgres without maintenance
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    autofailover: 'no'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 3
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 3
                    min_failover_timeout: 60
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
            postgresql.conf:
                archive_command: '/bin/true'
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And postgresql in container "postgresql1" has value "/bin/true" for option "archive_command"
        And container "postgresql1" has following config
        """
            postgresql.auto.conf: {}
        """
        When we set value "/bin/false" for option "archive_command" in "postgresql.auto.conf" config in container "postgresql1"
        Then postgresql in container "postgresql1" has value "/bin/true" for option "archive_command"
        And container "postgresql1" has following config
        """
            postgresql.auto.conf: {}
        """

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |
